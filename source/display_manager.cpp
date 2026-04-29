#include "display_manager.h"
#include <cstring>
#include <utility>

// Console objects for dual-screen support
PrintConsole topScreen, bottomScreen;

DisplayManager::DisplayManager() {
}

DisplayManager::~DisplayManager() {
}

void DisplayManager::init() {
    gfxInitDefault();
    consoleInit(GFX_TOP, &topScreen);
    consoleInit(GFX_BOTTOM, &bottomScreen);
}

void DisplayManager::exit() {
    gfxExit();
}

void DisplayManager::clearTopScreen() {
    consoleSelect(&topScreen);
    printf("\x1b[2J"); // Clear screen
}

void DisplayManager::clearBottomScreen() {
    consoleSelect(&bottomScreen);
    printf("\x1b[2J"); // Clear screen
}

void DisplayManager::swapBuffers() {
    gfxFlushBuffers();
    gspWaitForVBlank();
    gfxSwapBuffers();
}

void DisplayManager::drawText(gfxScreen_t screen, int x, int y, const char* text) {
    if (screen == GFX_TOP) {
        consoleSelect(&topScreen);
    } else {
        consoleSelect(&bottomScreen);
    }
    printf("\x1b[%d;%dH%s", y + 1, x + 1, text);
}

void DisplayManager::drawHorizontalLine(gfxScreen_t screen, int y, char style) {
    if (screen == GFX_TOP) {
        consoleSelect(&topScreen);
    } else {
        consoleSelect(&bottomScreen);
    }
    printf("\x1b[%d;0H", y + 1);
    for (int i = 0; i < 40; i++) printf("%c", style);
}

void DisplayManager::drawBox(gfxScreen_t screen, int x, int y, int width, int height) {
    if (screen == GFX_TOP) {
        consoleSelect(&topScreen);
    } else {
        consoleSelect(&bottomScreen);
    }

    // Top border
    printf("\x1b[%d;%dH/", y + 1, x + 1);
    for (int i = 0; i < width - 2; i++) printf("-");
    printf("\\");

    // Side borders
    for (int i = 1; i < height - 1; i++) {
        printf("\x1b[%d;%dH|", y + i + 1, x + 1);
        printf("\x1b[%d;%dH|", y + i + 1, x + width);
    }

    // Bottom border
    printf("\x1b[%d;%dH\\", y + height, x + 1);
    for (int i = 0; i < width - 2; i++) printf("-");
    printf("/");
}

const char* DisplayManager::getTypeColor(PokemonType type) {
    switch (type) {
        case PokemonType::NORMAL:   return COLOR_WHITE;
        case PokemonType::FIRE:     return COLOR_BRIGHT_RED;
        case PokemonType::WATER:    return COLOR_BRIGHT_BLUE;
        case PokemonType::GRASS:    return COLOR_BRIGHT_GREEN;
        case PokemonType::ELECTRIC: return COLOR_BRIGHT_YELLOW;
        case PokemonType::ICE:      return COLOR_BRIGHT_CYAN;
        case PokemonType::FIGHTING: return COLOR_RED;
        case PokemonType::POISON:   return COLOR_MAGENTA;
        case PokemonType::GROUND:   return COLOR_YELLOW;
        case PokemonType::FLYING:   return COLOR_CYAN;
        case PokemonType::PSYCHIC:  return COLOR_BRIGHT_MAGENTA;
        case PokemonType::BUG:      return COLOR_GREEN;
        case PokemonType::ROCK:     return COLOR_YELLOW;
        case PokemonType::GHOST:    return COLOR_MAGENTA;
        case PokemonType::DRAGON:   return COLOR_BRIGHT_MAGENTA;
        case PokemonType::DARK:     return COLOR_WHITE;
        case PokemonType::STEEL:    return COLOR_CYAN;
        case PokemonType::FAIRY:    return COLOR_BRIGHT_MAGENTA;
        default: return COLOR_WHITE;
    }
}

const char* DisplayManager::getStateColor(AppState state) {
    switch (state) {
        case AppState::LIST_VIEW:   return COLOR_BRIGHT_WHITE;
        case AppState::DETAIL_VIEW: return COLOR_BRIGHT_CYAN;
        case AppState::SEARCH_MODE: return COLOR_BRIGHT_YELLOW;
        default: return COLOR_WHITE;
    }
}

const char* DisplayManager::getTypeEmoji(PokemonType type) {
    switch (type) {
        case PokemonType::NORMAL:   return "*";
        case PokemonType::FIRE:     return "^";
        case PokemonType::WATER:    return "~";
        case PokemonType::GRASS:    return "v";
        case PokemonType::ELECTRIC: return "#";
        case PokemonType::ICE:      return "@";
        case PokemonType::FIGHTING: return "+";
        case PokemonType::POISON:   return "x";
        case PokemonType::GROUND:   return "_";
        case PokemonType::FLYING:   return "-";
        case PokemonType::PSYCHIC:  return "o";
        case PokemonType::BUG:      return "&";
        case PokemonType::ROCK:     return "O";
        case PokemonType::GHOST:    return "?";
        case PokemonType::DRAGON:   return "$";
        case PokemonType::DARK:     return "!";
        case PokemonType::STEEL:    return "=";
        case PokemonType::FAIRY:    return "*";
        default: return "?";
    }
}

const char* DisplayManager::getStateLabel(AppState state) {
    switch (state) {
        case AppState::LIST_VIEW:   return "LIST VIEW";
        case AppState::DETAIL_VIEW: return "DETAIL VIEW";
        case AppState::SEARCH_MODE: return "SEARCH MODE";
        default: return "UNKNOWN";
    }
}

void DisplayManager::drawPokemonDetailsTop(const Pokemon& pokemon, AppState state) {
    consoleSelect(&topScreen);
    printf("\x1b[2J"); // Clear screen
    printf("\x1b[0;0H");

    // Header with state indicator and color
    printf("%s[%s]%s\n", getStateColor(state), getStateLabel(state), COLOR_RESET);
    drawHorizontalLine(GFX_TOP, 1, '=');

    // Pokemon ID and Name with visual styling
    printf("\n");
    printf("%s  #%-6u %-20s%s\n", COLOR_BRIGHT_WHITE, pokemon.id, pokemon.name, COLOR_RESET);
    drawHorizontalLine(GFX_TOP, 4, '-');

    // Types with color coding
    printf("\n  Types:\n");
    for (int i = 0; i < pokemon.type_count; i++) {

        printf("    %s%s %s%s\n",
            getTypeColor(pokemon.types[i]),
            getTypeEmoji(pokemon.types[i]),
            type_names[static_cast<int>(std::to_underlying(pokemon.types[i]))],
            COLOR_RESET);
    }

    printf("\n  Description:\n");

    // Word-wrapped description with styled box
    int col = 0;
    for (int i = 0; pokemon.description[i] != '\0' && i < MAX_DESC_LENGTH; i++) {
        if (col >= 34 && pokemon.description[i] == ' ') {
            printf("\n  ");
            col = 0;
        } else {
            printf("%c", pokemon.description[i]);
            col++;
        }
    }
    printf("%s\n", COLOR_RESET);
}

void DisplayManager::drawPokemonListBottom(const Pokemon* pokemon_list, int list_size, int selected_index) {
    consoleSelect(&bottomScreen);
    printf("\x1b[2J"); // Clear screen
    printf("\x1b[0;0H");

    printf("%sPOKEMON LIST%s\n", COLOR_BRIGHT_WHITE, COLOR_RESET);
    drawHorizontalLine(GFX_BOTTOM, 1, '=');
    printf("\n");

    // Calculate which Pokemon to display
    int start_index = (selected_index > 4) ? selected_index - 4 : 0;
    int end_index = (start_index + 8 < list_size) ? start_index + 8 : list_size;

    // Draw Pokemon entries with colored selection
    for (int i = start_index; i < end_index; i++) {
        if (i == selected_index) {
            // Highlighted selection with color
            printf("%s>> #%3u %-14s%s\n", COLOR_BRIGHT_CYAN, pokemon_list[i].id, pokemon_list[i].name, COLOR_RESET);
        } else {
            printf("   #%3u %-14s\n", pokemon_list[i].id, pokemon_list[i].name);
        }
    }

    // Scroll indicators with color
    printf("\n");
    if (start_index > 0) {
        printf("%s[UP]%s ", COLOR_BRIGHT_YELLOW, COLOR_RESET);
    }
    if (end_index < list_size) {
        printf("%s[DOWN]%s\n", COLOR_BRIGHT_YELLOW, COLOR_RESET);
    } else {
        printf("\n");
    }

    // Show selected Pokemon number with color
    printf("%s[%d/%d]%s", COLOR_BRIGHT_GREEN, selected_index + 1, list_size, COLOR_RESET);
}

void DisplayManager::drawSearchModeBottom(const ApplicationState* app_state) {
    consoleSelect(&bottomScreen);
    printf("\x1b[2J"); // Clear screen
    printf("\x1b[0;0H");

    printf("%sSEARCH MODE%s\n", COLOR_BRIGHT_YELLOW, COLOR_RESET);
    drawHorizontalLine(GFX_BOTTOM, 1, '=');
    printf("\nSearch: ");
    printf("%s%s_%s\n", COLOR_BRIGHT_WHITE, app_state->getSearchText(), COLOR_RESET);
    drawHorizontalLine(GFX_BOTTOM, 4, '-');

    printf("\n%sResults:%s\n", COLOR_BRIGHT_CYAN, COLOR_RESET);

    // Show filtered results
    int filtered_count = app_state->getFilteredCount();
    int display_count = (filtered_count > 7) ? 7 : filtered_count;

    for (int i = 0; i < display_count; i++) {
        const Pokemon* poke = app_state->getFilteredPokemon(i);
        if (poke) {
            if (i == app_state->getSelectedIndex()) {
                printf("%s>> #%3u %s%s\n", COLOR_BRIGHT_CYAN, poke->id, poke->name, COLOR_RESET);
            } else {
                printf("   #%3u %s\n", poke->id, poke->name);
            }
        }
    }

    if (filtered_count == 0) {
        printf("%sNo results found.%s\n", COLOR_BRIGHT_RED, COLOR_RESET);
    } else if (filtered_count > 7) {
        printf("%s... and %d more%s\n", COLOR_BRIGHT_YELLOW, filtered_count - 7, COLOR_RESET);
    }
}










