#pragma once

#include <RE/Skyrim.h>

// ============================================
// Configuration -
// ============================================
struct MultishotConfig {
    bool enabled = true;
    int arrowCount = 3;
    float spreadAngle = 15.0f;
    int keyCode = 46; // 'C' key scan code
    float readyWindowDuration = 5.0f; // Duration of ready state in seconds
    float cooldownDuration = 20.0f; // Cooldown period in seconds
};

struct PenetratingArrowConfig {
    bool enabled = true;
    float chargeTime = 3.0f; // Time to charge penetrating arrow in seconds
    float cooldownDuration = 10.0f; // Cooldown period in seconds
    float damageMultiplier = 2.0f; // Damage multiplier for penetrating arrows
    float speedMultiplier = 1.5f; // Speed multiplier for penetrating arrows
};

struct Config {
    MultishotConfig multishot;
    PenetratingArrowConfig penetratingArrow;
    bool enablePerks = false; // Global setting to enable perk requirements

    Config();

    static Config* GetSingleton();
    void LoadFromINI();
    
    // Helper functions for perk checking
    static bool HasMultishotPerk();
    static bool HasPenetratePerk();
};

