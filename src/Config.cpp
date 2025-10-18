#include "Config.h"
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <SimpleIni.h>
#include <filesystem>

Config::Config() {

}

Config* Config::GetSingleton() {
    static Config instance;
    return &instance;
}

void Config::LoadFromINI() {
    CSimpleIniA ini;
    ini.SetUnicode();

    auto configPath = std::filesystem::path("Data") / "SKSE" / "Plugins" / "Multishot.ini";

    if (ini.LoadFile(configPath.string().c_str()) < 0) {
        SKSE::log::info("INI file not found at {}, using default values", configPath.string());
        return;
    }

    // Multishot Settings
    multishot.enabled = ini.GetBoolValue("Multishot", "bEnabled", multishot.enabled);
    multishot.arrowCount = static_cast<int>(ini.GetLongValue("Multishot", "iArrowCount", multishot.arrowCount));
    multishot.spreadAngle = static_cast<float>(ini.GetDoubleValue("Multishot", "fSpreadAngle", multishot.spreadAngle));
    multishot.keyCode = static_cast<int>(ini.GetLongValue("Multishot", "iKeyCode", multishot.keyCode));
    
    // Validate configuration values
    if (multishot.arrowCount < 2) {
        SKSE::log::warn("Arrow count {} is too low, setting to minimum of 2", multishot.arrowCount);
        multishot.arrowCount = 2;
    }
    if (multishot.arrowCount > 10) {
        SKSE::log::warn("Arrow count {} is too high, setting to maximum of 10", multishot.arrowCount);
        multishot.arrowCount = 10;
    }
    if (multishot.spreadAngle < 0.0f) {
        SKSE::log::warn("Spread angle {} is negative, setting to 0", multishot.spreadAngle);
        multishot.spreadAngle = 0.0f;
    }
    if (multishot.spreadAngle > 90.0f) {
        SKSE::log::warn("Spread angle {} is too wide, setting to maximum of 90", multishot.spreadAngle);
        multishot.spreadAngle = 90.0f;
    }
    
    SKSE::log::info("Multishot config loaded - Enabled: {}, Arrow Count: {}, Spread Angle: {}, Key Code: {}", 
                    multishot.enabled, multishot.arrowCount, multishot.spreadAngle, multishot.keyCode);
}

