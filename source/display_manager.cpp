#include "display_manager.h"
#include "renderer.h"
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <utility>

DisplayManager::DisplayManager() : cameraTexInitialized(false), spriteTexInitialized(false), currentSpriteSize(0) {
    Renderer::getInstance().init();
}

DisplayManager::~DisplayManager() {
    if (spriteTexInitialized) C3D_TexDelete(&spriteTex);
}

void DisplayManager::clearTopScreen() {
    // Renderer handles clearing via scene begin or explicit clear if needed
}

void DisplayManager::clearBottomScreen() {
    // No longer using console
}

void DisplayManager::swapBuffers() {
    Renderer::getInstance().endFrame();
}

void DisplayManager::beginFrame() {
    Renderer::getInstance().beginFrame();
}

// --- Rendering Implementation ---

void DisplayManager::drawPokemonDetails(const Pokemon &pokemon) {
    auto& r = Renderer::getInstance();
    
    // Clear screen to premium slate
    r.drawRect(0, 0, BOTTOM_WIDTH, BOTTOM_HEIGHT, Renderer::Color(18, 18, 28, 255), false);
    
    // Sleek header bar
    r.drawRect(0, 0, BOTTOM_WIDTH, 30, Renderer::Color(25, 25, 38, 255), false);
    r.drawRect(0, 30, BOTTOM_WIDTH, 2, Renderer::Color(50, 50, 70, 255), false);
    r.drawText(10, 6, 0.55f, Renderer::Color(255, 255, 255), "POKEDEX DATABASE", false);
    
    // Species tag in header
    r.drawText(180, 7, 0.45f, Renderer::Color(170, 175, 195), pokemon.species, false);
    
    // Height/Weight conversions
    float totalInches = pokemon.height * 3.93701f;
    int roundedInches = static_cast<int>(totalInches + 0.5f);
    int feet = roundedInches / 12;
    int inches = roundedInches % 12;
    float lbs = pokemon.weight * 0.220462f;
    
    char hVal[32];
    snprintf(hVal, sizeof(hVal), "%d' %02d\"  (%d.%dm)", feet, inches, pokemon.height / 10, pokemon.height % 10);
    char wVal[32];
    snprintf(wVal, sizeof(wVal), "%.1f lbs  (%.1f kg)", lbs, pokemon.weight / 10.0f);
    
    // 1. Height Box
    r.drawRectOutline(8, 38, 148, 48, 1.0f, Renderer::Color(45, 45, 60, 255), false);
    r.drawRect(9, 39, 146, 46, Renderer::Color(24, 24, 36, 255), false);
    r.drawText(14, 43, 0.32f, Renderer::Color(140, 140, 160), "HEIGHT PARAMETERS", false);
    r.drawText(14, 59, 0.44f, Renderer::Color(255, 255, 255), hVal, false);
    
    // 2. Weight Box
    r.drawRectOutline(164, 38, 148, 48, 1.0f, Renderer::Color(45, 45, 60, 255), false);
    r.drawRect(165, 39, 146, 46, Renderer::Color(24, 24, 36, 255), false);
    r.drawText(170, 43, 0.32f, Renderer::Color(140, 140, 160), "WEIGHT PARAMETERS", false);
    r.drawText(170, 59, 0.44f, Renderer::Color(255, 255, 255), wVal, false);
    
    // 3. Description Panel
    r.drawRectOutline(8, 92, 304, 118, 1.0f, Renderer::Color(45, 45, 60, 255), false);
    r.drawRect(9, 93, 302, 116, Renderer::Color(22, 22, 32, 255), false);
    
    // Description Divider
    r.drawRect(9, 113, 302, 1, Renderer::Color(45, 45, 60, 255), false);
    r.drawText(14, 97, 0.38f, Renderer::Color(150, 150, 170), "POKEDEX ANALYZER PROFILE", false);
    
    // Audio Readout Pill
    r.drawRect(198, 96, 102, 14, Renderer::Color(25, 60, 120, 200), false);
    r.drawRectOutline(198, 96, 102, 14, 1.0f, Renderer::Color(60, 120, 255, 255), false);
    r.drawText(204, 99, 0.28f, Renderer::Color(255, 255, 255), "🎤 AUDIO PLAYBACK", false);
    
    // Description text body wrapped nicely
    r.drawTextWrapped(14, 120, 0.42f, 290.0f, Renderer::Color(205, 210, 225), pokemon.description, false);
}

void DisplayManager::drawPokemonListBottom(const Pokemon* pokemon_list, int list_size, int selected_index) {
    auto& r = Renderer::getInstance();
    auto& app = ApplicationState::getInstance();
    
    // Clear screen to premium slate
    r.drawRect(0, 0, BOTTOM_WIDTH, BOTTOM_HEIGHT, Renderer::Color(18, 18, 28, 255), false);
    
    // Sleek header bar
    r.drawRect(0, 0, BOTTOM_WIDTH, 30, Renderer::Color(25, 25, 38, 255), false);
    r.drawRect(0, 30, BOTTOM_WIDTH, 2, Renderer::Color(50, 50, 70, 255), false);
    
    r.drawText(10, 6, 0.55f, Renderer::Color(255, 255, 255), "POKEDEX DIRECTORY", false);
    
    // Sort Mode pill badge
    const char* sortText = (app.getSortMode() == SortMode::NUMERICAL) ? "SORT: # ID" : "SORT: A-Z";
    r.drawRect(185, 5, 60, 20, Renderer::Color(45, 45, 65, 255), false);
    r.drawRectOutline(185, 5, 60, 20, 1.0f, Renderer::Color(80, 80, 110, 255), false);
    r.drawText(189, 9, 0.3f, Renderer::Color(200, 200, 220), sortText, false);
    
    // Scanned Counter pill badge
    char scannedCount[32];
    snprintf(scannedCount, sizeof(scannedCount), "SCANNED: %d", app.getPokemonCount());
    r.drawRect(250, 5, 62, 20, Renderer::Color(30, 80, 40, 200), false);
    r.drawRectOutline(250, 5, 62, 20, 1.0f, Renderer::Color(60, 160, 80, 255), false);
    r.drawText(254, 9, 0.3f, Renderer::Color(255, 255, 255), scannedCount, false);
    
    // Capped visible items (7 items)
    int max_visible = 7;
    int scroll_offset = 0;
    if (list_size > max_visible) {
        scroll_offset = selected_index - 3;
        if (scroll_offset < 0) scroll_offset = 0;
        if (scroll_offset > list_size - max_visible) {
            scroll_offset = list_size - max_visible;
        }
    }
    
    int end_index = std::min(list_size, scroll_offset + max_visible);
    
    for (int i = scroll_offset; i < end_index; i++) {
        float itemY = 36 + (i - scroll_offset) * 25;
        bool selected = (i == selected_index);
        
        u32 primTypeColor = getTypeColor(pokemon_list[i].types[0]);
        
        // Render capsule background
        if (selected) {
            // Highlighting capsule using a consistent sleek slate-blue focus color
            r.drawRect(8, itemY, 286, 21, Renderer::Color(40, 50, 80, 255), false);
            r.drawRectOutline(8, itemY, 286, 21, 1.0f, Renderer::Color(255, 255, 255, 200), false);
            r.drawRect(8, itemY, 4, 21, Renderer::Color(255, 255, 255), false); // white focus bar
        } else {
            // Muted capsule
            r.drawRect(8, itemY, 286, 21, Renderer::Color(25, 25, 38, 255), false);
            r.drawRectOutline(8, itemY, 286, 21, 1.0f, Renderer::Color(45, 45, 60, 255), false);
        }
        
        u32 textColor = selected ? Renderer::Color(255, 255, 255) : Renderer::Color(180, 185, 200);
        u32 idColor = selected ? Renderer::Color(255, 255, 255) : Renderer::Color(120, 125, 145);
        
        char idStr[16];
        snprintf(idStr, sizeof(idStr), "#%03d", pokemon_list[i].id);
        r.drawText(18, itemY + 4, 0.45f, idColor, idStr, false);
        r.drawText(56, itemY + 4, 0.48f, textColor, pokemon_list[i].name, false);
    }
    
    // Draw vertical scrollbar if list size exceeds visible rows
    if (list_size > max_visible) {
        float scrollTrackY = 36;
        float scrollTrackH = max_visible * 25 - 4;
        r.drawRect(304, scrollTrackY, 4, scrollTrackH, Renderer::Color(35, 35, 50, 255), false);
        
        float thumbH = std::max(16.0f, ((float)max_visible / (float)list_size) * scrollTrackH);
        float thumbY = scrollTrackY + ((float)scroll_offset / (float)(list_size - max_visible)) * (scrollTrackH - thumbH);
        
        r.drawRect(304, thumbY, 4, thumbH, Renderer::Color(160, 160, 180, 255), false);
    }
}

void DisplayManager::drawSearchModeBottom(const ApplicationState* app_state) {
    auto& r = Renderer::getInstance();
    r.drawRect(0, 0, BOTTOM_WIDTH, BOTTOM_HEIGHT, Renderer::Color(40, 20, 40), false);
    r.drawText(10, 10, 0.7f, Renderer::Color(255, 0, 255), "=== SEARCH MODE ===", false);
    
    char searchPrompt[128];
    snprintf(searchPrompt, sizeof(searchPrompt), "Search: %s_", app_state->getSearchText());
    r.drawText(10, 50, 0.6f, Renderer::Color(255, 255, 255), searchPrompt, false);
    
    char resultsCount[64];
    snprintf(resultsCount, sizeof(resultsCount), "Results found: %d", app_state->getFilteredCount());
    r.drawText(10, 80, 0.5f, Renderer::Color(200, 200, 200), resultsCount, false);
}

u32 DisplayManager::getTypeColor(PokemonType type) {
    switch (type) {
        case PokemonType::NORMAL:   return Renderer::Color(168, 168, 120);
        case PokemonType::FIRE:     return Renderer::Color(240, 128, 48);
        case PokemonType::WATER:    return Renderer::Color(104, 144, 240);
        case PokemonType::GRASS:    return Renderer::Color(120, 200, 80);
        case PokemonType::ELECTRIC: return Renderer::Color(248, 208, 48);
        case PokemonType::ICE:      return Renderer::Color(152, 216, 216);
        case PokemonType::FIGHTING: return Renderer::Color(192, 48, 40);
        case PokemonType::POISON:   return Renderer::Color(160, 64, 160);
        case PokemonType::GROUND:   return Renderer::Color(224, 192, 104);
        case PokemonType::FLYING:   return Renderer::Color(168, 144, 240);
        case PokemonType::PSYCHIC:  return Renderer::Color(248, 88, 136);
        case PokemonType::BUG:      return Renderer::Color(168, 184, 32);
        case PokemonType::ROCK:     return Renderer::Color(184, 160, 56);
        case PokemonType::GHOST:    return Renderer::Color(112, 88, 152);
        case PokemonType::DRAGON:   return Renderer::Color(112, 56, 248);
        case PokemonType::DARK:     return Renderer::Color(112, 88, 72);
        case PokemonType::STEEL:    return Renderer::Color(184, 184, 208);
        case PokemonType::FAIRY:    return Renderer::Color(238, 153, 172);
        default:                    return Renderer::Color(100, 100, 100);
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
    C2D_SceneBegin(Renderer::getInstance().getTopTarget());
    C2D_DrawImageAt(cameraImage, 0, 0, 0.5f);
}

void DisplayManager::drawViewfinderUI() {
    auto& r = Renderer::getInstance();
    // Background for better visibility
    r.drawRect(0, 0, BOTTOM_WIDTH, BOTTOM_HEIGHT, Renderer::Color(0, 0, 0, 180), false);
    
    r.drawText(40, 100, 0.6f, Renderer::Color(255, 255, 255), "Point at a Pokemon and press (R)", false);
    r.drawText(110, 180, 0.55f, Renderer::Color(255, 255, 80), "(B) Cancel", false);
}

void DisplayManager::drawSpinningPokeball(float cx, float cy, float radius, float progress, bool top) {
    auto& r = Renderer::getInstance();

    (void)progress; // Pure continuous spin regardless of progress parameter

    // Smooth continuous rotation angle
    static float angleOffset = 0.0f;
    angleOffset += 0.10f;
    if (angleOffset >= 6.28318530718f) {
        angleOffset -= 6.28318530718f;
    }
    float rotation = angleOffset;

    // 1. Outer Dark Shadow/Border
    r.drawCircle(cx, cy, radius + 3.0f, Renderer::Color(15, 15, 25, 255), top);

    // 2. Top Half (Red Hemisphere) & Bottom Half (White Hemisphere)
    const int numSegments = 32;
    for (int i = 0; i < numSegments; i++) {
        float a1 = rotation + (float)i / (float)numSegments * 6.28318530718f;
        float a2 = rotation + (float)(i + 1) / (float)numSegments * 6.28318530718f;

        float x1 = cx + radius * cosf(a1);
        float y1 = cy + radius * sinf(a1);
        float x2 = cx + radius * cosf(a2);
        float y2 = cy + radius * sinf(a2);

        u32 color = (i < (numSegments / 2)) ? Renderer::Color(230, 45, 45, 255) : Renderer::Color(245, 245, 250, 255);
        r.drawTriangle(cx, cy, x1, y1, x2, y2, color, top);
    }

    // 3. Middle Black Band
    float bandLength = radius * 1.02f;
    float bx1 = cx - bandLength * cosf(rotation);
    float by1 = cy - bandLength * sinf(rotation);
    float bx2 = cx + bandLength * cosf(rotation);
    float by2 = cy + bandLength * sinf(rotation);
    float bandThickness = std::max(2.5f, radius * 0.22f);
    r.drawLine(bx1, by1, bx2, by2, bandThickness, Renderer::Color(25, 25, 35, 255), top);

    // 4. Center Button Outer Ring
    float buttonOuterR = radius * 0.35f;
    r.drawCircle(cx, cy, buttonOuterR, Renderer::Color(25, 25, 35, 255), top);

    // 5. Center Button Inner White Circle
    float buttonInnerR = radius * 0.22f;
    r.drawCircle(cx, cy, buttonInnerR, Renderer::Color(255, 255, 255, 255), top);

    // 6. Center Button Innermost Pulsing Core
    float pulse = 0.5f + 0.5f * sinf(angleOffset * 2.0f);
    u8 pulseByte = static_cast<u8>(pulse * 25.0f);
    u32 coreColor = Renderer::Color(220, 230 + pulseByte, 255, 255);
    float buttonCoreR = radius * 0.12f;
    r.drawCircle(cx, cy, buttonCoreR, coreColor, top);
}

void DisplayManager::drawClassifyingUI() {
    auto& r = Renderer::getInstance();
    r.drawRect(0, 0, BOTTOM_WIDTH, BOTTOM_HEIGHT, Renderer::Color(18, 18, 28, 255), false);

    // Header bar
    r.drawRect(0, 0, BOTTOM_WIDTH, 30, Renderer::Color(25, 25, 38, 255), false);
    r.drawRect(0, 30, BOTTOM_WIDTH, 2, Renderer::Color(50, 50, 70, 255), false);
    r.drawText(10, 6, 0.55f, Renderer::Color(255, 255, 255), "POKEDEX AI VISION", false);

    const char* statusText = "CLASSIFYING TARGET POKEMON...";
    float tw = r.getTextWidth(statusText, 0.44f);
    r.drawText((BOTTOM_WIDTH - tw) / 2.0f, 44.0f, 0.44f, Renderer::Color(200, 210, 230), statusText, false);
}

void DisplayManager::drawEmptyListUI() {
    auto& r = Renderer::getInstance();
    // Top Screen
    r.drawRect(0, 0, TOP_WIDTH, TOP_HEIGHT, Renderer::Color(20, 20, 20), true);
    r.drawText(90, 100, 0.7f, Renderer::Color(150, 150, 150), "Pokedex Empty", true);

    // Bottom Screen
    r.drawRect(0, 0, BOTTOM_WIDTH, BOTTOM_HEIGHT, Renderer::Color(30, 30, 30), false);
    r.drawText(20, 100, 0.55f, Renderer::Color(255, 255, 255), "Press Y to start analyzing Pokemon", false);
}

void DisplayManager::drawProgressBar(float x, float y, float width, float height, float progress) {
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;

    // Center Pokeball in bottom screen display area
    float cx = (x > 0 && width > 0) ? (x + width / 2.0f) : (BOTTOM_WIDTH / 2.0f);
    float cy = (y > 0 && height > 0) ? (y + height / 2.0f) : (130.0f);
    float radius = 34.0f;

    // Draw ONLY the spinning Pokeball
    drawSpinningPokeball(cx, cy, radius, progress, false);
}

void DisplayManager::drawErrorUI() {
    auto& r = Renderer::getInstance();
    r.drawRect(0, 0, BOTTOM_WIDTH, BOTTOM_HEIGHT, Renderer::Color(50, 0, 0), false);
    r.drawText(95, 80, 0.8f, Renderer::Color(255, 0, 0), "ERROR PAGE!", false);
    r.drawText(90, 180, 0.55f, Renderer::Color(255, 255, 255), "(B) Back to List", false);
}

void DisplayManager::drawButtonPrompts(AppState state) {
    auto& r = Renderer::getInstance();
    
    // Only draw the persistent bar for standard navigation states
    if (state != AppState::LIST_VIEW && state != AppState::DETAIL_VIEW && state != AppState::SEARCH_MODE) {
        return;
    }

    // Stylish Bar Design
    r.drawRect(0, 218, BOTTOM_WIDTH, 2, Renderer::Color(100, 100, 100), false); // Separator line
    r.drawRect(0, 220, BOTTOM_WIDTH, 20, Renderer::Color(20, 20, 35, 220), false); // Dark translucent background
    
    float scale = 0.42f;
    float y = 222;
    u32 textColor = Renderer::Color(255, 255, 255);
    u32 colorA = Renderer::Color(255, 80, 80);   // Red-ish
    u32 colorB = Renderer::Color(255, 255, 80);  // Yellow-ish
    u32 colorX = Renderer::Color(80, 150, 255);  // Blue-ish
    u32 colorY = Renderer::Color(80, 255, 80);   // Green-ish

    switch (state) {
        case AppState::LIST_VIEW:
            if (ApplicationState::getInstance().getPokemonCount() > 0) {
                r.drawText(10,  y, scale, colorA, "(A)", false); r.drawText(35, y, scale, textColor, "Detail", false);
                r.drawText(95,  y, scale, colorX, "(X)", false); r.drawText(120, y, scale, textColor, "Search", false);
                r.drawText(185, y, scale, colorY, "(Y)", false); r.drawText(210, y, scale, textColor, "Cam", false);
                r.drawText(255, y, scale, textColor, "(ZL)", false); r.drawText(285, y, scale, textColor, "Sort", false);
            } else {
                r.drawText(110, y, scale, colorY, "(Y)", false); r.drawText(135, y, scale, textColor, "Analyze", false);
            }
            break;
        case AppState::DETAIL_VIEW:
            r.drawText(10,  y, scale, colorB, "(B)", false); r.drawText(35, y, scale, textColor, "Back", false);
            r.drawText(95,  y, scale, colorY, "(Y)", false); r.drawText(120, y, scale, textColor, "Cam", false);
            break;
        case AppState::SEARCH_MODE:
            r.drawText(10,  y, scale, colorA, "(A)", false); r.drawText(35, y, scale, textColor, "Select", false);
            r.drawText(95,  y, scale, colorB, "(B)", false); r.drawText(120, y, scale, textColor, "Cancel", false);
            break;
        default:
            break;
    }
}

void DisplayManager::initSpriteTexture(int size) {
    if (spriteTexInitialized && currentSpriteSize == size) return;
    if (spriteTexInitialized) C3D_TexDelete(&spriteTex);

    C3D_TexInit(&spriteTex, size, size, GPU_RGBA8);
    C3D_TexSetFilter(&spriteTex, GPU_LINEAR, GPU_LINEAR);
    spriteSubTex = { (uint16_t)size, (uint16_t)size, 0.0f, 1.0f, 1.0f, 0.0f };
    spriteImage = { &spriteTex, &spriteSubTex };
    spriteTexInitialized = true;
    currentSpriteSize = size;
}

void DisplayManager::updatePokemonSprite(const std::vector<uint8_t>& tiledBytes, int size) {
    if (tiledBytes.empty()) return;
    initSpriteTexture(size);
    memcpy(spriteTex.data, tiledBytes.data(), tiledBytes.size());
    C3D_TexFlush(&spriteTex);
}

void DisplayManager::drawPokemonSprite(float x, float y, float displaySize) {
    if (!spriteTexInitialized) return;
    C2D_SceneBegin(Renderer::getInstance().getTopTarget());
    
    float scale = displaySize / (float)currentSpriteSize;
    C2D_DrawImageAt(spriteImage, x, y, 0.5f, nullptr, scale, scale);
}

void DisplayManager::drawTypeBadge(float x, float y, PokemonType type, bool is_small, bool top) {
    auto& r = Renderer::getInstance();
    u32 color = getTypeColor(type);
    
    float w = is_small ? 55.0f : 75.0f;
    float h = is_small ? 14.0f : 18.0f;
    float scale = is_small ? 0.35f : 0.4f;
    
    r.drawRect(x, y, w, h, color, top);
    r.drawRectOutline(x, y, w, h, 1.0f, Renderer::Color(255, 255, 255, 180), top);
    
    const char* typeName = type_names[static_cast<int>(type)];
    float textWidth = r.getTextWidth(typeName, scale);
    float textX = x + (w - textWidth) / 2.0f;
    float textY = y + (h - (30.0f * scale)) / 2.0f;
    
    r.drawText(textX, textY, scale, Renderer::Color(255, 255, 255), typeName, top);
}

void DisplayManager::drawPokedexCardTop(const Pokemon &pokemon, bool showSprite) {
    auto& r = Renderer::getInstance();
    
    u32 typeColor = Renderer::Color(100, 100, 100);
    if (pokemon.type_count > 0) {
        typeColor = getTypeColor(pokemon.types[0]);
    }
    
    r.drawRectOutline(10, 10, 380, 220, 2.0f, typeColor, true);
    r.drawRect(12, 12, 376, 216, Renderer::Color(25, 25, 38, 255), true);
    
    char idStr[32];
    snprintf(idStr, sizeof(idStr), "#%03d", pokemon.id);
    r.drawText(20, 20, 0.45f, Renderer::Color(160, 160, 180), idStr, true);
    
    r.drawText(20, 35, 0.75f, Renderer::Color(255, 255, 255), pokemon.name, true);
    r.drawRect(20, 65, 200, 2, Renderer::Color(60, 60, 80), true);
    r.drawText(20, 72, 0.45f, Renderer::Color(180, 186, 200), pokemon.species, true);
    
    float badgeX = 20;
    for (int i = 0; i < pokemon.type_count; i++) {
        drawTypeBadge(badgeX, 95, pokemon.types[i], false, true);
        badgeX += 85;
    }
    
    if (showSprite) {
        r.drawRectOutline(250, 45, 120, 120, 2.0f, typeColor, true);
        r.drawRect(252, 47, 116, 116, (typeColor & 0xFFFFFF00) | 0x30, true);
        
        if (spriteTexInitialized) {
            drawPokemonSprite(250, 45, 120);
        }
        
        r.drawRect(260, 175, 100, 16, Renderer::Color(20, 120, 40, 200), true);
        r.drawRectOutline(260, 175, 100, 16, 1.0f, Renderer::Color(50, 200, 80), true);
        r.drawText(273, 177, 0.35f, Renderer::Color(255, 255, 255), "DATA ACQUIRED", true);
    } else {
        float totalInches = pokemon.height * 3.93701f;
        int roundedInches = static_cast<int>(totalInches + 0.5f);
        int feet = roundedInches / 12;
        int inches = roundedInches % 12;
        float lbs = pokemon.weight * 0.220462f;
        
        char hVal[16];
        snprintf(hVal, sizeof(hVal), "%d' %02d\"", feet, inches);
        char wVal[16];
        snprintf(wVal, sizeof(wVal), "%.1f lbs", lbs);
        
        r.drawRectOutline(20, 130, 95, 60, 1.0f, Renderer::Color(60, 60, 80), true);
        r.drawRect(21, 131, 93, 58, Renderer::Color(20, 20, 30, 180), true);
        r.drawText(26, 136, 0.32f, Renderer::Color(140, 140, 160), "HEIGHT", true);
        r.drawText(26, 156, 0.48f, Renderer::Color(255, 255, 255), hVal, true);
        
        r.drawRectOutline(125, 130, 95, 60, 1.0f, Renderer::Color(60, 60, 80), true);
        r.drawRect(126, 131, 93, 58, Renderer::Color(20, 20, 30, 180), true);
        r.drawText(131, 136, 0.32f, Renderer::Color(140, 140, 160), "WEIGHT", true);
        r.drawText(131, 156, 0.48f, Renderer::Color(255, 255, 255), wVal, true);
        
        r.drawRectOutline(250, 45, 120, 120, 1.0f, Renderer::Color(50, 50, 65), true);
        r.drawRect(251, 46, 118, 118, Renderer::Color(18, 18, 26), true);
        
        r.drawRect(255, 103, 110, 4, Renderer::Color(45, 45, 60), true);
        r.drawRect(298, 93, 24, 24, Renderer::Color(18, 18, 28), true);
        r.drawRectOutline(298, 93, 24, 24, 2.0f, Renderer::Color(45, 45, 60), true);
        
        r.drawText(263, 175, 0.38f, Renderer::Color(120, 120, 140), "SELECT FOR ENTRY", true);
    }
}
