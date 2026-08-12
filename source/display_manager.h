#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

extern "C" {
    #include <3ds.h>
    #include <citro2d.h>
    #include <citro3d.h>
}

#include "pokemon_data.h"
#include "app_state.h"

// ANSI color codes for 3DS console
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_WHITE   "\x1b[37m"
#define COLOR_RESET   "\x1b[0m"
#define COLOR_BRIGHT_RED     "\x1b[1;31m"
#define COLOR_BRIGHT_GREEN   "\x1b[1;32m"
#define COLOR_BRIGHT_YELLOW  "\x1b[1;33m"
#define COLOR_BRIGHT_BLUE    "\x1b[1;34m"
#define COLOR_BRIGHT_MAGENTA "\x1b[1;35m"
#define COLOR_BRIGHT_CYAN    "\x1b[1;36m"
#define COLOR_BRIGHT_WHITE   "\x1b[1;37m"

class DisplayManager {
public:
    // Screen constants
    static constexpr int TOP_WIDTH = 400;
    static constexpr int TOP_HEIGHT = 240;
    static constexpr int BOTTOM_WIDTH = 320;
    static constexpr int BOTTOM_HEIGHT = 240;

    DisplayManager(const DisplayManager&) = delete;
    DisplayManager& operator=(const DisplayManager&) = delete;

    static DisplayManager& getInstance() {
        static DisplayManager instance;
        return instance;
    }

    ~DisplayManager();

    // Screen rendering
    void beginFrame();
    void clearTopScreen();
    void clearBottomScreen();
    void swapBuffers();

    // Rendering functions with state awareness
    void drawPokemonDetails(const Pokemon &pokemon);
    void drawPokemonListBottom(const Pokemon* pokemon_list, int list_size, int selected_index);
    void drawSearchModeBottom(const ApplicationState* app_state);
    void drawViewfinderUI();
    void drawClassifyingUI();
    void drawEmptyListUI();
    void drawProgressBar(float x, float y, float width, float height, float progress);
    void drawSpinningPokeball(float cx, float cy, float radius, float progress, bool top = false);
    void drawErrorUI();
    void drawButtonPrompts(AppState state);
    void drawCameraPreview();
    void updateCameraTexture(u16* linearBuf);

    void drawPokemonSprite(float x, float y, float size);
    void updatePokemonSprite(const std::vector<uint8_t>& tiledBytes, int size);

    // Modern UI Overhaul helper functions
    void drawPokedexCardTop(const Pokemon &pokemon, bool showSprite);
    void drawTypeBadge(float x, float y, PokemonType type, bool is_small = false, bool top = true);

private:
    void initCameraTexture();
    void initSpriteTexture(int size);
    DisplayManager();
    // Helper functions for drawing
    void drawText(gfxScreen_t screen, int x, int y, const char* text);
    void drawHorizontalLine(gfxScreen_t screen, int y, char style);
    void drawBox(gfxScreen_t screen, int x, int y, int width, int height);
    u32 getTypeColor(PokemonType type);
    const char* getTypeEmoji(PokemonType type);
    const char* getStateLabel(AppState state);
    const char* getStateColor(AppState state);

    C3D_Tex cameraTex;
    C2D_Image cameraImage;
    Tex3DS_SubTexture cameraSubTex;
    bool cameraTexInitialized;

    C3D_Tex spriteTex;
    C2D_Image spriteImage;
    Tex3DS_SubTexture spriteSubTex;
    bool spriteTexInitialized;
    int currentSpriteSize;
};

#endif // DISPLAY_MANAGER_H
