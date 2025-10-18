#pragma once

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <chrono>

enum class MultishotState {
    Inactive,   // Normal state, multishot not available
    Ready,      // Ready window active, next shot will be multishot
    Cooldown    // Cooldown period, cannot activate ready state
};

class MultishotHandler : 
    public RE::BSTEventSink<RE::InputEvent*>,
    public RE::BSTEventSink<RE::BSAnimationGraphEvent>
{
public:
    static MultishotHandler* GetSingleton();

    // BSTEventSink overrides
    RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event, 
                                         RE::BSTEventSource<RE::InputEvent*>* a_eventSource) override;
    RE::BSEventNotifyControl ProcessEvent(const RE::BSAnimationGraphEvent* a_event,
                                         RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource) override;

    // Core functionality
    void ActivateReadyState();
    void OnArrowRelease(); // Call this from your animation event handler
    void LaunchMultishotArrows(RE::PlayerCharacter* player, RE::TESObjectWEAP* weapon, RE::TESAmmo* ammo, int arrowCount, int additionalArrows);
    void UpdateState(); // Check for state transitions (expiration, cooldown end)
    
    // State queries
    bool IsInReadyState() const;
    bool IsOnCooldown() const;
    MultishotState GetCurrentState() const;
    float GetRemainingReadyTime() const;
    float GetRemainingCooldownTime() const;
    
    // Utility methods
    bool CanActivateReadyState();
    bool IsValidBow(RE::TESObjectWEAP* weapon);
    bool HasSufficientAmmo(int requiredCount);
    void ConsumeAmmo(int count);
    void RegisterAnimationEventHandler();
    
private:
    MultishotState currentState = MultishotState::Inactive;
    std::chrono::steady_clock::time_point readyStateStartTime{};
    std::chrono::steady_clock::time_point cooldownStartTime{};
    std::chrono::steady_clock::time_point lastActivationTime{};
    
    MultishotHandler() = default;
    ~MultishotHandler() = default;
    MultishotHandler(const MultishotHandler&) = delete;
    MultishotHandler(MultishotHandler&&) = delete;
    MultishotHandler& operator=(const MultishotHandler&) = delete;
    MultishotHandler& operator=(MultishotHandler&&) = delete;
};