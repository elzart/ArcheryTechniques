#pragma once

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <chrono>

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
    void ToggleMultishot();
    void OnArrowRelease(); // Call this from your animation event handler
    void LaunchMultishotArrows(RE::PlayerCharacter* player, RE::TESObjectWEAP* weapon, RE::TESAmmo* ammo, int arrowCount, int additionalArrows);
    bool IsMultishotEnabled() const;
    bool CanToggleMultishot();
    bool IsValidBow(RE::TESObjectWEAP* weapon);
    bool HasSufficientAmmo(int requiredCount);
    void ConsumeAmmo(int count);
    void RegisterAnimationEventHandler();
    
private:
    bool multishotEnabled = false; // Toggle state
    std::chrono::steady_clock::time_point lastToggleTime{};
    
    MultishotHandler() = default;
    ~MultishotHandler() = default;
    MultishotHandler(const MultishotHandler&) = delete;
    MultishotHandler(MultishotHandler&&) = delete;
    MultishotHandler& operator=(const MultishotHandler&) = delete;
    MultishotHandler& operator=(MultishotHandler&&) = delete;
};