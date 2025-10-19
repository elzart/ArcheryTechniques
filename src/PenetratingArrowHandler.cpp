#include "PenetratingArrowHandler.h"
#include "Config.h"
#include "MultishotHandler.h"
#include <RE/A/ArrowProjectile.h>
#include <RE/M/MissileProjectile.h>
#include <cmath>
#include <chrono>
#include <numbers>

PenetratingArrowHandler* PenetratingArrowHandler::GetSingleton()
{
    static PenetratingArrowHandler singleton;
    return &singleton;
}

RE::BSEventNotifyControl PenetratingArrowHandler::ProcessEvent(const RE::BSAnimationGraphEvent* a_event,
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

    // Debug: Log all animation events to see what's available
    SKSE::log::debug("PenetratingArrow: Animation event received: {}", a_event->tag.c_str());
    
    // Check for bow draw and arrow release animation events
    if (a_event->tag == "bowDraw" || a_event->tag == "bowDrawStart") {
        SKSE::log::info("PenetratingArrow: Bow draw started");
        OnBowDrawStart();
    } else if (a_event->tag == "arrowRelease") {
        SKSE::log::info("PenetratingArrow: Arrow release detected");
        OnArrowRelease();
    } else if (a_event->tag == "bowDrawStop" || a_event->tag == "bowRelease" || 
               a_event->tag == "bowUnDraw" || a_event->tag == "weaponSwing" ||
               a_event->tag == "weaponLeftSwing") {
        // These events indicate the bow is no longer being drawn
        SKSE::log::info("PenetratingArrow: Bow draw stopped (event: {})", a_event->tag.c_str());
        OnBowDrawStop();
    }

    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl PenetratingArrowHandler::ProcessEvent(RE::InputEvent* const* a_event, 
                                                             RE::BSTEventSource<RE::InputEvent*>* /*a_eventSource*/)
{
    if (!a_event || !*a_event) {
        return RE::BSEventNotifyControl::kContinue;
    }
    
    // Use input events as a trigger for regular updates
    // This ensures the penetrating arrow system updates regularly when the player is active
    Update();
    
    return RE::BSEventNotifyControl::kContinue;
}

void PenetratingArrowHandler::Update()
{
    auto* config = Config::GetSingleton();
    if (!config->penetratingArrow.enabled) {
        return;
    }

    // Try to register animation events if not already done
    // Keep trying until successful (player might not be available immediately)
    static bool registrationSuccessful = false;
    if (!registrationSuccessful) {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (player) {
            RE::BSTSmartPointer<RE::BSAnimationGraphManager> animationGraphManager;
            if (player->GetAnimationGraphManager(animationGraphManager) && animationGraphManager) {
                if (!animationGraphManager->graphs.empty()) {
                    animationGraphManager->graphs.front()->GetEventSource<RE::BSAnimationGraphEvent>()->AddEventSink(this);
                    SKSE::log::info("PenetratingArrow: Animation event handler registered successfully");
                    registrationSuccessful = true;
                }
            }
        }
    }

    // Debug: Log that update is being called (only occasionally to avoid spam)
    static int updateCounter = 0;
    updateCounter++;
    
    // Log first update to confirm system is working
    if (updateCounter == 1) {
        SKSE::log::info("PenetratingArrow: First update called - system is running");
    }
    
    if (updateCounter % 300 == 0) { // Log every 300 updates
        SKSE::log::debug("PenetratingArrow: Update called (counter: {})", updateCounter);
    }

    UpdateBowDrawState();
    CheckForStateTransitions();
    
    // Also update the multishot system to ensure its notifications work properly
    // The multishot system doesn't have its own update loop, so we help it here
    MultishotHandler::GetSingleton()->UpdateState();
}

void PenetratingArrowHandler::UpdateBowDrawState()
{
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        return;
    }

    // Check if player is in a valid state
    if (player->IsInKillMove() || player->IsDead()) {
        ResetState();
        return;
    }

    // Check if UI is open
    auto* ui = RE::UI::GetSingleton();
    if (ui && ui->GameIsPaused()) {
        return; // Don't reset state, just pause tracking
    }

    // Check if a valid bow is equipped
    auto* equippedWeapon = player->GetEquippedObject(false);
    auto* weapon = equippedWeapon ? equippedWeapon->As<RE::TESObjectWEAP>() : nullptr;
    
    if (!IsValidBow(weapon)) {
        ResetState();
        return;
    }

    // Check if multishot is active - if so, reset penetrating arrow state
    auto* multishotHandler = MultishotHandler::GetSingleton();
    if (multishotHandler->IsInReadyState()) {
        if (currentState == PenetratingArrowState::Charging || currentState == PenetratingArrowState::Charged) {
            SKSE::log::info("PenetratingArrow: Multishot activated - resetting penetrating arrow state");
            ResetState();
        }
        return;
    }

    // Check if player is still continuously drawing
    // Bow draw events fire every ~500-700ms while actively drawing
    // If we haven't seen one in >2 seconds, player stopped drawing mid-charge
    if (currentState == PenetratingArrowState::Charging) {
        auto now = std::chrono::steady_clock::now();
        auto timeSinceLastBowDraw = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastBowDrawTime).count();
        
        // If it's been more than 2 seconds since the last bow draw event,
        // the player stopped drawing (events fire every ~500-700ms when drawing)
        if (timeSinceLastBowDraw > 2000) {
            SKSE::log::info("PenetratingArrow: No bow draw events for {}ms - player stopped drawing, resetting", timeSinceLastBowDraw);
            ResetState();
        }
    }
}

void PenetratingArrowHandler::CheckForStateTransitions()
{
    auto* config = Config::GetSingleton();
    auto now = std::chrono::steady_clock::now();
    
    if (currentState == PenetratingArrowState::Charging) {
        // Check if charging time has elapsed
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - chargingStartTime).count() / 1000.0f;
        if (elapsed >= config->penetratingArrow.chargeTime) {
            currentState = PenetratingArrowState::Charged;
            SKSE::log::info("PenetratingArrow: Arrow fully charged!");
            // RE::DebugNotification("Penetrating Arrow: CHARGED");
        }
    } else if (currentState == PenetratingArrowState::Cooldown) {
        // Check if cooldown has finished
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - cooldownStartTime).count() / 1000.0f;
        if (elapsed >= config->penetratingArrow.cooldownDuration) {
            currentState = PenetratingArrowState::Inactive;
            SKSE::log::info("PenetratingArrow: Cooldown finished");
            // RE::DebugNotification("Penetrating Arrow: Ready");
        }
    }
}

void PenetratingArrowHandler::OnBowDrawStart()
{
    // Always update the last bow draw time when we receive bow draw events
    lastBowDrawTime = std::chrono::steady_clock::now();
    
    if (currentState == PenetratingArrowState::Inactive) {
        if (CanStartCharging()) {
            StartCharging();
        }
    }
    // If we receive a bow draw event while charging, update the last draw time
    // This keeps the continuous draw detection active
    else if (currentState == PenetratingArrowState::Charging) {
        // Bow draw event received - player is still drawing
        SKSE::log::debug("PenetratingArrow: Bow draw event while charging - continuous draw confirmed");
    }
}

void PenetratingArrowHandler::OnBowDrawStop()
{
    // Reset state on any bow draw stop event - player released the bow without firing
    // Note: BowRelease fires BEFORE arrowRelease, so we need to be careful here
    // Only reset if we're in Charging state, not if Charged (to allow firing)
    if (currentState == PenetratingArrowState::Charging) {
        SKSE::log::info("PenetratingArrow: Bow draw stopped while charging - resetting state");
        ResetState();
    }
    // Don't reset Charged state - the arrowRelease event will handle the charged arrow
    // BowRelease fires just before arrowRelease when firing, so we need to preserve charged state
}

void PenetratingArrowHandler::OnArrowRelease()
{
    if (currentState != PenetratingArrowState::Charged) {
        return; // Normal shot, do nothing
    }

    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        return;
    }

    auto* equippedWeapon = player->GetEquippedObject(false);
    auto* weapon = equippedWeapon ? equippedWeapon->As<RE::TESObjectWEAP>() : nullptr;
    auto* ammo = player->GetCurrentAmmo();

    if (!weapon || !ammo) {
        ResetState();
        return;
    }

    // Transition to cooldown state
    currentState = PenetratingArrowState::Cooldown;
    cooldownStartTime = std::chrono::steady_clock::now();
    
    auto* config = Config::GetSingleton();
    SKSE::log::info("PenetratingArrow: Penetrating shot fired! Starting cooldown for {} seconds", 
                   config->penetratingArrow.cooldownDuration);
    RE::DebugNotification(std::format("Penetrating Arrow: Cooldown ({:.0f}s)", 
                                     config->penetratingArrow.cooldownDuration).c_str());

    // Add a very small delay to let the game create the arrow first
    auto* taskInterface = SKSE::GetTaskInterface();
    if (taskInterface) {
        taskInterface->AddTask([this, player, weapon, ammo]() {
            // Small delay to ensure arrow exists
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            auto* taskInterface2 = SKSE::GetTaskInterface();
            if (taskInterface2) {
                taskInterface2->AddTask([this, player, weapon, ammo]() {
                    LaunchPenetratingArrow(player, weapon, ammo);
                });
            }
        });
    }
}

void PenetratingArrowHandler::LaunchPenetratingArrow(RE::PlayerCharacter* player, RE::TESObjectWEAP* /*weapon*/, RE::TESAmmo* /*ammo*/)
{
    SKSE::log::info("PenetratingArrow: Modifying arrow for penetrating behavior");
    
    // Find the most recently fired arrow and modify it
    auto* projectileManager = RE::Projectile::Manager::GetSingleton();
    if (!projectileManager) {
        return;
    }

    // Look for the player's most recent arrow
    RE::Projectile* targetArrow = nullptr;
    float shortestLivingTime = 0.5f; // Look at recent arrows (with 50ms delay this should be ~0.05s)
    int playerArrowsFound = 0;
    
    SKSE::log::debug("PenetratingArrow: Searching for player arrows in all projectile arrays...");
    
    // Helper lambda to search an array
    auto searchArray = [&](const RE::BSTArray<RE::ProjectileHandle>& array, const char* arrayName) {
        SKSE::log::debug("PenetratingArrow: Checking {} array (size: {})", arrayName, array.size());
        for (auto& projectileHandle : array) {
            auto projectilePtr = projectileHandle.get();
            auto* projectile = projectilePtr ? projectilePtr.get() : nullptr;
            
            if (projectile && projectile->IsMissileProjectile()) {
                auto& projData = projectile->GetProjectileRuntimeData();
                
                // Check if this is a player arrow
                auto shooterHandle = projData.shooter.get();
                auto* shooter = shooterHandle ? shooterHandle.get() : nullptr;
                
                if (shooter == player) {
                    playerArrowsFound++;
                    SKSE::log::debug("PenetratingArrow: Found player arrow in {} with livingTime: {:.3f}s", arrayName, projData.livingTime);
                    
                    if (projData.livingTime < shortestLivingTime) {
                        targetArrow = projectile;
                        shortestLivingTime = projData.livingTime;
                        SKSE::log::debug("PenetratingArrow: This is the newest arrow so far (from {})", arrayName);
                    }
                }
            }
        }
    };
    
    // Search all three projectile arrays
    searchArray(projectileManager->pending, "pending");
    searchArray(projectileManager->unlimited, "unlimited");
    searchArray(projectileManager->limited, "limited");
    
    SKSE::log::info("PenetratingArrow: Found {} player arrows total across all arrays", playerArrowsFound);
    
    if (targetArrow) {
        auto& projData = targetArrow->GetProjectileRuntimeData();
        
        // Modify projectile for penetrating behavior
        // Increase power for better penetration
        projData.power = 2.0f;
        
        // Keep the kDestroyAfterHit flag - we want it to stop at walls, just not at NPCs
        // The kImpale impact result should handle NPC penetration specifically
        
        // Increase speed multiplier for better penetration
        projData.speedMult = 1.5f;
        
        // Try to remove enchantment and set penetration behavior if this is an ArrowProjectile
        auto* arrowProjectile = targetArrow->As<RE::ArrowProjectile>();
        if (arrowProjectile) {
            auto& arrowData = arrowProjectile->GetArrowRuntimeData();
            if (arrowData.enchantItem) {
                SKSE::log::info("PenetratingArrow: Removing enchantment from arrow");
                arrowData.enchantItem = nullptr;
            }
        }
        
        // Set impact result to allow damage but continue through targets
        auto* missileProjectile = targetArrow->As<RE::MissileProjectile>();
        if (missileProjectile) {
            auto& missileData = missileProjectile->GetMissileRuntimeData();
            // Set to kImpale instead of kDestroy - this should allow damage but continue through
            missileData.impactResult = RE::ImpactResult::kImpale;
            SKSE::log::info("PenetratingArrow: Set impactResult to kImpale for penetration with damage");
        }
        
        // Instead of making it pass through everything, we need a more targeted approach
        // We'll use a different method that allows damage to NPCs but continues through them
        
        // Don't modify the base projectile type - that made it pass through everything
        // Instead, we'll use the runtime approach with better collision handling
        
        SKSE::log::info("PenetratingArrow: Using selective penetration - should hit NPCs but pass through them");
        
        SKSE::log::info("PenetratingArrow: Successfully modified arrow for penetrating behavior (power: {:.2f}, speedMult: {:.2f})", 
                       projData.power, projData.speedMult);
    } else {
        SKSE::log::warn("PenetratingArrow: Could not find player arrow to modify");
    }
}

void PenetratingArrowHandler::StartCharging()
{
    currentState = PenetratingArrowState::Charging;
    chargingStartTime = std::chrono::steady_clock::now();
    
    auto* config = Config::GetSingleton();
    SKSE::log::info("PenetratingArrow: Started charging for {} seconds", config->penetratingArrow.chargeTime);
    // RE::DebugNotification("Penetrating Arrow: Charging...");
}

void PenetratingArrowHandler::ResetState()
{
    if (currentState != PenetratingArrowState::Inactive && currentState != PenetratingArrowState::Cooldown) {
        SKSE::log::debug("PenetratingArrow: Resetting state to inactive");
    }
    currentState = PenetratingArrowState::Inactive;
}

void PenetratingArrowHandler::RegisterAnimationEventHandler()
{
    static bool registered = false;
    if (registered) {
        SKSE::log::debug("PenetratingArrow: Animation event handler already registered");
        return; // Already registered
    }
    
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        SKSE::log::warn("PenetratingArrow: Player not available for animation event registration");
        return;
    }
    
    RE::BSTSmartPointer<RE::BSAnimationGraphManager> animationGraphManager;
    if (player->GetAnimationGraphManager(animationGraphManager) && animationGraphManager) {
        if (!animationGraphManager->graphs.empty()) {
            // Register this PenetratingArrowHandler as an animation event sink
            animationGraphManager->graphs.front()->GetEventSource<RE::BSAnimationGraphEvent>()->AddEventSink(this);
            SKSE::log::info("PenetratingArrow: Animation event handler registered successfully");
            registered = true;
        } else {
            SKSE::log::error("PenetratingArrow: No animation graphs available");
        }
    } else {
        SKSE::log::error("PenetratingArrow: Failed to get AnimationGraphManager");
    }
}

// State query methods
bool PenetratingArrowHandler::IsCharged() const
{
    return currentState == PenetratingArrowState::Charged;
}

bool PenetratingArrowHandler::IsCharging() const
{
    return currentState == PenetratingArrowState::Charging;
}

bool PenetratingArrowHandler::IsOnCooldown() const
{
    return currentState == PenetratingArrowState::Cooldown;
}

PenetratingArrowState PenetratingArrowHandler::GetCurrentState() const
{
    return currentState;
}

float PenetratingArrowHandler::GetChargingProgress() const
{
    if (currentState != PenetratingArrowState::Charging) {
        return 0.0f;
    }
    
    auto* config = Config::GetSingleton();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - chargingStartTime).count() / 1000.0f;
    float progress = elapsed / config->penetratingArrow.chargeTime;
    return std::min(1.0f, progress);
}

float PenetratingArrowHandler::GetRemainingCooldownTime() const
{
    if (currentState != PenetratingArrowState::Cooldown) {
        return 0.0f;
    }
    
    auto* config = Config::GetSingleton();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - cooldownStartTime).count() / 1000.0f;
    float remaining = config->penetratingArrow.cooldownDuration - elapsed;
    return std::max(0.0f, remaining);
}

// Utility methods
bool PenetratingArrowHandler::CanStartCharging() const
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

    // Check if perk is required and if player has it
    auto* config = Config::GetSingleton();
    if (config->enablePerks) {
        if (!Config::HasPenetratePerk()) {
            SKSE::log::debug("PenetratingArrow: Player does not have required perk");
            return false;
        }
    }

    // Check if multishot is active - cannot fire penetrating arrow with multishot
    auto* multishotHandler = MultishotHandler::GetSingleton();
    if (multishotHandler->IsInReadyState()) {
        SKSE::log::debug("PenetratingArrow: Cannot start charging - multishot is in ready state");
        return false;
    }

    // Can only start charging if currently inactive
    return currentState == PenetratingArrowState::Inactive;
}

bool PenetratingArrowHandler::IsValidBow(RE::TESObjectWEAP* weapon) const
{
    if (!weapon) {
        return false;
    }

    // Check if it's a bow (not crossbow)
    return weapon->GetWeaponType() == RE::WEAPON_TYPE::kBow;
}

// Stamina methods removed - no stamina cost for penetrating arrows
