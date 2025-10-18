#pragma once

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <chrono>

enum class PenetratingArrowState {
    Inactive,   // Normal state, not tracking bow draw
    Drawing,    // Bow is being drawn
    Charging,   // Bow is fully drawn, charging for penetrating shot
    Charged,    // Arrow is charged and ready for penetrating shot
    Cooldown    // Cooldown period after firing penetrating arrow
};

class PenetratingArrowHandler : 
    public RE::BSTEventSink<RE::BSAnimationGraphEvent>,
    public RE::BSTEventSink<RE::InputEvent*>
{
public:
    static PenetratingArrowHandler* GetSingleton();

    // BSTEventSink overrides
    RE::BSEventNotifyControl ProcessEvent(const RE::BSAnimationGraphEvent* a_event,
                                         RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource) override;
    RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event,
                                         RE::BSTEventSource<RE::InputEvent*>* a_eventSource) override;

    // Core functionality
    void Update(); // Called periodically to check bow state
    void OnBowDrawStart(); // Called when bow draw starts
    void OnBowDrawStop(); // Called when bow draw stops
    void OnArrowRelease(); // Called when arrow is released
    void StartCharging(); // Begin charging process
    void ResetState(); // Reset to inactive state
    
    // State queries
    bool IsCharged() const;
    bool IsCharging() const;
    bool IsOnCooldown() const;
    PenetratingArrowState GetCurrentState() const;
    float GetChargingProgress() const; // Returns 0.0-1.0 progress
    float GetRemainingCooldownTime() const;
    
    // Utility methods
    bool CanStartCharging() const;
    bool IsValidBow(RE::TESObjectWEAP* weapon) const;
    void RegisterAnimationEventHandler();
    void LaunchPenetratingArrow(RE::PlayerCharacter* player, RE::TESObjectWEAP* weapon, RE::TESAmmo* ammo);
    
private:
    PenetratingArrowState currentState = PenetratingArrowState::Inactive;
    std::chrono::steady_clock::time_point chargingStartTime{};
    std::chrono::steady_clock::time_point cooldownStartTime{};
    std::chrono::steady_clock::time_point lastBowDrawTime{};
    
    // Internal methods
    void UpdateBowDrawState();
    void CheckForStateTransitions();
    
    PenetratingArrowHandler() = default;
    ~PenetratingArrowHandler() = default;
    PenetratingArrowHandler(const PenetratingArrowHandler&) = delete;
    PenetratingArrowHandler(PenetratingArrowHandler&&) = delete;
    PenetratingArrowHandler& operator=(const PenetratingArrowHandler&) = delete;
    PenetratingArrowHandler& operator=(PenetratingArrowHandler&&) = delete;
};
