#include "log.h"
#include "settings.h"
#include "combat_classes.h"
#include "hook.h"

void OnDataLoaded()
{
    // Initialize our systems after all game data is loaded
    logger::info("Game data loaded, initializing Combat Classes");
    
    // Load settings
    Settings::GetSingleton()->LoadSettings();
    
    // Initialize the combat classes manager
    CombatClassesManager::GetSingleton()->Initialize();
}

void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
    switch (a_msg->type) {
    case SKSE::MessagingInterface::kDataLoaded:
        OnDataLoaded();
        break;
    case SKSE::MessagingInterface::kPostLoad:
        // Register hooks and event sinks after SKSE plugins are loaded
        RegisterHooks();
        break;
    case SKSE::MessagingInterface::kPreLoadGame:
        break;
    case SKSE::MessagingInterface::kPostLoadGame:
        // Handle post-load game events
        Settings::GetSingleton()->LoadSettings();
        CombatClassesManager::GetSingleton()->Initialize();
        break;
    case SKSE::MessagingInterface::kNewGame:
        // Handle new game
        Settings::GetSingleton()->LoadSettings();
        break;
    }
}

SKSEPluginLoad(const SKSE::LoadInterface *skse) {
    SKSE::Init(skse);
    SetupLog();

    logger::info("CS_CombatClasses v{} loading...", SKSE::PluginDeclaration::GetSingleton()->GetVersion().string());
    
    // Initialize papyrus interface
    auto papyrus = SKSE::GetPapyrusInterface();
    if (!papyrus->Register([](RE::BSScript::IVirtualMachine* /*vm*/) { 
        // You can register any papyrus native functions here if needed
        return true;
    })) {
        logger::error("Failed to register papyrus functions");
        return false;
    }

    auto messaging = SKSE::GetMessagingInterface();
    if (!messaging->RegisterListener("SKSE", MessageHandler)) {
        return false;
    }

    logger::info("CS_CombatClasses loaded successfully");
    return true;
}