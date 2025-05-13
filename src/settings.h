#pragma once

#include "SimpleIni.h"

namespace logger = SKSE::log;

class Settings {
private:
    static inline Settings* instance = nullptr;
    CSimpleIniA ini;
    std::unordered_map<std::string, bool> followersEnabled;
    
    // Default settings
    float baseAccuracyBonus = 30.0f;
    float attackAngleMult = 0.5f;
    float aimOffsetV = 0.85f;
    float aimSightedDelay = 0.1f;
    bool autoApplyImprovements = true;
    float bowAccuracyBonus = 20.0f;
    float specialBowBonus = 15.0f;
    float knockbackMagnitude = 1000.0f;
    float knockbackInterval = 10.0f;
    
    // Weapon FormIDs
    std::unordered_map<std::string, RE::FormID> specialBows;
    std::unordered_map<std::string, RE::FormID> specialSwords;

    // Actor FormIDs - name to formID mapping for followers
    std::unordered_map<std::string, RE::FormID> followers;

    Settings() = default;

public:
    static Settings* GetSingleton() {
        if (!instance) {
            instance = new Settings();
        }
        return instance;
    }

    bool LoadSettings() {
        ini.SetUnicode();
        
        // Get the path to the settings file
        auto configDir = SKSE::log::log_directory();
        if (!configDir) {
            logger::error("Failed to get config directory");
            return false;
        }

        auto configPath = *configDir / "CS_CombatClasses/Settings.ini";
        
        // Load the settings file
        SI_Error rc = ini.LoadFile(configPath.string().c_str());
        if (rc < 0) {
            // If file doesn't exist, create it with default values
            logger::info("Settings file not found, creating with defaults");
            SaveSettings();
            return true;
        }

        // Load general settings
        baseAccuracyBonus = static_cast<float>(ini.GetDoubleValue("General", "fBaseAccuracyBonus", 30.0));
        attackAngleMult = static_cast<float>(ini.GetDoubleValue("General", "fAttackAngleMult", 0.5));
        aimOffsetV = static_cast<float>(ini.GetDoubleValue("General", "fAimOffsetV", 0.85));
        aimSightedDelay = static_cast<float>(ini.GetDoubleValue("General", "fAimSightedDelay", 0.1));
        autoApplyImprovements = ini.GetBoolValue("General", "bAutoApplyImprovements", true);
        bowAccuracyBonus = static_cast<float>(ini.GetDoubleValue("General", "fBowAccuracyBonus", 20.0));
        specialBowBonus = static_cast<float>(ini.GetDoubleValue("General", "fSpecialBowBonus", 15.0));
        knockbackMagnitude = static_cast<float>(ini.GetDoubleValue("General", "fKnockbackMagnitude", 1000.0));
        knockbackInterval = static_cast<float>(ini.GetDoubleValue("General", "fKnockbackInterval", 10.0));

        // Load follower settings
        CSimpleIniA::TNamesDepend sections;
        ini.GetAllSections(sections);
        for (const auto& section : sections) {
            std::string sectionName = section.pItem;
            if (sectionName.find("Follower:") == 0) {
                std::string followerName = sectionName.substr(9); // Remove "Follower:" prefix
                std::string formIDStr = ini.GetValue(sectionName.c_str(), "FormID", "");
                std::string pluginName = ini.GetValue(sectionName.c_str(), "Plugin", "Skyrim.esm");
                std::string enabledStr = ini.GetValue(sectionName.c_str(), "Enabled", "true");
                
                if (!formIDStr.empty()) {
                    try {
                        uint32_t formID = std::stoul(formIDStr, nullptr, 16);
                        auto* dataHandler = RE::TESDataHandler::GetSingleton();
                        auto* form = dataHandler->LookupForm(formID, pluginName);
                        if (form) {
                            followers[followerName] = form->GetFormID();
                            followersEnabled[followerName] = (enabledStr == "true");
                        }
                    } catch (const std::exception& e) {
                        logger::error("Error parsing follower FormID: {}", e.what());
                    }
                }
            } else if (sectionName.find("SpecialBow:") == 0) {
                std::string bowName = sectionName.substr(11); // Remove "SpecialBow:" prefix
                std::string formIDStr = ini.GetValue(sectionName.c_str(), "FormID", "");
                std::string pluginName = ini.GetValue(sectionName.c_str(), "Plugin", "Skyrim.esm");
                
                if (!formIDStr.empty()) {
                    try {
                        uint32_t formID = std::stoul(formIDStr, nullptr, 16);
                        auto* dataHandler = RE::TESDataHandler::GetSingleton();
                        auto* form = dataHandler->LookupForm(formID, pluginName);
                        if (form) {
                            specialBows[bowName] = form->GetFormID();
                        }
                    } catch (const std::exception& e) {
                        logger::error("Error parsing special bow FormID: {}", e.what());
                    }
                }
            } else if (sectionName.find("SpecialSword:") == 0) {
                std::string swordName = sectionName.substr(13); // Remove "SpecialSword:" prefix
                std::string formIDStr = ini.GetValue(sectionName.c_str(), "FormID", "");
                std::string pluginName = ini.GetValue(sectionName.c_str(), "Plugin", "Skyrim.esm");
                
                if (!formIDStr.empty()) {
                    try {
                        uint32_t formID = std::stoul(formIDStr, nullptr, 16);
                        auto* dataHandler = RE::TESDataHandler::GetSingleton();
                        auto* form = dataHandler->LookupForm(formID, pluginName);
                        if (form) {
                            specialSwords[swordName] = form->GetFormID();
                        }
                    } catch (const std::exception& e) {
                        logger::error("Error parsing special sword FormID: {}", e.what());
                    }
                }
            }
        }

        logger::info("Settings loaded successfully");
        return true;
    }

    bool SaveSettings() {
        // Set general settings
        ini.SetDoubleValue("General", "fBaseAccuracyBonus", baseAccuracyBonus);
        ini.SetDoubleValue("General", "fAttackAngleMult", attackAngleMult);
        ini.SetDoubleValue("General", "fAimOffsetV", aimOffsetV);
        ini.SetDoubleValue("General", "fAimSightedDelay", aimSightedDelay);
        ini.SetBoolValue("General", "bAutoApplyImprovements", autoApplyImprovements);
        ini.SetDoubleValue("General", "fBowAccuracyBonus", bowAccuracyBonus);
        ini.SetDoubleValue("General", "fSpecialBowBonus", specialBowBonus);
        ini.SetDoubleValue("General", "fKnockbackMagnitude", knockbackMagnitude);
        ini.SetDoubleValue("General", "fKnockbackInterval", knockbackInterval);

        // Add a default follower (Samandriel) if none exist
        if (followers.empty()) {
            std::string sectionName = "Follower:Samandriel";
            ini.SetValue(sectionName.c_str(), "FormID", "14000", "FormID in hexadecimal");
            ini.SetValue(sectionName.c_str(), "Plugin", "YourMod.esp", "Plugin name");
            ini.SetValue(sectionName.c_str(), "Enabled", "true", "Enable/disable this follower");
        } else {
            // Save follower settings
            for (const auto& [name, formID] : followers) {
                std::string sectionName = "Follower:" + name;
                
                auto* form = RE::TESForm::LookupByID(formID);
                if (form) {
                    const auto* file = form->GetFile(0);
                    std::string modName = file ? file->GetFilename() : std::string("Skyrim.esm");
                    
                    std::stringstream ss;
                    ss << std::hex << std::uppercase << (formID & 0xFFFFFF);
                    
                    ini.SetValue(sectionName.c_str(), "FormID", ss.str().c_str());
                    ini.SetValue(sectionName.c_str(), "Plugin", modName.c_str());
                    ini.SetValue(sectionName.c_str(), "Enabled", followersEnabled[name] ? "true" : "false");
                }
            }
        }

        // Add default special weapons if none exist
        if (specialBows.empty()) {
            std::string sectionName = "SpecialBow:Truthseeker";
            ini.SetValue(sectionName.c_str(), "FormID", "14001", "FormID in hexadecimal");
            ini.SetValue(sectionName.c_str(), "Plugin", "YourMod.esp", "Plugin name");
        } else {
            // Save special bow settings
            for (const auto& [name, formID] : specialBows) {
                std::string sectionName = "SpecialBow:" + name;
                
                auto* form = RE::TESForm::LookupByID(formID);
                if (form) {
                    const auto* file = form->GetFile(0);
                    std::string modName = file ? file->GetFilename() : std::string("Skyrim.esm");
                    
                    std::stringstream ss;
                    ss << std::hex << std::uppercase << (formID & 0xFFFFFF);
                    
                    ini.SetValue(sectionName.c_str(), "FormID", ss.str().c_str());
                    ini.SetValue(sectionName.c_str(), "Plugin", modName.c_str());
                }
            }
        }

        if (specialSwords.empty()) {
            std::string sectionName = "SpecialSword:Sevenfold";
            ini.SetValue(sectionName.c_str(), "FormID", "14002", "FormID in hexadecimal");
            ini.SetValue(sectionName.c_str(), "Plugin", "YourMod.esp", "Plugin name");
        } else {
            // Save special sword settings
            for (const auto& [name, formID] : specialSwords) {
                std::string sectionName = "SpecialSword:" + name;
                
                auto* form = RE::TESForm::LookupByID(formID);
                if (form) {
                    const auto* file = form->GetFile(0);
                    std::string modName = file ? file->GetFilename() : std::string("Skyrim.esm");
                    
                    std::stringstream ss;
                    ss << std::hex << std::uppercase << (formID & 0xFFFFFF);
                    
                    ini.SetValue(sectionName.c_str(), "FormID", ss.str().c_str());
                    ini.SetValue(sectionName.c_str(), "Plugin", modName.c_str());
                }
            }
        }

        // Get the path to the settings file
        auto configDir = SKSE::log::log_directory();
        if (!configDir) {
            logger::error("Failed to get config directory");
            return false;
        }

        auto configPath = *configDir / "CS_CombatClasses/Settings.ini";
        
        // Create directories if they don't exist
        std::filesystem::create_directories(configPath.parent_path());
        
        // Save the settings file
        SI_Error rc = ini.SaveFile(configPath.string().c_str());
        if (rc < 0) {
            logger::error("Failed to save settings file: {}", rc);
            return false;
        }

        logger::info("Settings saved successfully");
        return true;
    }

    // Getters
    float GetBaseAccuracyBonus() const { return baseAccuracyBonus; }
    float GetAttackAngleMult() const { return attackAngleMult; }
    float GetAimOffsetV() const { return aimOffsetV; }
    float GetAimSightedDelay() const { return aimSightedDelay; }
    bool GetAutoApplyImprovements() const { return autoApplyImprovements; }
    float GetBowAccuracyBonus() const { return bowAccuracyBonus; }
    float GetSpecialBowBonus() const { return specialBowBonus; }
    float GetKnockbackMagnitude() const { return knockbackMagnitude; }
    float GetKnockbackInterval() const { return knockbackInterval; }

    bool IsFollowerEnabled(RE::FormID formID) const {
        for (const auto& [name, id] : followers) {
            if (id == formID) {
                auto it = followersEnabled.find(name);
                if (it != followersEnabled.end()) {
                    return it->second;
                }
                return true; // Default to enabled if not specified
            }
        }
        return false;
    }

    bool IsFollower(RE::FormID formID) const {
        for (const auto& [name, id] : followers) {
            if (id == formID) {
                return true;
            }
        }
        return false;
    }

    bool IsSpecialBow(RE::FormID formID) const {
        for (const auto& [name, id] : specialBows) {
            if (id == formID) {
                return true;
            }
        }
        return false;
    }

    bool IsSpecialSword(RE::FormID formID) const {
        for (const auto& [name, id] : specialSwords) {
            if (id == formID) {
                return true;
            }
        }
        return false;
    }

    const std::unordered_map<std::string, RE::FormID>& GetFollowers() const {
        return followers;
    }

    const std::unordered_map<std::string, RE::FormID>& GetSpecialBows() const {
        return specialBows;
    }

    const std::unordered_map<std::string, RE::FormID>& GetSpecialSwords() const {
        return specialSwords;
    }
};