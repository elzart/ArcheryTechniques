#include "MultishotHandler.h"
#include "PenetratingArrowHandler.h"
#include "Config.h"
#include <cmath>
#include <chrono>
#include <numbers>
#include <vector>

MultishotHandler* MultishotHandler::GetSingleton()
{
    static MultishotHandler singleton;
    return &singleton;
}

RE::BSEventNotifyControl MultishotHandler::ProcessEvent(RE::InputEvent* const* a_event, 
                                                       RE::BSTEventSource<RE::InputEvent*>* /*a_eventSource*/)
{
    if (!a_event || !*a_event) {
        return RE::BSEventNotifyControl::kContinue;
    }

    auto* config = Config::GetSingleton();
    if (!config->multishot.enabled) {
        return RE::BSEventNotifyControl::kContinue;
    }

    for (auto* event = *a_event; event; event = event->next) {
        if (event->GetEventType() != RE::INPUT_EVENT_TYPE::kButton) {
            continue;
        }

        auto* buttonEvent = event->AsButtonEvent();
        if (!buttonEvent || buttonEvent->GetDevice() != RE::INPUT_DEVICE::kKeyboard) {
            continue;
        }

        // Check if it's our configured key and it's being pressed (not released)
        if (buttonEvent->GetIDCode() == static_cast<std::uint32_t>(config->multishot.keyCode) && 
            buttonEvent->IsDown()) {  // Use IsDown() for key press detection
            
            // Add debouncing to prevent double-triggering
            auto now = std::chrono::steady_clock::now();
            if ((now - lastActivationTime) >= std::chrono::milliseconds(200)) {
                if (CanActivateReadyState()) {
                    ActivateReadyState();
                    lastActivationTime = now;
                }
            }
            
            // Don't consume this event - let it pass through
            break;
        }
    }

    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl MultishotHandler::ProcessEvent(const RE::BSAnimationGraphEvent* a_event,
                                                       RE::BSTEventSource<RE::BSAnimationGraphEvent>* /*a_eventSource*/)
{
    if (!a_event || !a_event->holder) {
        return RE::BSEventNotifyControl::kContinue;
    }

    // Only process events for the player
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player || a_event->holder != player) {
        return RE::BSEventNotifyControl::kContinue;
    }

    // Check for arrow release animation event - only process "arrowRelease", NOT "bowRelease"
    // This prevents duplicate processing
    if (a_event->tag == "arrowRelease") {
        SKSE::log::info("Arrow release detected: {}", a_event->tag.c_str());
        OnArrowRelease();
    }

    return RE::BSEventNotifyControl::kContinue;
}

void MultishotHandler::ActivateReadyState()
{
    // Update state before checking - this handles expiration and cooldown transitions
    UpdateState();
    
    if (currentState != MultishotState::Inactive) {
        // Already in ready state or on cooldown
        if (currentState == MultishotState::Ready) {
            SKSE::log::info("Multishot already in ready state");
            RE::DebugNotification("Multishot: Already READY");
        } else if (currentState == MultishotState::Cooldown) {
            float remainingCooldown = GetRemainingCooldownTime();
            SKSE::log::info("Multishot on cooldown, {} seconds remaining", remainingCooldown);
            RE::DebugNotification(std::format("Multishot: Cooldown ({:.0f}s)", remainingCooldown).c_str());
        }
        return;
    }
    
    // Activate ready state
    currentState = MultishotState::Ready;
    readyStateStartTime = std::chrono::steady_clock::now();
    
    // Register animation event handler when first activated
    RegisterAnimationEventHandler();
    
    auto* config = Config::GetSingleton();
    SKSE::log::info("Multishot ready state activated for {} seconds", config->multishot.readyWindowDuration);
    RE::DebugNotification("Multishot: READY");
}

void MultishotHandler::RegisterAnimationEventHandler()
{
    static bool registered = false;
    if (registered) {
        return; // Already registered
    }
    
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (player) {
        RE::BSTSmartPointer<RE::BSAnimationGraphManager> animationGraphManager;
        if (player->GetAnimationGraphManager(animationGraphManager) && animationGraphManager) {
            if (!animationGraphManager->graphs.empty()) {
                // Register this MultishotHandler as an animation event sink
                animationGraphManager->graphs.front()->GetEventSource<RE::BSAnimationGraphEvent>()->AddEventSink(this);
                SKSE::log::info("Animation event handler registered successfully");
                registered = true;
            } else {
                SKSE::log::error("No animation graphs available");
            }
        } else {
            SKSE::log::error("Failed to get AnimationGraphManager in toggle");
        }
    }
}

bool MultishotHandler::CanActivateReadyState()
{
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        return false;
    }

    // Check if player is in a valid state
    if (player->IsInKillMove() || player->IsDead()) {
        return false;
    }

    // Check if UI is open
    auto* ui = RE::UI::GetSingleton();
    if (ui && ui->GameIsPaused()) {
        return false;
    }

    // Check if a valid bow is equipped
    auto* equippedWeapon = player->GetEquippedObject(false);
    auto* weapon = equippedWeapon ? equippedWeapon->As<RE::TESObjectWEAP>() : nullptr;
    
    if (!IsValidBow(weapon)) {
        return false;
    }

    // Update state to check for transitions
    UpdateState();
    
    // Can only activate if currently inactive
    return currentState == MultishotState::Inactive;
}

bool MultishotHandler::IsValidBow(RE::TESObjectWEAP* weapon)
{
    if (!weapon) {
        return false;
    }

    // Check if it's a bow (not crossbow)
    return weapon->GetWeaponType() == RE::WEAPON_TYPE::kBow;
}

// This gets called by your animation event handler when player releases arrow
void MultishotHandler::OnArrowRelease()
{
    // Update state to handle any transitions
    UpdateState();
    
    if (currentState != MultishotState::Ready) {
        return; // Normal shot, do nothing
    }

    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        return;
    }

    auto* config = Config::GetSingleton();
    int arrowCount = config->multishot.arrowCount;
    
    if (arrowCount < 2) {
        return;
    }

    // The vanilla arrow has already been fired, so we fire (arrowCount - 1) additional arrows
    int additionalArrows = arrowCount - 1;
    
    if (!HasSufficientAmmo(additionalArrows)) {
        SKSE::log::info("Insufficient ammo for multishot");
        RE::DebugNotification("Multishot: Insufficient ammo");
        return;
    }

    auto* equippedWeapon = player->GetEquippedObject(false);
    auto* weapon = equippedWeapon ? equippedWeapon->As<RE::TESObjectWEAP>() : nullptr;
    auto* ammo = player->GetCurrentAmmo();

    if (!weapon || !ammo) {
        return;
    }

    // Transition to cooldown state
    currentState = MultishotState::Cooldown;
    cooldownStartTime = std::chrono::steady_clock::now();
    
    SKSE::log::info("Multishot triggered! Starting cooldown for {} seconds", config->multishot.cooldownDuration);
    RE::DebugNotification(std::format("Multishot: Cooldown ({:.0f}s)", config->multishot.cooldownDuration).c_str());

    // Delay multishot launch to let vanilla arrow launch completely first
    auto* taskInterface = SKSE::GetTaskInterface();
    if (!taskInterface) {
        return;
    }

    // Capture all needed data for the delayed launch
    taskInterface->AddTask([this, player, weapon, ammo, arrowCount, additionalArrows]() {
        LaunchMultishotArrows(player, weapon, ammo, arrowCount, additionalArrows);
    });
}

void MultishotHandler::LaunchMultishotArrows(RE::PlayerCharacter* player, RE::TESObjectWEAP* weapon, RE::TESAmmo* ammo, int arrowCount, int additionalArrows)
{
    auto* config = Config::GetSingleton();
    if (!config) {
        SKSE::log::error("Could not get config singleton");
        return;
    }

    // Get the fire node and calculate proper angles like the game does
    auto* currentProcess = player->GetActorRuntimeData().currentProcess;
    if (!currentProcess) {
        SKSE::log::error("Player has no current process");
        return;
    }
    
    const auto& biped = player->GetBiped2();
    auto* fireNode = weapon->IsCrossbow() ? currentProcess->GetMagicNode(biped) : currentProcess->GetWeaponNode(biped);
    
    if (!fireNode) {
        SKSE::log::error("Could not get fire node");
        return;
    }
    
    // Get the vanilla origin and angles
    RE::NiPoint3 origin = fireNode->world.translate;
    RE::Projectile::ProjectileRot baseAngles{};
    
    // Let the game calculate the proper angles using Unk_A0
    player->Unk_A0(fireNode, baseAngles.x, baseAngles.z, origin);
    
    // Calculate the target velocity magnitude from weapon speed
    float arrowSpeed = weapon->weaponData.speed * 1500.0f;
    
    SKSE::log::info("Firing {} additional arrows with spread angle {}", additionalArrows, config->multishot.spreadAngle);
    SKSE::log::info("DEBUG: Base pitch: {:.3f}°, yaw: {:.3f}°, target speed: {:.3f}", 
                   baseAngles.x * 180.0f / std::numbers::pi_v<float>,
                   baseAngles.z * 180.0f / std::numbers::pi_v<float>,
                   arrowSpeed);
    
    // Calculate spread pattern
    // For 3 arrows with 15° spread: left at -7.5°, center at 0° (vanilla), right at +7.5°
    // Since vanilla arrow is at center (0°), we only fire the side arrows
    float totalSpread = config->multishot.spreadAngle * (arrowCount - 1);
    float startAngle = -totalSpread / 2.0f;
    float angleIncrement = (arrowCount > 1) ? totalSpread / (arrowCount - 1) : 0.0f;
    
    struct ArrowData {
        RE::ProjectileHandle handle;
        float targetSpeed;
    };
    std::vector<ArrowData> launchedArrows;
    
    // Launch each arrow with modified yaw angle (horizontal spread only)
    // We need to launch arrows at positions 0, 1, 2, ... but SKIP the center position
    // For arrowCount=3: positions are -7.5° (i=0), 0° (SKIP, vanilla), +7.5° (i=2)
    int centerIndex = (arrowCount - 1) / 2; // For 3 arrows, center is at index 1
    
    for (int i = 0; i < arrowCount; ++i) {
        // Skip the center arrow (vanilla arrow is already there)
        if (i == centerIndex) {
            continue;
        }
        
        float currentSpread = startAngle + (i * angleIncrement);
        float spreadRadians = currentSpread * std::numbers::pi_v<float> / 180.0f;
        
        // Apply spread to yaw angle only (horizontal spread)
        float adjustedAngleZ = baseAngles.z + spreadRadians;
        
        // Calculate offset position to prevent arrow collision
        // Offset arrows horizontally based on their spread direction
        RE::NiPoint3 offsetOrigin = origin;
        
        // Get camera right vector for horizontal offset
        auto* camera = RE::PlayerCamera::GetSingleton();
        if (camera && camera->cameraRoot) {
            auto* cameraNode = camera->cameraRoot.get();
            if (cameraNode) {
                // Get right vector from camera (perpendicular to view direction)
                RE::NiPoint3 rightVector = cameraNode->world.rotate.GetVectorX();
                
                // Offset by 5 units per arrow position from center
                float offsetDistance = (i - centerIndex) * 5.0f;
                offsetOrigin.x += rightVector.x * offsetDistance;
                offsetOrigin.y += rightVector.y * offsetDistance;
                offsetOrigin.z += rightVector.z * offsetDistance;
                
                SKSE::log::info("DEBUG: Arrow {} offset by {:.3f} units horizontally", i, offsetDistance);
            }
        }
        
        SKSE::log::info("DEBUG: Arrow {} - Spread: {:.3f}°, Pitch: {:.3f}°, Yaw: {:.3f}°", 
                       i, currentSpread,
                       baseAngles.x * 180.0f / std::numbers::pi_v<float>,
                       adjustedAngleZ * 180.0f / std::numbers::pi_v<float>);
        
        // Use LaunchArrow with the offset origin and spread angles
        RE::ProjectileHandle projectileHandle;
        if (RE::Projectile::LaunchArrow(&projectileHandle, player, ammo, weapon, offsetOrigin, 
                                       RE::Projectile::ProjectileRot{baseAngles.x, adjustedAngleZ})) {
            launchedArrows.push_back({projectileHandle, arrowSpeed});
            SKSE::log::info("DEBUG: Arrow {} launched successfully", i);
        } else {
            SKSE::log::error("DEBUG: Arrow {} failed to launch", i);
        }
    }
    
    // Set speedMult and power immediately for launched arrows
    for (const auto& arrowData : launchedArrows) {
        auto projectilePtr = arrowData.handle.get();
        if (projectilePtr) {
            auto* proj = projectilePtr.get();
            if (proj) {
                auto& runtimeData = proj->GetProjectileRuntimeData();
                runtimeData.power = 1.0f;
                runtimeData.speedMult = 1.0f;
                
                SKSE::log::info("DEBUG: Set power={:.3f}, speedMult={:.3f} for multishot arrow", 
                              runtimeData.power, runtimeData.speedMult);
            }
        }
    }
    
    // Add debugging for vanilla arrow using a task
    if (!launchedArrows.empty()) {
        auto* taskInterface = SKSE::GetTaskInterface();
        if (taskInterface) {
            taskInterface->AddTask([player]() {
                // Find and debug the vanilla arrow
                auto* projectileManager = RE::Projectile::Manager::GetSingleton();
                if (!projectileManager) {
                    return;
                }
                
                SKSE::log::info("DEBUG: Searching for vanilla arrow...");
                int playerArrowsFound = 0;
                
                // Check unlimited projectiles
                for (auto& projectileHandle : projectileManager->unlimited) {
                    auto projectilePtr = projectileHandle.get();
                    auto* projectile = projectilePtr ? projectilePtr.get() : nullptr;
                    
                    if (projectile && projectile->IsMissileProjectile()) {
                        auto& projData = projectile->GetProjectileRuntimeData();
                        
                        // Check if this is a player arrow
                        auto shooterHandle = projData.shooter.get();
                        auto* shooter = shooterHandle ? shooterHandle.get() : nullptr;
                        
                        if (shooter == player && projData.livingTime < 0.2f) {
                            playerArrowsFound++;
                            
                            auto velocity = projData.velocity;
                            float speed = velocity.Length();
                            
                            SKSE::log::info("DEBUG: Player arrow #{} - livingTime: {:.3f}s, velocity: ({:.3f}, {:.3f}, {:.3f}), speed: {:.3f}, power: {:.3f}, speedMult: {:.3f}", 
                                          playerArrowsFound, projData.livingTime,
                                          velocity.x, velocity.y, velocity.z, speed,
                                          projData.power, projData.speedMult);
                        }
                    }
                }
                
                SKSE::log::info("DEBUG: Found {} player arrows total", playerArrowsFound);
            });
        }
        
        ConsumeAmmo(static_cast<int>(launchedArrows.size()));
        SKSE::log::info("Successfully launched {} additional arrows", launchedArrows.size());
    }
}

bool MultishotHandler::HasSufficientAmmo(int requiredCount)
{
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        return false;
    }

    auto* ammo = player->GetCurrentAmmo();
    if (!ammo) {
        return false;
    }

    auto inventory = player->GetInventory();
    auto it = inventory.find(ammo);
    if (it != inventory.end()) {
        auto& [count, entry] = it->second;
        return count >= requiredCount;
    }

    return false;
}

void MultishotHandler::ConsumeAmmo(int count)
{
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        return;
    }

    auto* ammo = player->GetCurrentAmmo();
    if (!ammo) {
        return;
    }

    // Remove arrows from inventory
    player->RemoveItem(ammo, count, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
    
    SKSE::log::debug("Consumed {} arrows", count);
}

// State query methods
bool MultishotHandler::IsInReadyState() const
{
    return currentState == MultishotState::Ready;
}

bool MultishotHandler::IsOnCooldown() const
{
    return currentState == MultishotState::Cooldown;
}

MultishotState MultishotHandler::GetCurrentState() const
{
    return currentState;
}

float MultishotHandler::GetRemainingReadyTime() const
{
    if (currentState != MultishotState::Ready) {
        return 0.0f;
    }
    
    auto* config = Config::GetSingleton();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - readyStateStartTime).count() / 1000.0f;
    float remaining = config->multishot.readyWindowDuration - elapsed;
    return std::max(0.0f, remaining);
}

float MultishotHandler::GetRemainingCooldownTime() const
{
    if (currentState != MultishotState::Cooldown) {
        return 0.0f;
    }
    
    auto* config = Config::GetSingleton();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - cooldownStartTime).count() / 1000.0f;
    float remaining = config->multishot.cooldownDuration - elapsed;
    return std::max(0.0f, remaining);
}

void MultishotHandler::UpdateState()
{
    auto* config = Config::GetSingleton();
    auto now = std::chrono::steady_clock::now();
    
    if (currentState == MultishotState::Ready) {
        // Check if ready window has expired
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - readyStateStartTime).count() / 1000.0f;
        if (elapsed >= config->multishot.readyWindowDuration) {
            currentState = MultishotState::Inactive;
            SKSE::log::info("Multishot ready window expired");
            RE::DebugNotification("Multishot: Expired");
        }
    } else if (currentState == MultishotState::Cooldown) {
        // Check if cooldown has finished
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - cooldownStartTime).count() / 1000.0f;
        if (elapsed >= config->multishot.cooldownDuration) {
            currentState = MultishotState::Inactive;
            SKSE::log::info("Multishot cooldown finished");
            RE::DebugNotification("Multishot: Ready to activate");
        }
    }
    
    // Penetrating arrow system is now fully independent
}