# SimpleMultishot Plugin

A Skyrim SKSE plugin that provides two powerful archery enhancement systems: **Multishot** for firing multiple arrows simultaneously, and **Penetrating Arrows** for arrows that pierce through enemies.

## Features

### Multishot System
- Fire multiple arrows at once by pressing the 'C' key (configurable)
- Works only with bows (not crossbows) and only when bow is drawn and ready
- Configurable number of arrows (2-10, default: 3)
- Horizontal fan spread pattern with configurable angle
- Smart ammunition consumption (only consumes arrows for successful launches)
- Built-in cooldown system (20 seconds default) to prevent spam firing
- 5-second ready window after cooldown ends
- Proper 3D aiming with pitch compensation
- Animation state validation for realistic bow mechanics

### Penetrating Arrow System
- Hold bow at full draw for 3 continuous seconds to charge a penetrating arrow
- Charged arrows pierce through NPCs while dealing damage, but stop at walls/objects
- Enchantment effects are removed from penetrating arrows for balance
- Enhanced damage (2x power) and speed (1.5x) for penetrating shots
- Built-in cooldown system (10 seconds default) after firing
- Requires truly continuous draw - stopping and resuming resets the charge
- Cannot start charging while multishot is active

### General Features
- Both systems can be used independently, with one-way restriction
- Fully configurable via INI file with validation
- Real-time notifications for system status
- Comprehensive logging for debugging and state tracking

## Installation

1. Install SKSE64
2. Copy the compiled DLL to `Data/SKSE/Plugins/`
3. Copy `Multishot.ini` to `Data/SKSE/Plugins/` (optional, for custom configuration)

## Configuration

Edit `Data/SKSE/Plugins/Multishot.ini` to customize both systems:

### Multishot Settings
- `bEnabled`: Enable/disable multishot (1/0, default: 1)
- `iArrowCount`: Number of arrows per multishot (2-10, default: 3)
- `fSpreadAngle`: Spread angle in degrees (5-45, default: 15.0)
- `iKeyCode`: Key code for activation (default: 46 = 'C' key)
- `fReadyWindowDuration`: Ready window duration in seconds (1-30, default: 5.0)
- `fCooldownDuration`: Cooldown duration in seconds (5-300, default: 20.0)

### Penetrating Arrow Settings
- `bEnabled`: Enable/disable penetrating arrows (1/0, default: 1)
- `fChargeTime`: Time to charge penetrating arrow in seconds (1-10, default: 3.0)
- `fCooldownDuration`: Cooldown duration in seconds (0-300, default: 10.0)

### Key Codes Reference

Common key codes for `iKeyCode`:
- C = 46 (default)
- V = 47
- B = 48
- X = 45
- Z = 44

## Usage

### Multishot System
1. Equip a bow and draw it
2. Press the configured key ('C' by default) to activate multishot mode
3. You have 5 seconds to fire your multishot
4. Release the arrow to fire multiple arrows simultaneously
5. Wait for the 20-second cooldown before activating again

### Penetrating Arrow System
1. Equip a bow and draw it fully
2. Hold the bow at full draw for 3 continuous seconds
3. You'll see the arrow become charged (check logs for confirmation)
4. Release to fire a penetrating arrow that pierces through enemies
5. Wait for the 10-second cooldown before charging again

### System Interaction
- **One-Way Restriction**: You cannot start charging a penetrating arrow while multishot is active
- **Multishot Priority**: You can activate multishot at any time, even while charging a penetrating arrow
- **Independent Cooldowns**: Each system maintains its own cooldown timer
- **Flexible Usage**: Multishot can interrupt penetrating arrow charging, allowing tactical choices

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

### Multishot System
- Registers an input event handler to capture key presses
- Uses animation events to detect arrow release
- Validates player state and weapon type
- Calculates spread angles for arrow trajectories
- Launches multiple projectiles using the game's arrow system
- Manages ammunition consumption and cooldown states

### Penetrating Arrow System
- Uses animation events (`bowDraw`, `arrowRelease`) for bow state tracking
- Implements continuous draw detection with timeout mechanism
- Modifies projectile runtime data for penetration behavior
- Sets `impactResult` to `kImpale` for damage + penetration
- Removes enchantment effects from penetrating arrows
- Independent state management with charging and cooldown phases

### Shared Infrastructure
- Both systems share an update loop for state management
- **One-Way Restriction Logic**: Penetrating arrow checks multishot state before charging
- Cross-system notification updates ensure proper UI feedback
- Comprehensive logging system for debugging and monitoring
- Thread-safe singleton pattern for handler management

## License

MIT License - see LICENSE file for details.