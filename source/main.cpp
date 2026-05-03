#include "main.h"

#include <cstdio>
#include <cstdlib>
#include <malloc.h>
#include <memory>
#include <string>
#include <3ds.h>

#include "app_state.h"
#include "camera.h"
#include "display_manager.h"
#include "input_handler.h"
#include "pokemon_api.h"
#include "pokemon_data.h"
#include "text_to_speech.h"

using std::string;
using enum AppState;

int main(int argc, char *argv[]) {
    auto &applicationState = ApplicationState::getInstance();
    auto &input = InputHandler::getInstance();
    auto &display = DisplayManager::getInstance();
    auto &pokemonApi = PokemonApi::getInstance();
    auto &camera = Camera::getInstance();
    auto &textToSpeech = TextToSpeech::getInstance();

    SwkbdState swkbd;
    string keyboardInput;
    keyboardInput.reserve(64);

    u8* cameraFrame = (u8*)memalign(0x1000, 400 * 240 * 3);

    // Create test Pokemon data
    Pokemon test_pokemon[] = {
        {1, "Bulbasaur", {PokemonType::GRASS, PokemonType::POISON}, 2, "A small quadrupedal Pokemon with a bulb on its back."},
        {4, "Charmander", {PokemonType::FIRE}, 1, "A small lizard Pokemon that breathes fire."},
        {7, "Squirtle", {PokemonType::WATER}, 1, "A small turtle Pokemon with a hard shell."},
        {25, "Pikachu", {PokemonType::ELECTRIC}, 1, "An electric mouse Pokemon that shoots lightning."}
    };

    applicationState.setPokemonList(test_pokemon, 4);
    applicationState.setState(LIST_VIEW);

    bool startup = true;
    bool listRedraw = true;
    Pokemon detail;

    while (aptMainLoop()) {
        input.update();

        AppState previous_state;
        if (startup) {
            previous_state = DETAIL_VIEW;
            startup = false;
        } else {
            previous_state = applicationState.getCurrentState();
        }

        // Input handling
        if (input.isUpPressed()) {
            if (applicationState.getCurrentState() == LIST_VIEW) {
                applicationState.moveSelection(-1, applicationState.getPokemonCount());
                listRedraw = true;
            } else if (applicationState.getCurrentState() == SEARCH_MODE) {
                applicationState.moveSelection(-1, applicationState.getFilteredCount());
            }
        }
        if (input.isDownPressed()) {
            if (applicationState.getCurrentState() == LIST_VIEW) {
                applicationState.moveSelection(1, applicationState.getPokemonCount());
                listRedraw = true;
            } else if (applicationState.getCurrentState() == SEARCH_MODE) {
                applicationState.moveSelection(1, applicationState.getFilteredCount());
            }
        }

        if (input.isAPressed() && (applicationState.getCurrentState() == LIST_VIEW || applicationState.getCurrentState() == SEARCH_MODE)) {
            applicationState.setState(DETAIL_VIEW);
        }

        if (input.isBPressed()) {
            if (applicationState.getCurrentState() == DETAIL_VIEW || applicationState.getCurrentState() == SEARCH_MODE || applicationState.getCurrentState() == ERROR) {
                applicationState.setState(LIST_VIEW);
            } else if (applicationState.getCurrentState() == VIEWFINDER) {
                camera.stopCapture();
                applicationState.setState(LIST_VIEW);
            }
        }

        if (input.isKeyDown(KEY_X)) {
            if (applicationState.getCurrentState() == LIST_VIEW) {
                applicationState.setState(SEARCH_MODE);
                applicationState.clearSearchText();
            } else if (applicationState.getCurrentState() == SEARCH_MODE) {
                applicationState.setState(LIST_VIEW);
            }
        }

        if (input.isKeyDown(KEY_Y) && (applicationState.getCurrentState() == LIST_VIEW || applicationState.getCurrentState() == DETAIL_VIEW)) {
            applicationState.setState(VIEWFINDER);
            camera.startCapture();
        }

        if (input.isStartPressed()) break;

        // State changes and rendering
        if (previous_state != applicationState.getCurrentState() || listRedraw) {
            display.clearTopScreen();
            display.clearBottomScreen();

            switch (applicationState.getCurrentState()) {
                case LIST_VIEW:
                    if (listRedraw) {
                        display.drawPokemonDetailsTop(*applicationState.getSelectedPokemon(), applicationState.getCurrentState());
                        display.drawPokemonListBottom(applicationState.getPokemonList().data(), applicationState.getPokemonCount(), applicationState.getSelectedIndex());
                        listRedraw = false;
                    }
                    break;
                case DETAIL_VIEW: {
                    if (auto pokemon = pokemonApi.getPokemon(applicationState.getSelectedPokemon()->name)) {
                        detail = *pokemon;
                        display.drawPokemonDetailsTop(detail, applicationState.getCurrentState());
                        textToSpeech.sayPokemonInformation(detail);
                    } else {
                        applicationState.setState(ERROR);
                    }
                    break;
                }
                case SEARCH_MODE:
                    swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, -1);
                    swkbdSetHintText(&swkbd, "Enter your Pokemon name");
                    swkbdInputText(&swkbd, keyboardInput.data(), 64);
                    applicationState.setSearchText(keyboardInput.data());
                    applicationState.performSearch();
                    display.drawPokemonDetailsTop(*applicationState.getFilteredSelectedPokemon(), applicationState.getCurrentState());
                    display.drawSearchModeBottom(&applicationState);
                    break;
                case VIEWFINDER:
                    printf("\x1b[2J\x1b[10;5H" COLOR_BRIGHT_WHITE "Point at a Pokemon and press (R)" COLOR_RESET);
                    printf("\x1b[12;10H" COLOR_YELLOW "Press (B) to Cancel" COLOR_RESET);
                    break;
                case CLASSIFYING:
                    display.clearBottomScreen();
                    printf("\x1b[10;10H" COLOR_YELLOW "Classifying Pokemon..." COLOR_RESET);
                    break;
                case ERROR:
                    printf(COLOR_RED "ERROR PAGE!\n" COLOR_RESET);
                    break;
            }
        }

        // Special rendering for Viewfinder
        if (applicationState.getCurrentState() == VIEWFINDER) {
            camera.getLatestFrame(cameraFrame);
            u8 *fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, nullptr, nullptr);
            camera.writePictureToFramebufferRGB24_Y2R(fb, cameraFrame, 0, 0, 400, 240);
            
            if (input.isKeyDown(KEY_R)) {
                applicationState.setState(CLASSIFYING);
                display.clearBottomScreen();
                printf("\x1b[10;10H" COLOR_YELLOW "Classifying Pokemon..." COLOR_RESET);
                display.swapBuffers();
                
                // For classification, we need the frame in a format the API expects
                // Assuming it expects the RGB24 or similar. Using the captured frame.
                if (auto pokemon = pokemonApi.classifyImage(cameraFrame, 400 * 240 * 3)) {
                    applicationState.addPokemon(*pokemon);
                    applicationState.setState(DETAIL_VIEW);
                } else {
                    applicationState.setState(ERROR);
                }
                camera.stopCapture();
            }
        }

        display.swapBuffers();
    }

    free(cameraFrame);
    return 0;
}
