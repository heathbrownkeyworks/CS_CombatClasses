#pragma once

#include "settings.h"

// Add ActorValue enum if it's not defined
namespace AV {
    enum ActorValue {
        kMarksman = 8,
        kAttackAngleMult = 96,
        kAimOffset_V = 96 + 1,       // Adjust as needed based on your CommonLibSSE
        kAimSightedDelay = 96 + 2,    // Adjust as needed
        kCombatHealthRegenMult = 96 + 3  // Adjust as needed
    };
}

class CombatClassesManager {
private:
    static inline CombatClassesManager* instance = nullptr;
    
    // Track actor state
    struct ActorState {
        bool improvementsApplied = false;
        bool hasSpecialBowBonus = false;
        bool swordKnockbackActive = false;
        float originalMarksman = 0.0f;
        float originalAttackAngleMult = 1.0f;
        float originalAimOffsetV = 1.0f;
        float originalAimSightedDelay = 0.25f;
        float originalCombatHealthRegenMult = 1.0f;
        RE::FormID equippedBowID = 0;
        RE::FormID equippedSwordID = 0;
        std::chrono::steady_clock::time_point lastKnockbackTime;
    };
    
    // Maps actor formIDs to their state
    std::unordered_map<RE::FormID, ActorState> actorStates;
    
    CombatClassesManager() = default;

public:
    static CombatClassesManager* GetSingleton() {
        if (!instance) {
            instance = new CombatClassesManager();
        }
        return instance;
    }
    
    void Initialize() {
        logger::info("Initializing Combat Classes Manager");
        
        // Initialize for all followers that are already loaded
        auto dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            logger::error("Failed to get data handler");
            return;
        }
        
        const auto& settings = Settings::GetSingleton();
        const auto& followers = settings->GetFollowers();
        
        for (const auto& [name, formID] : followers) {
            if (settings->IsFollowerEnabled(formID)) {
                auto actor = RE::TESForm::LookupByID<RE::Actor>(formID);
                if (actor && actor->Is3DLoaded()) {
                    logger::info("Initializing follower: {}", name);
                    ApplyAccuracyImprovements(actor);
                    
                    // Check if they already have equipment
                    auto rightHand = actor->GetEquippedObject(false);
                    if (rightHand) {
                        auto weapon = rightHand->As<RE::TESObjectWEAP>();
                        if (weapon) {
                            HandleWeaponEquipped(actor, weapon);
                        }
                    }
                }
            }
        }
    }
    
    void OnActorEquip(RE::Actor* actor, RE::TESBoundObject* object) {
        if (!actor || !object) return;
        
        auto settings = Settings::GetSingleton();
        if (!settings->IsFollower(actor->GetFormID())) return;
        
        auto weapon = object->As<RE::TESObjectWEAP>();
        if (weapon) {
            HandleWeaponEquipped(actor, weapon);
        }
    }
    
    void OnActorUnequip(RE::Actor* actor, RE::TESBoundObject* object) {
        if (!actor || !object) return;
        
        auto settings = Settings::GetSingleton();
        if (!settings->IsFollower(actor->GetFormID())) return;
        
        auto weapon = object->As<RE::TESObjectWEAP>();
        if (weapon) {
            HandleWeaponUnequipped(actor, weapon);
        }
    }
    
    void OnActorLoad(RE::Actor* actor) {
        if (!actor) return;
        
        auto settings = Settings::GetSingleton();
        if (settings->IsFollower(actor->GetFormID()) && settings->IsFollowerEnabled(actor->GetFormID())) {
            logger::info("Follower loaded: {}", actor->GetName());
            
            if (settings->GetAutoApplyImprovements()) {
                ApplyAccuracyImprovements(actor);
            }
        }
    }
    
    void OnActorUnload(RE::Actor* actor) {
        if (!actor) return;
        
        auto settings = Settings::GetSingleton();
        if (settings->IsFollower(actor->GetFormID())) {
            logger::info("Follower unloaded: {}", actor->GetName());
            
            RemoveAccuracyImprovements(actor);
            RemoveBowBonus(actor);
            RemoveSpecialBowBonus(actor);
            StopSwordKnockback(actor);
            
            // Remove from tracking
            actorStates.erase(actor->GetFormID());
        }
    }
    
    void Update(RE::Actor* actor) {
        if (!actor) return;
        
        auto settings = Settings::GetSingleton();
        if (!settings->IsFollower(actor->GetFormID()) || !settings->IsFollowerEnabled(actor->GetFormID())) {
            return;
        }
        
        auto actorID = actor->GetFormID();
        auto it = actorStates.find(actorID);
        if (it == actorStates.end()) {
            return;
        }
        
        auto& state = it->second;
        
        // Handle sword knockback
        if (state.swordKnockbackActive && state.equippedSwordID != 0) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - state.lastKnockbackTime).count();
            
            if (elapsed >= settings->GetKnockbackInterval()) {
                // Time to do knockback
                HandleSwordKnockback(actor);
                state.lastKnockbackTime = now;
            }
        }
    }
    
private:
    void HandleWeaponEquipped(RE::Actor* actor, RE::TESObjectWEAP* weapon) {
        auto settings = Settings::GetSingleton();
        auto weaponID = weapon->GetFormID();
        auto actorID = actor->GetFormID();
        
        // Get or create actor state
        auto& state = actorStates[actorID];
        
        // Check weapon type
        if (weapon->GetWeaponType() == RE::WEAPON_TYPE::kBow) {
            // Apply bow bonus
            ApplyBowBonus(actor);
            state.equippedBowID = weaponID;
            
            // Check if it's a special bow
            if (settings->IsSpecialBow(weaponID)) {
                ApplySpecialBowBonus(actor);
                
                // Notify player if the follower is player's follower
                if (actor->IsPlayerTeammate()) {
                    auto name = weapon->GetName();
                    RE::DebugNotification(fmt::format("{}'s Improved Aim Activated", name).c_str());
                }
            }
        } else if (settings->IsSpecialSword(weaponID)) {
            // It's a special sword, start the knockback effect
            StartSwordKnockback(actor);
            state.equippedSwordID = weaponID;
            
            // Notify player if the follower is player's follower
            if (actor->IsPlayerTeammate()) {
                auto name = weapon->GetName();
                RE::DebugNotification(fmt::format("{}'s Knockback Power Activated", name).c_str());
            }
        }
    }
    
    void HandleWeaponUnequipped(RE::Actor* actor, RE::TESObjectWEAP* weapon) {
        auto settings = Settings::GetSingleton();
        auto weaponID = weapon->GetFormID();
        auto actorID = actor->GetFormID();
        
        auto it = actorStates.find(actorID);
        if (it == actorStates.end()) {
            return;
        }
        
        auto& state = it->second;
        
        // Check weapon type
        if (weapon->GetWeaponType() == RE::WEAPON_TYPE::kBow) {
            // Remove bow bonuses
            RemoveBowBonus(actor);
            
            // Check if it's the special bow
            if (settings->IsSpecialBow(weaponID)) {
                RemoveSpecialBowBonus(actor);
            }
            
            state.equippedBowID = 0;
        } else if (settings->IsSpecialSword(weaponID)) {
            // It's the special sword, stop the knockback effect
            StopSwordKnockback(actor);
            state.equippedSwordID = 0;
        }
    }
    
    void ApplyAccuracyImprovements(RE::Actor* actor) {
        if (!actor) return;
        
        auto actorID = actor->GetFormID();
        auto it = actorStates.find(actorID);
        
        // If already applied, return
        if (it != actorStates.end() && it->second.improvementsApplied) {
            return;
        }
        
        // Get settings
        auto settings = Settings::GetSingleton();
        
        // Create state for this actor if it doesn't exist
        auto& state = actorStates[actorID];
        
        // Store original values
        state.originalMarksman = actor->GetActorValueByName("Marksman");
        state.originalAttackAngleMult = actor->GetActorValueByName("attackAngleMult");
        state.originalAimOffsetV = actor->GetActorValueByName("aimOffsetV");
        state.originalAimSightedDelay = actor->GetActorValueByName("aimSightedDelay");
        state.originalCombatHealthRegenMult = actor->GetActorValueByName("combatHealthRegenMult");
        
        // Apply improvements
        if (actor->GetActorValueByName("Marksman") < state.originalMarksman + settings->GetBaseAccuracyBonus()) {
            actor->SetActorValueByName("Marksman", state.originalMarksman + settings->GetBaseAccuracyBonus());
        }
        
        actor->SetActorValueByName("attackAngleMult", settings->GetAttackAngleMult());
        actor->SetActorValueByName("aimOffsetV", settings->GetAimOffsetV());
        actor->SetActorValueByName("aimSightedDelay", settings->GetAimSightedDelay());
        actor->SetActorValueByName("combatHealthRegenMult", 2.0f);
        
        state.improvementsApplied = true;
        
        // Notify player if the follower is player's follower
        if (actor->IsPlayerTeammate()) {
            RE::DebugNotification(fmt::format("{}'s Accuracy Improvements Applied", actor->GetName()).c_str());
        }
        
        logger::info("Applied accuracy improvements to {}", actor->GetName());
    }
    
    void RemoveAccuracyImprovements(RE::Actor* actor) {
        if (!actor) return;
        
        auto actorID = actor->GetFormID();
        auto it = actorStates.find(actorID);
        
        // If not applied, return
        if (it == actorStates.end() || !it->second.improvementsApplied) {
            return;
        }
        
        auto& state = it->second;
        
        // Restore original values
        actor->SetActorValueByName("Marksman", state.originalMarksman);
        actor->SetActorValueByName("attackAngleMult", state.originalAttackAngleMult);
        actor->SetActorValueByName("aimOffsetV", state.originalAimOffsetV);
        actor->SetActorValueByName("aimSightedDelay", state.originalAimSightedDelay);
        actor->SetActorValueByName("combatHealthRegenMult", state.originalCombatHealthRegenMult);
        
        state.improvementsApplied = false;
        
        logger::info("Removed accuracy improvements from {}", actor->GetName());
    }
    
    void ApplyBowBonus(RE::Actor* actor) {
        if (!actor) return;
        
        auto settings = Settings::GetSingleton();
        actor->ModActorValueByName("Marksman", settings->GetBowAccuracyBonus());
        actor->SetActorValueByName("attackAngleMult", settings->GetAttackAngleMult() * 0.8f);
        
        logger::info("Applied bow bonus to {}", actor->GetName());
    }
    
    void RemoveBowBonus(RE::Actor* actor) {
        if (!actor) return;
        
        auto settings = Settings::GetSingleton();
        actor->ModActorValueByName("Marksman", -settings->GetBowAccuracyBonus());
        actor->SetActorValueByName("attackAngleMult", settings->GetAttackAngleMult());
        
        logger::info("Removed bow bonus from {}", actor->GetName());
    }
    
    void ApplySpecialBowBonus(RE::Actor* actor) {
        if (!actor) return;
        
        auto actorID = actor->GetFormID();
        auto it = actorStates.find(actorID);
        
        // If already applied, return
        if (it != actorStates.end() && it->second.hasSpecialBowBonus) {
            return;
        }
        
        auto settings = Settings::GetSingleton();
        actor->ModActorValueByName("Marksman", settings->GetSpecialBowBonus());
        actor->SetActorValueByName("attackAngleMult", settings->GetAttackAngleMult() * 0.6f);
        
        // Update state
        auto& state = actorStates[actorID];
        state.hasSpecialBowBonus = true;
        
        logger::info("Applied special bow bonus to {}", actor->GetName());
    }
    
    void RemoveSpecialBowBonus(RE::Actor* actor) {
        if (!actor) return;
        
        auto actorID = actor->GetFormID();
        auto it = actorStates.find(actorID);
        
        // If not applied, return
        if (it == actorStates.end() || !it->second.hasSpecialBowBonus) {
            return;
        }
        
        auto settings = Settings::GetSingleton();
        actor->ModActorValueByName("Marksman", -settings->GetSpecialBowBonus());
        actor->SetActorValueByName("attackAngleMult", settings->GetAttackAngleMult() * 0.8f);
        
        // Update state
        it->second.hasSpecialBowBonus = false;
        
        logger::info("Removed special bow bonus from {}", actor->GetName());
    }
    
    void StartSwordKnockback(RE::Actor* actor) {
        if (!actor) return;
        
        auto actorID = actor->GetFormID();
        auto& state = actorStates[actorID];
        
        // If already active, return
        if (state.swordKnockbackActive) {
            return;
        }
        
        state.swordKnockbackActive = true;
        state.lastKnockbackTime = std::chrono::steady_clock::now();
        
        logger::info("Started sword knockback for {}", actor->GetName());
    }
    
    void StopSwordKnockback(RE::Actor* actor) {
        if (!actor) return;
        
        auto actorID = actor->GetFormID();
        auto it = actorStates.find(actorID);
        
        if (it != actorStates.end()) {
            it->second.swordKnockbackActive = false;
            logger::info("Stopped sword knockback for {}", actor->GetName());
        }
    }
    
    void HandleSwordKnockback(RE::Actor* actor) {
        if (!actor) return;
        
        auto settings = Settings::GetSingleton();
        
        // Find nearest enemy
        auto nearestEnemy = GetNearestEnemy(actor);
        if (nearestEnemy && !nearestEnemy->IsDead() && nearestEnemy->IsHostileToActor(actor)) {
            // Apply knockback effect - using Skyrim's native PushActorAway function
            // We need to access it through the game's script system
            RE::TESForm* gameObj = RE::TESForm::LookupByID(0x14);  // Game's formID
            if (gameObj) {
                auto vm = SKSE::GetPapyrusInterface()->GetVirtualMachine();
                if (vm) {
                    // Create argument list
                    RE::TESObjectREFR* source = actor;
                    RE::TESObjectREFR* target = nearestEnemy;
                    float magnitude = settings->GetKnockbackMagnitude();
                    
                    RE::BSFixedString funcName("PushActorAway");
                    RE::BSFixedString objName("Game");
                    
                    // Call the function
                    std::vector<RE::BSScript::Variable> args;
                    
                    RE::BSScript::Variable targetVar;
                    targetVar.SetObject(target);
                    args.push_back(targetVar);
                    
                    RE::BSScript::Variable sourceVar;
                    sourceVar.SetObject(source);
                    args.push_back(sourceVar);
                    
                    RE::BSScript::Variable magVar;
                    magVar.SetFloat(magnitude);
                    args.push_back(magVar);
                    
                    auto nullHandle = RE::BSScript::IObjectHandlePolicy::InvalidHandle;
                    
                    vm->SendEvent(nullHandle, objName.c_str(), funcName.c_str(), args);
                }
            }
            
            // Notify player if the follower is player's teammate
            if (actor->IsPlayerTeammate()) {
                auto weapon = actor->GetEquippedObject(false);
                if (weapon) {
                    auto name = weapon->GetName();
                    RE::DebugNotification(fmt::format("{} unleashes a powerful knockback!", name).c_str());
                }
            }
            
            logger::info("{} performed knockback on {}", actor->GetName(), nearestEnemy->GetName());
        }
    }
    
    RE::Actor* GetNearestEnemy(RE::Actor* actor) {
        if (!actor) return nullptr;
        
        std::vector<RE::Actor*> combatTargets;
        
        // Add player's combat target if relevant
        auto player = RE::PlayerCharacter::GetSingleton();
        if (player) {
            auto target = player->GetActorRuntimeData().currentCombatTarget.get();
            if (target && !target->IsDead() && target->IsHostileToActor(actor)) {
                combatTargets.push_back(target.get());
            }
        }
        
        // Add actor's combat target
        auto targetPtr = actor->GetActorRuntimeData().currentCombatTarget.get();
        if (targetPtr && !targetPtr->IsDead()) {
            // Check if already in the list
            bool found = false;
            for (auto target : combatTargets) {
                if (target == targetPtr.get()) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                combatTargets.push_back(targetPtr.get());
            }
        }
        
        // Find nearest from the targets
        RE::Actor* nearestActor = nullptr;
        float nearestDistance = 99999.0f;
        
        for (auto target : combatTargets) {
            if (target) {
                float currentDistance = actor->GetPosition().GetDistance(target->GetPosition());
                if (currentDistance < nearestDistance) {
                    nearestDistance = currentDistance;
                    nearestActor = target;
                }
            }
        }
        
        return nearestActor;
    }
};