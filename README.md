# ColdSun's Combat Classes

## Overview
This SKSE Plugin is designed as a mod author's resource that extends combat functionality of followers in Skyrim. It improves follower combat AI, especially for ranged combat, and adds special weapon effects.

## Features
- **Improved Follower Accuracy:** Enhances follower's accuracy with ranged weapons
- **Special Weapon Bonuses:** Configure special bows and weapons with unique bonuses
- **Knockback Effect:** Special swords can trigger automatic knockback effects against enemies
- **Configurable:** All settings can be adjusted through an INI file
- **Multiple Follower Support:** Works with any number of custom followers
- **Performance Friendly:** Implemented as an SKSE plugin instead of Papyrus for better performance

## Requirements
- Skyrim Special Edition (Latest version)
- SKSE SE 2.2.3+
- Address Library for SKSE Plugins

## Installation
1. Install SKSE and Address Library
2. Extract the archive into your Skyrim/Data folder
3. Configure the plugin by editing `Data/SKSE/Plugins/CS_CombatClasses/Settings.ini`
4. Launch the game through SKSE

## Configuration
The plugin is configured through `Settings.ini` located in the `Data/SKSE/Plugins/CS_CombatClasses` directory. You can customize:

### General Settings
- Base accuracy bonus
- Attack angle multiplier
- Aim offset and delay values
- Bow accuracy bonuses
- Knockback effects

### Follower Configuration
Add followers by creating sections like:
```ini
[Follower:FollowerName]
FormID=0123ABC ; Form ID in hexadecimal
Plugin=YourMod.esp ; Plugin containing the follower
Enabled=true ; Enable or disable this follower
```

### Special Weapons
Configure special weapons that provide additional bonuses:
```ini
[SpecialBow:BowName]
FormID=0123DEF ; Form ID in hexadecimal
Plugin=YourMod.esp ; Plugin containing the bow

[SpecialSword:SwordName]
FormID=0123GHI ; Form ID in hexadecimal
Plugin=YourMod.esp ; Plugin containing the sword
```

## For Mod Authors
This plugin is designed as a resource for mod authors to enhance follower capabilities. To integrate with your mod:

1. Include this plugin as a requirement for your mod
2. Configure your followers and special weapons in the settings file
3. Distribute the configured settings file with your mod

## Building from Source
The project uses CMake and vcpkg for building:

1. Install Visual Studio 2022 with C++ development tools
2. Install CMake
3. Clone the repository
4. Set up vcpkg according to the included vcpkg.json
5. Build using CMake

## Credits
- Author: heathbrownkeyworks
- Original concept derived from the Papyrus-based CSV_SamandrielAccuracyScript

## License
MIT License - See LICENSE file for details

## Compatibility
- Compatible with most follower mods
- Should work with combat overhauls, but specific interactions may vary

## Known Issues
- None currently. Please report any issues you find.

## Future Plans
- Add combat style customization
- Implement more special weapon effects
- Add support for magic and spell improvements