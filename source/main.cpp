#include "main.h"

#include <malloc.h>
#include <memory>

#include "display_manager.h"
#include "input_handler.h"
#include "app_state.h"
#include "pokemon_api.h"
#include "text_to_speech.h"
#include "camera.h"
#include "secrets.h"

using std::string;
using enum AppState;

int main(int argc, char *argv[]) {
    auto &display = DisplayManager::getInstance();
    auto &pokemonApi = PokemonApi::getInstance();
    auto &camera = Camera::getInstance();
    auto &input = InputHandler::getInstance();
    auto &app_state = ApplicationState::getInstance();
    auto &textToSpeech = TextToSpeech::getInstance();

    SwkbdState swkbd;
    string keyboardInput;
    keyboardInput.reserve(64);

    Pokemon detail;
    u16* cameraFrame = (u16*)memalign(0x1000, CAM_BUF_SIZE);

    // Create test Pokemon data
    Pokemon test_pokemon[] = {
        {1, "Bulbasaur", {PokemonType::GRASS, PokemonType::POISON}, 2, "A small quadrupedal Pokemon with a bulb on its back."},
        {4, "Charmander", {PokemonType::FIRE}, 1, "A small lizard Pokemon that breathes fire."},
        {7, "Squirtle", {PokemonType::WATER}, 1, "A small turtle Pokemon with a hard shell."},
        {25, "Pikachu", {PokemonType::ELECTRIC}, 1, "An electric mouse Pokemon that shoots lightning."}
    };

    app_state.setPokemonList(test_pokemon, 4);
    app_state.setState(LIST_VIEW);

    bool startup = true;

    while (aptMainLoop()) {
        display.beginFrame();
        input.update();

        AppState previous_state = app_state.getCurrentState();
        if (startup) {
            previous_state = DETAIL_VIEW; // Force initial "change"
            startup = false;
        }

        // 1. Input handling
        if (input.isUpPressed()) {
            if (app_state.getCurrentState() == LIST_VIEW) {
                app_state.moveSelection(-1, app_state.getPokemonCount());
            } else if (app_state.getCurrentState() == SEARCH_MODE) {
                app_state.moveSelection(-1, app_state.getFilteredCount());
            }
        }
        if (input.isDownPressed()) {
            if (app_state.getCurrentState() == LIST_VIEW) {
                app_state.moveSelection(1, app_state.getPokemonCount());
            } else if (app_state.getCurrentState() == SEARCH_MODE) {
                app_state.moveSelection(1, app_state.getFilteredCount());
            }
        }

        if (input.isAPressed()) {
            if (app_state.getCurrentState() == LIST_VIEW || app_state.getCurrentState() == SEARCH_MODE) {
                app_state.setState(DETAIL_VIEW);
            }
        }

        if (input.isBPressed()) {
            if (app_state.getCurrentState() == DETAIL_VIEW || app_state.getCurrentState() == SEARCH_MODE || app_state.getCurrentState() == ERROR) {
                app_state.setState(LIST_VIEW);
            } else if (app_state.getCurrentState() == VIEWFINDER) {
                camera.exit();
                app_state.setState(LIST_VIEW);
            }
        }

        if (input.isKeyDown(KEY_X)) {
            if (app_state.getCurrentState() == LIST_VIEW) {
                app_state.setState(SEARCH_MODE);
                app_state.clearSearchText();
            } else if (app_state.getCurrentState() == SEARCH_MODE) {
                app_state.setState(LIST_VIEW);
            }
        }

        if (input.isKeyDown(KEY_Y) && (app_state.getCurrentState() == LIST_VIEW || app_state.getCurrentState() == DETAIL_VIEW)) {
            camera.init();
            app_state.setState(VIEWFINDER);
        }

        if (input.isStartPressed()) break;

        // 2. Persistent Viewfinder rendering & Classification Trigger
        if (app_state.getCurrentState() == VIEWFINDER) {
            if (camera.isFrameReady()) {
                camera.copyFrame(cameraFrame);
                camera.clearFrameReady();
                display.updateCameraTexture(cameraFrame);
            }
            display.drawCameraPreview();
            
            if (input.isKeyDown(KEY_R)) {
                app_state.setState(CLASSIFYING);
                display.drawClassifyingUI();
                display.swapBuffers();
                
                if (auto pokemon = pokemonApi.classifyImage((uint8_t*)cameraFrame, CAM_BUF_SIZE)) {
                    app_state.addPokemon(*pokemon);
                    app_state.setState(DETAIL_VIEW);
                } else {
                    app_state.setState(ERROR);
                }
                camera.exit();
                // We need to start a new frame after a synchronous block like this
                display.beginFrame();
            }
        }

        // 3. Main UI Rendering (Every Frame)
        bool state_changed = (previous_state != app_state.getCurrentState());
        bool should_speak = false;

        switch (app_state.getCurrentState()) {
            case LIST_VIEW:
                display.drawPokemonDetailsTop(*app_state.getSelectedPokemon(), app_state.getCurrentState());
                display.drawPokemonListBottom(app_state.getPokemonList().data(), app_state.getPokemonCount(), app_state.getSelectedIndex());
                break;

            case DETAIL_VIEW:
                if (state_changed) {
                    if (auto pokemon = pokemonApi.getPokemon(app_state.getSelectedPokemon()->name)) {
                        detail = *pokemon;
                        
                        // Fetch sprite (using a small 128x128 size)
                        auto spriteData = pokemonApi.getPokemonSprite(detail.name, 128);
                        display.updatePokemonSprite(spriteData, 128);
                        
                        should_speak = true;
                    } else {
                        app_state.setState(ERROR);
                    }
                }
                display.drawPokemonDetailsTop(detail, app_state.getCurrentState());
                display.drawPokemonSprite(250, 40, 120); // Draw at (250, 40) with 120px display size
                break;

            case SEARCH_MODE:
                if (state_changed) {
                    swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, -1);
                    swkbdSetHintText(&swkbd, "Enter your Pokemon name");
                    swkbdInputText(&swkbd, keyboardInput.data(), 64);
                    app_state.setSearchText(keyboardInput.data());
                    app_state.performSearch();
                }
                display.drawPokemonDetailsTop(*app_state.getFilteredSelectedPokemon(), app_state.getCurrentState());
                display.drawSearchModeBottom(&app_state);
                break;

            case VIEWFINDER:
                display.drawViewfinderUI();
                break;

            case CLASSIFYING:
                display.drawClassifyingUI();
                break;

            case ERROR:
                display.drawErrorUI();
                break;
        }

        display.swapBuffers();

        if (should_speak) {
            textToSpeech.sayPokemonInformation(detail);
        }
    }

    free(cameraFrame);
    return 0;
}
