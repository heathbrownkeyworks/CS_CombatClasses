#pragma once

#include "combat_classes.h"
#include <chrono>

using namespace std::chrono_literals;

class EquipEventHandler : public RE::BSTEventSink<RE::TESEquipEvent> {
private:
    static inline EquipEventHandler* instance = nullptr;
    
    EquipEventHandler() = default;

public:
    static EquipEventHandler* GetSingleton() {
        if (!instance) {
            instance = new EquipEventHandler();
        }
        return instance;
    }
    
    RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* event, RE::BSTEventSource<RE::TESEquipEvent>*) override {
        if (!event) {
            return RE::BSEventNotifyControl::kContinue;
        }
        
        // In some versions of CommonLibSSE, the actor is stored directly in the event
        // In others, it's referenced by FormID. Check your TESEquipEvent.h for structure.
        RE::Actor* actor = RE::TESForm::LookupByID<RE::Actor>(event->actor);
        if (!actor) {
            return RE::BSEventNotifyControl::kContinue;
        }
        
        RE::TESBoundObject* object = RE::TESForm::LookupByID<RE::TESBoundObject>(event->baseObject);
        if (!object) {
            return RE::BSEventNotifyControl::kContinue;
        }
        
        if (event->equipped) {
            // Equip event
            CombatClassesManager::GetSingleton()->OnActorEquip(actor, object);
        } else {
            // Unequip event
            CombatClassesManager::GetSingleton()->OnActorUnequip(actor, object);
        }
        
        return RE::BSEventNotifyControl::kContinue;
    }
    
    void Register() {
        RE::ScriptEventSourceHolder* eventHolder = RE::ScriptEventSourceHolder::GetSingleton();
        if (eventHolder) {
            eventHolder->AddEventSink<RE::TESEquipEvent>(this);
            logger::info("Registered equip event handler");
        }
    }
};

class LoadGameEventHandler : public RE::BSTEventSink<RE::TESLoadGameEvent> {
private:
    static inline LoadGameEventHandler* instance = nullptr;
    
    LoadGameEventHandler() = default;

public:
    static LoadGameEventHandler* GetSingleton() {
        if (!instance) {
            instance = new LoadGameEventHandler();
        }
        return instance;
    }
    
    RE::BSEventNotifyControl ProcessEvent(const RE::TESLoadGameEvent*, RE::BSTEventSource<RE::TESLoadGameEvent>*) override {
        logger::info("Game loaded, initializing Combat Classes Manager");
        
        // Load settings
        Settings::GetSingleton()->LoadSettings();
        
        // Initialize manager
        CombatClassesManager::GetSingleton()->Initialize();
        
        return RE::BSEventNotifyControl::kContinue;
    }
    
    void Register() {
        RE::ScriptEventSourceHolder* eventHolder = RE::ScriptEventSourceHolder::GetSingleton();
        if (eventHolder) {
            eventHolder->AddEventSink<RE::TESLoadGameEvent>(this);
            logger::info("Registered load game event handler");
        }
    }
};

// SKSE Task that is called periodically
class PeriodicUpdateTask {
private:
    static inline PeriodicUpdateTask* instance = nullptr;
    
    PeriodicUpdateTask() = default;
    
    // Handler for registered actors
    std::unordered_set<RE::FormID> registeredActors;

public:
    static PeriodicUpdateTask* GetSingleton() {
        if (!instance) {
            instance = new PeriodicUpdateTask();
        }
        return instance;
    }

    static void Register() {
        auto task = GetSingleton();
        
        const auto& followers = Settings::GetSingleton()->GetFollowers();
        for (const auto& [name, formID] : followers) {
            if (Settings::GetSingleton()->IsFollowerEnabled(formID)) {
                task->RegisterActor(formID);
            }
        }
        
        // Schedule the first update
        SKSE::GetTaskInterface()->AddTask([task]() {
            task->ProcessAll();
        });
        
        logger::info("Registered periodic update task");
    }
    
    void RegisterActor(RE::FormID formID) {
        registeredActors.insert(formID);
    }
    
    void UnregisterActor(RE::FormID formID) {
        registeredActors.erase(formID);
    }
    
    void ProcessAll() {
        for (auto formID : registeredActors) {
            auto actor = RE::TESForm::LookupByID<RE::Actor>(formID);
            if (actor && actor->Is3DLoaded()) {
                CombatClassesManager::GetSingleton()->Update(actor);
            }
        }
        
        // Schedule the next update
        auto taskInterface = SKSE::GetTaskInterface();
        if (taskInterface) {
            taskInterface->AddTask([this]() {
                this->ProcessAll();
            });
        }
    }
};

class FormDeleteEventHandler : public RE::BSTEventSink<RE::TESFormDeleteEvent> {
private:
    static inline FormDeleteEventHandler* instance = nullptr;
    
    FormDeleteEventHandler() = default;

public:
    static FormDeleteEventHandler* GetSingleton() {
        if (!instance) {
            instance = new FormDeleteEventHandler();
        }
        return instance;
    }
    
    RE::BSEventNotifyControl ProcessEvent(const RE::TESFormDeleteEvent* event, RE::BSTEventSource<RE::TESFormDeleteEvent>*) override {
        if (!event) {
            return RE::BSEventNotifyControl::kContinue;
        }
        
        auto actor = RE::TESForm::LookupByID<RE::Actor>(event->formID);
        if (actor) {
            CombatClassesManager::GetSingleton()->OnActorUnload(actor);
            PeriodicUpdateTask::GetSingleton()->UnregisterActor(event->formID);
        }
        
        return RE::BSEventNotifyControl::kContinue;
    }
    
    void Register() {
        RE::ScriptEventSourceHolder* eventHolder = RE::ScriptEventSourceHolder::GetSingleton();
        if (eventHolder) {
            eventHolder->AddEventSink<RE::TESFormDeleteEvent>(this);
            logger::info("Registered form delete event handler");
        }
    }
};

class CellLoadEventHandler : public RE::BSTEventSink<RE::TESCellFullyLoadedEvent> {
private:
    static inline CellLoadEventHandler* instance = nullptr;
    
    CellLoadEventHandler() = default;

public:
    static CellLoadEventHandler* GetSingleton() {
        if (!instance) {
            instance = new CellLoadEventHandler();
        }
        return instance;
    }
    
    RE::BSEventNotifyControl ProcessEvent(const RE::TESCellFullyLoadedEvent* event, RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*) override {
        if (!event) {
            return RE::BSEventNotifyControl::kContinue;
        }
        
        auto cell = event->cell;
        if (!cell) {
            return RE::BSEventNotifyControl::kContinue;
        }
        
        // Process actors in this cell
        const auto& refList = cell->GetRuntimeData().references;
        for (auto& ref : refList) {
            if (ref && ref.get()) {
                auto actor = ref->As<RE::Actor>();
                if (actor) {
                    CombatClassesManager::GetSingleton()->OnActorLoad(actor);
                    
                    // Register for periodic updates if this is a follower
                    if (Settings::GetSingleton()->IsFollower(actor->GetFormID())) {
                        PeriodicUpdateTask::GetSingleton()->RegisterActor(actor->GetFormID());
                    }
                }
            }
        }
        
        return RE::BSEventNotifyControl::kContinue;
    }
    
    void Register() {
        RE::ScriptEventSourceHolder* eventHolder = RE::ScriptEventSourceHolder::GetSingleton();
        if (eventHolder) {
            eventHolder->AddEventSink<RE::TESCellFullyLoadedEvent>(this);
            logger::info("Registered cell load event handler");
        }
    }
};

void RegisterHooks() {
    // Register event handlers
    EquipEventHandler::GetSingleton()->Register();
    LoadGameEventHandler::GetSingleton()->Register();
    FormDeleteEventHandler::GetSingleton()->Register();
    CellLoadEventHandler::GetSingleton()->Register();
    
    // Set up periodic updates
    PeriodicUpdateTask::Register();
    
    logger::info("All hooks registered");
}