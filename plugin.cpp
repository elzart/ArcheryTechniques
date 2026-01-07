#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <thread>

#include "Config.h"
#include "MultishotHandler.h"
#include "PenetratingArrowHandler.h"

using namespace std::literals;


// Animation Event Handler for Arrow Release

class AnimationEventHandler : public RE::BSTEventSink<RE::BSAnimationGraphEvent>
{
public:
    static AnimationEventHandler* GetSingleton()
    {
        static AnimationEventHandler singleton;
        return &singleton;
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::BSAnimationGraphEvent* a_event,
                                         RE::BSTEventSource<RE::BSAnimationGraphEvent>* /*a_eventSource*/) override
    {
        if (!a_event || !a_event->holder) {
            return RE::BSEventNotifyControl::kContinue;
        }

        // Only process events for the player
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player || a_event->holder != player) {
            return RE::BSEventNotifyControl::kContinue;
        }

        // Check for arrow release animation events
        if (a_event->tag == "arrowRelease" || a_event->tag == "bowRelease") {
            SKSE::log::info("Arrow release detected: {}", a_event->tag.c_str());
            MultishotHandler::GetSingleton()->OnArrowRelease();
        }

        return RE::BSEventNotifyControl::kContinue;
    }

private:
    AnimationEventHandler() = default;
    ~AnimationEventHandler() = default;
    AnimationEventHandler(const AnimationEventHandler&) = delete;
    AnimationEventHandler(AnimationEventHandler&&) = delete;
    AnimationEventHandler& operator=(const AnimationEventHandler&) = delete;
    AnimationEventHandler& operator=(AnimationEventHandler&&) = delete;
};

// ============================================
// Plugin Declaration
// ============================================
SKSEPluginInfo(
    .Version = { 1, 0, 0, 0 },
    .Name = "ArcheryTechniques"sv,
    .Author = "Elzar125"sv,
    .SupportEmail = ""sv,
    .StructCompatibility = SKSE::StructCompatibility::Independent,
    .RuntimeCompatibility = SKSE::VersionIndependence::AddressLibrary
)

// ============================================
// Setup Logging
// ============================================
void SetupLog() {
    auto logsFolder = SKSE::log::log_directory();
    if (!logsFolder) SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
    auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
    auto logFilePath = *logsFolder / std::format("{}.log", pluginName);
    auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.string(), true);
    auto loggerPtr = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));
    spdlog::set_default_logger(std::move(loggerPtr));
    spdlog::set_level(spdlog::level::info);
    spdlog::flush_on(spdlog::level::info);
}

// ============================================
// SKSE Message Handler
// ============================================
void OnSKSEMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        SKSE::log::info("Data loaded, registering event handlers...");
        

        auto* config = Config::GetSingleton();
        config->LoadFromINI();
        

        auto* inputDeviceManager = RE::BSInputDeviceManager::GetSingleton();
        if (inputDeviceManager) {
            inputDeviceManager->AddEventSink(MultishotHandler::GetSingleton());
            SKSE::log::info("Multishot input handler registered");
        } else {
            SKSE::log::error("Failed to get BSInputDeviceManager singleton");
        }


        auto* eventSource = RE::BSInputDeviceManager::GetSingleton();
        if (eventSource) {

            eventSource->AddEventSink(PenetratingArrowHandler::GetSingleton());
            SKSE::log::info("Penetrating arrow handler registered for input events");
        }
        
        SKSE::log::info("Penetrating arrow handler initialized independently");


        SKSE::log::info("Plugin initialization complete - animation events will be registered when player loads");
    }
}

// ============================================
// Plugin Entry Point
// ============================================
SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SetupLog();

    auto* plugin = SKSE::PluginDeclaration::GetSingleton();
    auto version = plugin->GetVersion();

    SKSE::log::info("{} {} is loading...", plugin->GetName(), version);
    SKSE::Init(skse);

    // Register for SKSE messages
    auto* messaging = SKSE::GetMessagingInterface();
    if (messaging) {
        messaging->RegisterListener(OnSKSEMessage);
        SKSE::log::info("Registered for SKSE messages");
    } else {
        SKSE::log::error("Failed to get messaging interface");
        return false;
    }

    SKSE::log::info("{} has finished loading.", plugin->GetName());
    return true;
}