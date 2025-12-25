# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Écran Stratégie V4** is an embedded robotics control system for the C.R.A.C (French robotics competition) 2025. It runs on an STM32F469NI Discovery board and provides a touchscreen interface for strategy selection, robot control, and actuator testing.

**Current Version**: v0.1 (early development)

## Build Commands

### Building the Project
```bash
pio run
```

### Uploading to Board
```bash
pio run --target upload
```

### Monitoring Serial Output
```bash
pio device monitor --baud 921600
```

### Clean Build
```bash
pio run --target clean
```

## Hardware Platform

- **Board**: STM32F469NI-Discovery (DISCO_F469NI)
- **MCU**: STM32F469 (ARM Cortex-M4, 168 MHz)
- **Framework**: mbed OS (RTOS)
- **Display**: 4-inch 800x480 LCD touchscreen (OTM8009A controller)
- **Storage**: SD card (FAT filesystem mounted at `/sd/`)
- **Communication**:
  - CAN2 bus at 1 Mbps (RX=PB_5, TX=PB_13) - **Note: CAN1 conflicts with touchscreen**
  - USB Serial at 921600 baud
- **Audio**: CS43L22 codec for MP3 playback

## Architecture Overview

### Threading Model

The application uses a multi-threaded architecture with 4-5 primary threads:

1. **Main Thread**: State machine handling user interaction and match orchestration
2. **ThreadLvgl**: GUI rendering with 5ms refresh ticker (LVGL 8.3.4)
3. **ThreadCAN**: CAN bus communication (3 internal threads: read, dispatch, write)
4. **ThreadSD**: SD card file system operations
5. **ThreadSound**: MP3 audio playback

### Core Component Interaction

```
main.cpp
  ├─ Ihm (GUI via ThreadLvgl)
  ├─ ThreadCAN (CAN bus abstraction)
  │   ├─ Deplacement (robot movement)
  │   └─ Herkulex (servo/actuator control)
  ├─ ThreadSD (file system)
  └─ Strategie (state machine + instruction execution)
```

### Custom Libraries (lib/)

Critical custom libraries and their responsibilities:

- **threadCAN**: CAN bus abstraction with callback registration system
  - Use `threadCAN.registerIds(idMin, idMax, callback)` to handle CAN message ranges
  - CAN2 only (CAN1 incompatible with touchscreen)

- **threadLvgl**: LVGL GUI rendering engine
  - 5ms ticker for display refresh
  - Thread-safe with mutex protection

- **threadSD**: FAT file system with CAN remote control capability
  - Mounted at `/sd/`
  - Can be controlled via 4 consecutive CAN IDs (default 0x3F0-0x3F3)

- **threadSound**: MP3 playback using Helix decoder

- **ihm**: Human-Machine Interface (GUI implementation)
  - Tab-based interface: SD Init, Match, Config, Actionneur
  - Event flags for button interactions (IHM_FLAG_*)

- **strategie**: Strategy state machine
  - Main handler: `canProcessRx()` processes CAN feedback
  - Main loop: `machineStrategie()` executes instructions

- **deplacement**: Robot movement control via CAN
  - Sends position commands (XYT, rotation, lines, curves)
  - Receives odometry feedback

- **herkulex**: Servo motor and actuator control via CAN
  - Controls servos (RGB LED feedback), grippers, vacuum systems

- **instruction**: Strategy file parsing
  - Text-based instruction format (one per line)
  - Instruction types: MV_BEZIER, MV_LINE, MV_TURN, MV_XYT, ACTION, etc.

- **evitement**: Obstacle avoidance logic
  - States: NO_POINT, DANGER_MV, OBSTACLE_SUR_TRAJECTOIRE, NO_DANGER

- **identCrac**: CAN ID definitions (254 constants)
  - Motor control: 0x020-0x038
  - Actuators: 0x255-0x277
  - Telemetry: 0x3B0-0x3B9
  - Debug: 0x5A0-0x5C6

- **config**: INI-based configuration management
  - Uses mIni library (header-only)
  - Access with `config["Section"]["key"]` notation

## State Machine Flow

### Main State Machine (main.cpp)
```
multi_init (idle state)
  ├─ departClicked() → show_run_page → run_show() thread
  ├─ Test_ventouse() → test (actuator testing)
  ├─ construction_niveaux_2() → test
  ├─ Niveaux_base() → test
  ├─ Position_init() → test
  └─ lacherflag() → test
```

### Strategy Execution (strategie.cpp)
```
ETAT_GAME_INIT
  ├─ ETAT_GAME_RECALAGE (calibration)
  ├─ ETAT_GAME_WAIT_FOR_JACK
  ├─ ETAT_GAME_START (100s match timer)
  ├─ ETAT_GAME_LOAD_NEXT_INSTRUCTION
  ├─ ETAT_GAME_PROCESS_INSTRUCTION
  ├─ ETAT_GAME_WAIT_ACK (blocks until CAN acknowledgment)
  ├─ ETAT_GAME_OBSTACLE (lidar detection handling)
  └─ ETAT_END_LOOP
```

## Important Patterns and Conventions

### CAN Message Handling
Register callback handlers for CAN ID ranges:
```cpp
threadCAN.registerIds(0x01, 0x7FF, canProcessRx);

void canProcessRx(CANMessage *rxMsg) {
    // Handle received CAN message
}
```

### Thread Safety
- **Mutexes**: Used in ThreadLvgl and ThreadSD for critical sections
- **Event Flags**: Used for inter-thread signaling (e.g., ACK waiting)
- **Thread Priorities**:
  - CAN read/dispatch: osPriorityHigh
  - CAN write: osPriorityAboveNormal
  - Main/SD/LVGL: Normal

### GUI Event Model (LVGL)
GUI events use static handlers that set flags checked by main thread:
```cpp
static void eventHandler(lv_event_t *e) {
    // Set IHM_FLAG_* flags
}

// Main thread checks:
if (ihm.departClicked()) { ... }
```

### Strategy File Format
Text files with one instruction per line stored in `/sd/strategie/`:
```
B 2 100 50 0 3.14 1 2 0 50 100 0 0    # Bezier curve
L 1000 N 1 0 0                        # Line movement
T 90 R 1 0 0                          # Turn
A PINCE 1 2                           # Action (gripper)
```

## File System Structure

SD card mounted at `/sd/`:
```
/sd/
├── strategie/      # Strategy text files
├── mp3/            # Audio files
└── config.ini      # Configuration file
```

Configuration is read via `readConfig()` which populates the global `config` map.

## Key Global Variables

Located in `global.h`:
- `x_robot`, `y_robot`, `theta_robot`: Current robot position
- `target_x_robot`, `target_y_robot`, `target_theta_robot`: Target position
- `Cote`: Team color (0=JAUNE, 1=VIOLET)
- `color`: Match starting color
- `gameEtat`: Strategy state machine state
- `listeInstructions`: Parsed instruction queue

## Development Notes

### Initialization Sequence
1. Serial initialization (921600 baud)
2. ThreadSD registers with ThreadCAN
3. ThreadCAN registers callback for IDs 0x01-0x7FF
4. Read configuration from SD card
5. List strategy files from `/sd/strategie/`
6. Display GUI and wait for user interaction

### System Reset
The system performs `NVIC_SystemReset()` after:
- Match completion
- Manual reset via actuator test menu (option 4)
- SD card absence timeout (60 seconds)

### Actuator Testing
The actuator test interface allows testing individual components:
- Vacuum systems (front/rear)
- Construction level mechanisms
- Base level mechanisms
- Position initialization
- Flag release mechanisms

Each test prompts for:
1. Actuator selection (Avant/Arrière/Les deux/Annuler)
2. Level selection (interactive button matrix)

### Memory Management
- Threads created with `new Thread()` in `run_show()`
- LVGL objects allocated once in IHM constructor
- CAN mail buffers: 100 messages each (read + write queues)

### Debug Output
- Serial console via USB at 921600 baud
- Printf statements throughout for state tracking
- LED indicators on GPIO pins: PG_6, PD_4, PD_5
- Jack pin (D7) for safety interlock

## Common Issues and Solutions

### CAN Bus
- Only CAN2 is available (CAN1 conflicts with touchscreen LCD pins)
- CAN baud rate is 1 Mbps
- Use callback registration for handling message ranges

### SD Card
- Verify card is inserted before operations
- ThreadSD waits for FLAG_READY before proceeding
- File paths must start with `/sd/`

### LVGL GUI
- GUI operations must occur in ThreadLvgl context or be mutex-protected
- Use `ihm.getFlag()` for thread-safe event checking
- 5ms refresh rate for smooth UI updates

### Build Issues
- The `mbedignore.py` pre-script handles mbed library exclusions
- Build flag `-D USE_STM32469I_DISCO_REVB` is required for board support
- All custom libraries must be listed in `platformio.ini` under `lib_deps`
