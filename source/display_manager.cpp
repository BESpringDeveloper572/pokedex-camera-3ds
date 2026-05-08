#include "display_manager.h"
#include <cstdio>
#include <utility>

// Console objects
PrintConsole bottomScreenConsole;
C3D_RenderTarget* topTarget;

DisplayManager::DisplayManager() : cameraTexInitialized(false) {
    gfxInitDefault();
    
    // Initialize Citro3D and Citro2D
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // Create a target for the Top Screen
    topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

    // ONLY initialize console on the bottom screen
    consoleInit(GFX_BOTTOM, &bottomScreenConsole);
}

DisplayManager::~DisplayManager() {
    C2D_Fini();
    C3D_Fini();
    gfxExit();
}

void DisplayManager::clearTopScreen() {
    C2D_TargetClear(topTarget, C2D_Color32(30, 30, 30, 255)); // Dark Grey background
}

void DisplayManager::clearBottomScreen() {
    consoleSelect(&bottomScreenConsole);
    printf("\x1b[2J\x1b[H"); 
}

void DisplayManager::swapBuffers() {
    C3D_FrameEnd(0);
}

void DisplayManager::beginFrame() {
    C3D_FrameBegin(0);
    C2D_SceneBegin(topTarget);
}

// --- Rendering Implementation ---

void DisplayManager::drawPokemonDetailsTop(const Pokemon& pokemon, AppState state) {
    clearBottomScreen();
    u32 color = C2D_Color32(100, 100, 100, 255);
    if (pokemon.type_count > 0) {
        switch(pokemon.types[0]) {
            case PokemonType::FIRE:     color = C2D_Color32(255, 69, 0, 255); break;
            case PokemonType::WATER:    color = C2D_Color32(30, 144, 255, 255); break;
            case PokemonType::GRASS:    color = C2D_Color32(50, 205, 50, 255); break;
            case PokemonType::ELECTRIC: color = C2D_Color32(255, 215, 0, 255); break;
            default: break;
        }
    }
    
    C2D_DrawRectSolid(0, 0, 0.1f, TOP_WIDTH, 40, color);
    
    consoleSelect(&bottomScreenConsole);
    printf("\x1b[1;1H" COLOR_BRIGHT_WHITE "#%03d %-15s" COLOR_RESET, pokemon.id, pokemon.name);
    printf("\x1b[3;1HTypes: ");
    for(int i=0; i<pokemon.type_count; i++) {
        printf("%s[%s] " COLOR_RESET, getTypeColor(pokemon.types[i]), type_names[static_cast<int>(pokemon.types[i])]);
    }
    printf("\x1b[5;1HDescription:\n%s", pokemon.description);
}

void DisplayManager::drawPokemonListBottom(const Pokemon* pokemon_list, int list_size, int selected_index) {
    consoleSelect(&bottomScreenConsole);
    printf("\x1b[8;1H" COLOR_CYAN "--- Pokedex List ---" COLOR_RESET "\n");
    
    for (int i = 0; i < list_size; i++) {
        if (i == selected_index) {
            printf(COLOR_BRIGHT_YELLOW "> #%03d %-15s" COLOR_RESET "\n", pokemon_list[i].id, pokemon_list[i].name);
        } else {
            printf("  #%03d %-15s\n", pokemon_list[i].id, pokemon_list[i].name);
        }
    }
}

void DisplayManager::drawSearchModeBottom(const ApplicationState* app_state) {
    consoleSelect(&bottomScreenConsole);
    printf("\x1b[1;1H" COLOR_MAGENTA "=== SEARCH MODE ===" COLOR_RESET);
    printf("\x1b[3;1HSearch: %s_", app_state->getSearchText());
    
    int count = app_state->getFilteredCount();
    printf("\x1b[5;1HResults found: %d", count);
    
    if (count > 0) {
        printf("\x1b[7;1HPress (A) to view selected");
    }
}

const char* DisplayManager::getTypeColor(PokemonType type) {
    switch (type) {
        case PokemonType::FIRE:     return COLOR_RED;
        case PokemonType::WATER:    return COLOR_BLUE;
        case PokemonType::GRASS:    return COLOR_GREEN;
        case PokemonType::ELECTRIC: return COLOR_YELLOW;
        case PokemonType::PSYCHIC:  return COLOR_MAGENTA;
        default:                    return COLOR_WHITE;
    }
}

// --- Camera Methods ---

void DisplayManager::initCameraTexture() {
    if (cameraTexInitialized) return;
    C3D_TexInit(&cameraTex, 512, 256, GPU_RGB565);
    C3D_TexSetFilter(&cameraTex, GPU_LINEAR, GPU_LINEAR);
    cameraSubTex = { 400, 240, 0.0f, 1.0f, 400.0f/512.0f, 1.0f - (240.0f/256.0f) };
    cameraImage = { &cameraTex, &cameraSubTex };
    cameraTexInitialized = true;
}

void DisplayManager::updateCameraTexture(u16* linearBuf) {
    if (!cameraTexInitialized) initCameraTexture();
    u16* dst = (u16*)cameraTex.data;

    // No direct dependency on Camera class here anymore
    for (u32 y = 0; y < 240; y++) {
        for (u32 x = 0; x < 400; x++) {
            u32 dstPos = ((((y >> 3) * (512 >> 3) + (x >> 3)) << 6) + 
                         ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | 
                          ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3)));
            dst[dstPos] = linearBuf[y * 400 + x];
        }
    }
    C3D_TexFlush(&cameraTex);
}

void DisplayManager::drawCameraPreview() {
    if (!cameraTexInitialized) return;
    C2D_DrawImageAt(cameraImage, 0, 0, 0.5f);
}

void DisplayManager::drawViewfinderUI() {
    consoleSelect(&bottomScreenConsole);
    printf("\x1b[10;5H" COLOR_BRIGHT_WHITE "Point at a Pokemon and press (R)" COLOR_RESET);
    printf("\x1b[12;10H" COLOR_YELLOW "Press (B) to Cancel" COLOR_RESET);
}
