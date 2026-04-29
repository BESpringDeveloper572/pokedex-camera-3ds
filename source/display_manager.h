#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "3ds.h"
#include <cstdio>
#include "pokemon_data.h"
#include "app_state.h"

// Console objects for dual-screen support
extern PrintConsole topScreen, bottomScreen;

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

    DisplayManager();
    ~DisplayManager();

    // Initialization
    void init();
    void exit();

    // Screen rendering
    void clearTopScreen();
    void clearBottomScreen();
    void swapBuffers();

    // Rendering functions with state awareness
    void drawPokemonDetailsTop(const Pokemon& pokemon, AppState state);
    void drawPokemonListBottom(const Pokemon* pokemon_list, int list_size, int selected_index);
    void drawSearchModeBottom(const ApplicationState* app_state);

private:
    // Helper functions for drawing
    void drawText(gfxScreen_t screen, int x, int y, const char* text);
    void drawHorizontalLine(gfxScreen_t screen, int y, char style);
    void drawBox(gfxScreen_t screen, int x, int y, int width, int height);
    const char* getTypeColor(PokemonType type);
    const char* getTypeEmoji(PokemonType type);
    const char* getStateLabel(AppState state);
    const char* getStateColor(AppState state);
};

#endif // DISPLAY_MANAGER_H





