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

    auto configPath = std::filesystem::path("Data") / "SKSE" / "Plugins" / "ArcheryTechniques.ini";

    if (ini.LoadFile(configPath.string().c_str()) < 0) {
        SKSE::log::info("INI file not found at {}, using default values", configPath.string());
        return;
    }

    // General Settings
    enablePerks = ini.GetBoolValue("General", "bEnablePerks", enablePerks);
    
    // Multishot Settings
    multishot.enabled = ini.GetBoolValue("Multishot", "bEnabled", multishot.enabled);
    multishot.arrowCount = static_cast<int>(ini.GetLongValue("Multishot", "iArrowCount", multishot.arrowCount));
    multishot.spreadAngle = static_cast<float>(ini.GetDoubleValue("Multishot", "fSpreadAngle", multishot.spreadAngle));
    multishot.keyCode = static_cast<int>(ini.GetLongValue("Multishot", "iKeyCode", multishot.keyCode));
    multishot.readyWindowDuration = static_cast<float>(ini.GetDoubleValue("Multishot", "fReadyWindowDuration", multishot.readyWindowDuration));
    multishot.cooldownDuration = static_cast<float>(ini.GetDoubleValue("Multishot", "fCooldownDuration", multishot.cooldownDuration));
    
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
    if (multishot.readyWindowDuration < 1.0f) {
        SKSE::log::warn("Ready window duration {} is too short, setting to minimum of 1 second", multishot.readyWindowDuration);
        multishot.readyWindowDuration = 1.0f;
    }
    if (multishot.readyWindowDuration > 30.0f) {
        SKSE::log::warn("Ready window duration {} is too long, setting to maximum of 30 seconds", multishot.readyWindowDuration);
        multishot.readyWindowDuration = 30.0f;
    }
    if (multishot.cooldownDuration < 5.0f) {
        SKSE::log::warn("Cooldown duration {} is too short, setting to minimum of 5 seconds", multishot.cooldownDuration);
        multishot.cooldownDuration = 5.0f;
    }
    if (multishot.cooldownDuration > 300.0f) {
        SKSE::log::warn("Cooldown duration {} is too long, setting to maximum of 300 seconds", multishot.cooldownDuration);
        multishot.cooldownDuration = 300.0f;
    }
    
    // Penetrating Arrow Settings
    penetratingArrow.enabled = ini.GetBoolValue("PenetratingArrow", "bEnabled", penetratingArrow.enabled);
    penetratingArrow.chargeTime = static_cast<float>(ini.GetDoubleValue("PenetratingArrow", "fChargeTime", penetratingArrow.chargeTime));
    penetratingArrow.cooldownDuration = static_cast<float>(ini.GetDoubleValue("PenetratingArrow", "fCooldownDuration", penetratingArrow.cooldownDuration));
    penetratingArrow.damageMultiplier = static_cast<float>(ini.GetDoubleValue("PenetratingArrow", "fDamageMultiplier", penetratingArrow.damageMultiplier));
    penetratingArrow.speedMultiplier = static_cast<float>(ini.GetDoubleValue("PenetratingArrow", "fSpeedMultiplier", penetratingArrow.speedMultiplier));
    
    // Validate penetrating arrow configuration values
    if (penetratingArrow.chargeTime < 1.0f) {
        SKSE::log::warn("Penetrating arrow charge time {} is too short, setting to minimum of 1 second", penetratingArrow.chargeTime);
        penetratingArrow.chargeTime = 1.0f;
    }
    if (penetratingArrow.chargeTime > 10.0f) {
        SKSE::log::warn("Penetrating arrow charge time {} is too long, setting to maximum of 10 seconds", penetratingArrow.chargeTime);
        penetratingArrow.chargeTime = 10.0f;
    }
    // Stamina cost validation removed
    if (penetratingArrow.cooldownDuration < 0.0f) {
        SKSE::log::warn("Penetrating arrow cooldown duration {} is negative, setting to 0", penetratingArrow.cooldownDuration);
        penetratingArrow.cooldownDuration = 0.0f;
    }
    if (penetratingArrow.cooldownDuration > 300.0f) {
        SKSE::log::warn("Penetrating arrow cooldown duration {} is too long, setting to maximum of 300 seconds", penetratingArrow.cooldownDuration);
        penetratingArrow.cooldownDuration = 300.0f;
    }
    
    SKSE::log::info("General config loaded - Enable Perks: {}", enablePerks);
    
    SKSE::log::info("Multishot config loaded - Enabled: {}, Arrow Count: {}, Spread Angle: {}, Key Code: {}, Ready Window: {}s, Cooldown: {}s", 
                    multishot.enabled, multishot.arrowCount, multishot.spreadAngle, multishot.keyCode, 
                    multishot.readyWindowDuration, multishot.cooldownDuration);
    
    SKSE::log::info("Penetrating Arrow config loaded - Enabled: {}, Charge Time: {}s, Cooldown: {}s", 
                    penetratingArrow.enabled, penetratingArrow.chargeTime, penetratingArrow.cooldownDuration);
}

bool Config::HasMultishotPerk() {
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        SKSE::log::warn("Player not available for perk check");
        return false;
    }
    
    // Look up perk by editor ID
    auto* form = RE::TESForm::LookupByEditorID("ArcheryTechniquesMultishot");
    if (!form) {
        SKSE::log::warn("Could not find multishot perk (ArcheryTechniquesMultishot)");
        return false;
    }
    
    auto* perk = form->As<RE::BGSPerk>();
    if (!perk) {
        SKSE::log::warn("ArcheryTechniquesMultishot is not a perk");
        return false;
    }
    
    bool hasPerk = player->HasPerk(perk);
    SKSE::log::debug("Player {} multishot perk", hasPerk ? "has" : "does not have");
    return hasPerk;
}

bool Config::HasPenetratePerk() {
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        SKSE::log::warn("Player not available for perk check");
        return false;
    }
    
    // Look up perk by editor ID
    auto* form = RE::TESForm::LookupByEditorID("ArcheryTechniquesPenetratingArrow");
    if (!form) {
        SKSE::log::warn("Could not find penetrate perk (ArcheryTechniquesPenetratingArrow)");
        return false;
    }
    
    auto* perk = form->As<RE::BGSPerk>();
    if (!perk) {
        SKSE::log::warn("ArcheryTechniquesPenetratingArrow is not a perk");
        return false;
    }
    
    bool hasPerk = player->HasPerk(perk);
    SKSE::log::debug("Player {} penetrate perk", hasPerk ? "has" : "does not have");
    return hasPerk;
}



