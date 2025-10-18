# Multishot Plugin

A Skyrim SKSE plugin that allows players to fire multiple arrows simultaneously when using a bow.

## Features

- Fire multiple arrows at once by pressing the 'C' key (configurable)
- Works only with bows (not crossbows) and only when bow is drawn and ready
- Configurable number of arrows (2-10, default: 3)
- Horizontal fan spread pattern with configurable angle
- Smart ammunition consumption (only consumes arrows for successful launches)
- Built-in cooldown system to prevent spam firing
- Proper 3D aiming with pitch compensation
- Animation state validation for realistic bow mechanics
- Fully configurable via INI file with validation

## Installation

1. Install SKSE64
2. Copy the compiled DLL to `Data/SKSE/Plugins/`
3. Copy `Multishot.ini` to `Data/SKSE/Plugins/` (optional, for custom configuration)

## Configuration

Edit `Data/SKSE/Plugins/Multishot.ini` to customize:

- `bEnabled`: Enable/disable the plugin (1/0)
- `iArrowCount`: Number of arrows per multishot (default: 3)
- `fSpreadAngle`: Spread angle in degrees (default: 15.0)
- `iKeyCode`: Key code for activation (default: 46 = 'C' key)

### Key Codes Reference

Common key codes for `iKeyCode`:
- C = 46 (default)
- V = 47
- B = 48
- X = 45
- Z = 44

## Usage

1. Equip a bow and draw it
2. Press the configured key ('C' by default) to fire multiple arrows
3. Ensure you have sufficient arrows in your inventory

The plugin will:
- Check if you have a bow equipped (not crossbow)
- Verify the weapon is drawn
- Ensure you have enough arrows for the multishot
- Fire arrows in a horizontal fan pattern
- Consume the appropriate number of arrows from your inventory

## Requirements

- Skyrim Special Edition or Anniversary Edition
- SKSE64
- Sufficient arrows in inventory for multishot

## Building

1. Install Visual Studio 2022 with C++ development tools
2. Install vcpkg and set the `VCPKG_ROOT` environment variable
3. Clone this repository with submodules:
   ```
   git submodule add -b ng https://github.com/alandtse/CommonLibVR.git extern/CommonLibVR
   git submodule update --init --recursive
   ```
4. Open the project in Visual Studio 2022 or your preferred C++ IDE
5. Build the project (CMake will automatically configure dependencies)

## Technical Details

This plugin uses CommonLibSSE-NG and supports Skyrim SE, AE, GOG, and VR versions.

The multishot system:
- Registers an input event handler to capture key presses
- Validates player state and weapon type
- Calculates spread angles for arrow trajectories
- Launches multiple projectiles using the game's arrow system
- Manages ammunition consumption

## License

MIT License - see LICENSE file for details.