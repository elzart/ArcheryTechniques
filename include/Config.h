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
};

struct Config {
    MultishotConfig multishot;

    Config();

    static Config* GetSingleton();
    void LoadFromINI();
};

