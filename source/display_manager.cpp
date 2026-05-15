#include "display_manager.h"
#include "renderer.h"
#include <cstdio>
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
    
    u32 color = Renderer::Color(100, 100, 100);
    if (pokemon.type_count > 0) {
        color = getTypeColor(pokemon.types[0]);
    }
    
    // Top Screen UI
    r.drawRect(0, 0, TOP_WIDTH, 40, color, true);
    char idName[64];
    snprintf(idName, sizeof(idName), "#%03d %s", pokemon.id, pokemon.name);
    r.drawText(10, 10, 0.6f, Renderer::Color(255, 255, 255), idName, true);
    
    r.drawRect(0, 0, BOTTOM_WIDTH, BOTTOM_HEIGHT, Renderer::Color(30, 30, 30), false);
    r.drawText(10, 10, 0.5f, Renderer::Color(200, 200, 200), "Species:", false);
    r.drawText(80, 10, 0.5f, Renderer::Color(255, 255, 255), pokemon.species, false);
    
    r.drawText(10, 30, 0.5f, Renderer::Color(200, 200, 200), "Types:", false);
    
    float typeX = 70;
    for(int i=0; i<pokemon.type_count; i++) {
        u32 typeColor = getTypeColor(pokemon.types[i]);
        r.drawRect(typeX, 30, 80, 20, typeColor, false);
        r.drawText(typeX + 5, 32, 0.4f, Renderer::Color(255, 255, 255), type_names[static_cast<int>(pokemon.types[i])], false);
        typeX += 90;
    }
    
    r.drawText(10, 60, 0.5f, Renderer::Color(255, 255, 255), "Description:", false);
    r.drawTextWrapped(10, 80, 0.45f, 300.0f, Renderer::Color(200, 200, 200), pokemon.description, false);
}

void DisplayManager::drawPokemonListBottom(const Pokemon* pokemon_list, int list_size, int selected_index) {
    auto& r = Renderer::getInstance();
    auto& app = ApplicationState::getInstance();
    
    r.drawRect(0, 0, BOTTOM_WIDTH, BOTTOM_HEIGHT, Renderer::Color(30, 30, 30), false);
    r.drawText(10, 5, 0.6f, Renderer::Color(0, 255, 255), "--- Pokedex List ---", false);
    
    // Sort Mode Indicator
    const char* sortText = (app.getSortMode() == SortMode::NUMERICAL) ? "Sort: # ID" : "Sort: A-Z";
    r.drawText(220, 5, 0.45f, Renderer::Color(200, 200, 200), sortText, false);
    
    // Scanned Counter
    char scannedCount[32];
    snprintf(scannedCount, sizeof(scannedCount), "Scanned: %d", app.getPokemonCount());
    r.drawText(220, 22, 0.45f, Renderer::Color(0, 255, 0), scannedCount, false);
    
    for (int i = 0; i < list_size; i++) {
        float y = 30 + i * 20;
        u32 color = (i == selected_index) ? Renderer::Color(255, 255, 0) : Renderer::Color(255, 255, 255);
        char entry[64];
        snprintf(entry, sizeof(entry), "%s #%03d %s", (i == selected_index ? ">" : " "), pokemon_list[i].id, pokemon_list[i].name);
        r.drawText(10, y, 0.5f, color, entry, false);
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

void DisplayManager::drawClassifyingUI() {
    auto& r = Renderer::getInstance();
    r.drawRect(0, 0, BOTTOM_WIDTH, BOTTOM_HEIGHT, Renderer::Color(30, 30, 30), false);
    r.drawText(60, 100, 0.7f, Renderer::Color(255, 255, 0), "Classifying Pokemon...", false);
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
    auto& r = Renderer::getInstance();
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;

    // Background bar
    r.drawRect(x, y, width, height, Renderer::Color(60, 60, 60), false);
    // Filled bar
    r.drawRect(x, y, width * progress, height, Renderer::Color(0, 255, 0), false);
    // Border
    r.drawRect(x - 2, y - 2, width + 4, 2, Renderer::Color(200, 200, 200), false); // Top
    r.drawRect(x - 2, y + height, width + 4, 2, Renderer::Color(200, 200, 200), false); // Bottom
    r.drawRect(x - 2, y - 2, 2, height + 4, Renderer::Color(200, 200, 200), false); // Left
    r.drawRect(x + width, y - 2, 2, height + 4, Renderer::Color(200, 200, 200), false); // Right
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
