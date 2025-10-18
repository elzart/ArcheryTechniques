#pragma once

#include <string>

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
};

struct Config {
    MultishotConfig multishot;
    PenetratingArrowConfig penetratingArrow;

    Config();

    static Config* GetSingleton();
    void LoadFromINI();
};

