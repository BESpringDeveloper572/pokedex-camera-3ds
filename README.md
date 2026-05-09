# 3DS Pokedex App

A high-performance graphical Pokedex application for Nintendo 3DS built with **libctru**, **citro2d**, and **citro3d** in C++. Features a searchable database of Pokémon with dual-screen hardware-accelerated rendering, dynamic sprite fetching, and server-side optimized image processing.

## Project Status

### ✅ COMPLETED - Phase 1: Core Data Structures
- ✅ Pokemon data structure with ID, name, types, and descriptions.
- ✅ Efficient memory-mapped types for 3DS constraints.

### ✅ COMPLETED - Phase 2: UI & Rendering Framework
- ✅ **Hardware Accelerated Rendering:** Fully migrated from `printf` console output to a custom `Renderer` class using **Citro2D**.
- ✅ **Pixel-Perfect Graphics:** Optimized for both the 400x240 top screen and 320x240 bottom screen.
- ✅ **Dynamic Word Wrap:** Custom text engine to handle Pokédex descriptions on the 3DS GPU.
- ✅ **Full Input Support:** D-pad navigation, button shortcuts, and software keyboard integration.

### ✅ COMPLETED - Phase 3: Network & API Integration
- ✅ **RESTful Client:** Implemented via `libcurl` for high-speed API communication.
- ✅ **Dynamic Sprite Fetching:** Automatic download and display of Pokémon artwork.
- ✅ **Server-Side Optimization:** API pre-processes images into the **3DS 8x8 tiled format**, allowing instant `memcpy` loading into VRAM without 3DS-side decoding.
- ✅ **Security:** Token-based authentication for API requests.

### ✅ COMPLETED - Phase 4: Graphics & Polish
- ✅ **Full Type Support:** All 18 Pokémon types (Normal to Fairy) with accurate color badges.
- ✅ **Unified Theme:** Professional dark theme UI with hardware-accelerated text rendering.

## 🎯 Next Steps - What's Left

### HIGH PRIORITY: Data & Optimization
1. **Full Pokedex Population**
   - Expand beyond test Pokémon to the full 1,000+ entries.
   - Implement paginated loading to manage memory.

2. **Local Caching**
   - Save fetched Pokémon and sprites to the SD card.
   - Implement an offline mode for use without Wi-Fi.

### Additional Features (FUTURE)
- [ ] Pokemon stats display (HP, Attack, Defense, etc.)
- [ ] Evolution chains and type effectiveness.
- [ ] Audio integration for Pokémon cries.

## Controls
- **D-pad Up/Down**: Navigate Pokemon list
- **A Button**: Enter detail view / Confirm search
- **B Button**: Return to list / Exit current mode
- **X Button**: Toggle Search Mode (Software Keyboard)
- **Y Button**: Enter Viewfinder Mode (AR Classification)
- **START**: Exit application

## Technical Stack

- **Language**: C++23
- **Graphics Engine**: Citro2D / Citro3D (PICA200 Hardware Acceleration)
- **Networking**: libcurl + mbedTLS
- **JSON Parsing**: json-c
- **Platform**: Nintendo 3DS (devkitPro / arm-none-eabi-gcc)
- **API Backend**: FastAPI (Python) + PIL for 3DS tiling logic

## Configuration

The application requires a `source/secrets.h` file to communicate with the backend API. This file is excluded from version control to protect credentials.

Create `source/secrets.h` with the following content:
```cpp
#ifndef INC_3DS_APP_SECRETS_H
#define INC_3DS_APP_SECRETS_H

#define API_KEY "your_api_key_here"
#define API_HOSTNAME "your_api_hostname_here"
#define USER_AGENT "3DSPokedex/1.0.0"

#endif
```

## Build & Deployment

Build using CMake:
```bash
mkdir build && cd build
cmake ..
make
```

## Project Architecture

```
Core
├── renderer.h/cpp          - Citro2D hardware abstraction layer
├── display_manager.h/cpp   - UI layout and state-aware rendering
├── app_state.h/cpp         - State machine and data management logic

Networking
├── pokemon_api.h/cpp       - libcurl client with tiled sprite support
└── secrets.h               - API credentials and hostname

System
├── input_handler.h/cpp     - 3DS button and touchscreen processing
├── camera.h/cpp            - Hardware camera control for AR features
└── main.cpp                - Synchronized application lifecycle
```
