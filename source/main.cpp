#include "main.h"

#include <malloc.h>

#include "PokemonClassifierClient.h"
#include "secrets.h"

using std::string;
using enum AppState;

constexpr size_t SOC_ALIGN = 0x1000;
constexpr size_t SOC_BUFFERSIZE = 0x100000;

int main(int argc, char *argv[]) {
	// Initialize subsystems
	DisplayManager display;
	display.init();

	InputHandler input;
	ApplicationState app_state;

	TextToSpeech textToSpeech;
	ndspInit();

	u32 *soc_buffer = nullptr;

	soc_buffer = static_cast<u32*>(std::aligned_alloc(SOC_ALIGN, SOC_BUFFERSIZE));
	if(soc_buffer) {
		Result soc_ret = socInit(soc_buffer, SOC_BUFFERSIZE);
		if (R_FAILED(soc_ret)) {
			printf(COLOR_RED "Failed to initialize networking (SOC)! 0x%08lX\n" COLOR_RESET, soc_ret);
		}
	} else {
		printf(COLOR_RED "Failed to allocate SOC buffer!\n" COLOR_RESET);
	}

	char macStr[18];

	// Use snprintf to format each byte of the array into the string buffer
	snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
			 OS_SharedConfig->wifi_macaddr[0],
			 OS_SharedConfig->wifi_macaddr[1],
			 OS_SharedConfig->wifi_macaddr[2],
			 OS_SharedConfig->wifi_macaddr[3],
			 OS_SharedConfig->wifi_macaddr[4],
			 OS_SharedConfig->wifi_macaddr[5]);
	PokemonClassifierClient classifier(API_BASE_URL, API_KEY, macStr);
	if (!classifier.init()) {
		printf(COLOR_RED "Failed to initialize camera/network services!\n" COLOR_RESET);
	}

	SwkbdState swkbd;
	string keyboardInput;
	keyboardInput.reserve(64);

	// Create test Pokemon data
	Pokemon test_pokemon[] = {
		{
			1,
			"Bulbasaur",
			{PokemonType::GRASS, PokemonType::POISON},
			2,
			"A small quadrupedal Pokemon with a bulb on its back."
		},
		{
			4,
			"Charmander",
			{PokemonType::FIRE},
			1,
			"A small lizard Pokemon that breathes fire."
		},
		{
			7,
			"Squirtle",
			{PokemonType::WATER},
			1,
			"A small turtle Pokemon with a hard shell."
		},
		{
			25,
			"Pikachu",
			{PokemonType::ELECTRIC},
			1,
			"An electric mouse Pokemon that shoots lightning."
		}
	};

	int test_pokemon_count = 4;

	// Initialize app state with Pokemon list
	app_state.setPokemonList(test_pokemon, test_pokemon_count);
	app_state.setState(LIST_VIEW);

	bool startup = true;

	// Main application loop
	while (aptMainLoop()) {
		// Update input state
		input.update();

		AppState previous_state;
		if (startup) {
			previous_state = DETAIL_VIEW;
			startup = false;
		} else {
			previous_state = app_state.getCurrentState();
		}

		// Handle input based on current state
		if (input.isUpPressed()) {
			if (app_state.getCurrentState() == LIST_VIEW) {
				app_state.moveSelection(-1, app_state.getPokemonCount());
				display.clearTopScreen();
				display.clearBottomScreen();
				display.drawPokemonDetailsTop(*app_state.getSelectedPokemon(), app_state.getCurrentState());
				display.drawPokemonListBottom(app_state.getPokemonList().data(), app_state.getPokemonCount(),
											  app_state.getSelectedIndex());
			} else if (app_state.getCurrentState() == SEARCH_MODE) {
				app_state.moveSelection(-1, app_state.getFilteredCount());
			}
		}

		if (input.isDownPressed()) {
			if (app_state.getCurrentState() == LIST_VIEW) {
				app_state.moveSelection(1, app_state.getPokemonCount());
				display.clearTopScreen();
				display.clearBottomScreen();
				display.drawPokemonDetailsTop(*app_state.getSelectedPokemon(), app_state.getCurrentState());
				display.drawPokemonListBottom(app_state.getPokemonList().data(), app_state.getPokemonCount(),
											  app_state.getSelectedIndex());
			} else if (app_state.getCurrentState() == SEARCH_MODE) {
				app_state.moveSelection(1, app_state.getFilteredCount());
			}
		}

		// A button: select and go to detail view
		if (input.isAPressed()) {
			if (app_state.getCurrentState() == LIST_VIEW) {
				app_state.setState(DETAIL_VIEW);
			} else if (app_state.getCurrentState() == SEARCH_MODE) {
				app_state.setState(DETAIL_VIEW);
			}
		}

		// B button: back to list view
		if (input.isBPressed()) {
			if (app_state.getCurrentState() == DETAIL_VIEW) {
				app_state.setState(LIST_VIEW);
			} else if (app_state.getCurrentState() == SEARCH_MODE) {
				app_state.setState(LIST_VIEW);
			}
		}

		// X button: toggle search mode
		if (input.isKeyDown(KEY_X)) {
			if (app_state.getCurrentState() == LIST_VIEW) {
				app_state.setState(SEARCH_MODE);
				app_state.clearSearchText();
			} else if (app_state.getCurrentState() == SEARCH_MODE) {
				app_state.setState(LIST_VIEW);
			}
		}

		// Y button: classification mode
		if (input.isKeyDown(KEY_Y)) {
			if (app_state.getCurrentState() == LIST_VIEW || app_state.getCurrentState() == DETAIL_VIEW) {
				classifier.startViewfinder();
				app_state.setState(VIEWFINDER);
			}
		}

		// A button in Viewfinder: Capture
		if (input.isKeyDown(KEY_A) && app_state.getCurrentState() == VIEWFINDER) {
			app_state.setState(CLASSIFYING);
		}

		// B button in Viewfinder: Cancel
		if (input.isKeyDown(KEY_B) && app_state.getCurrentState() == VIEWFINDER) {
			classifier.stopViewfinder();
			app_state.setState(LIST_VIEW);
		}

		// START button: exit application
		if (input.isStartPressed())
			break;

		if (previous_state != app_state.getCurrentState() || app_state.getCurrentState() == CLASSIFYING || app_state.getCurrentState() == VIEWFINDER) {
			// Clear screens (only if state changed, or special handling for viewfinder)
			if (previous_state != app_state.getCurrentState() && app_state.getCurrentState() != VIEWFINDER) {
				display.clearTopScreen();
				display.clearBottomScreen();
			}

			// Render based on current state
			switch (app_state.getCurrentState()) {
				case LIST_VIEW:
					display.drawPokemonDetailsTop(*app_state.getSelectedPokemon(), app_state.getCurrentState());
					display.drawPokemonListBottom(app_state.getPokemonList().data(), app_state.getPokemonCount(),
					                              app_state.getSelectedIndex());
					break;

				case DETAIL_VIEW:
					display.drawPokemonDetailsTop(*app_state.getSelectedPokemon(), app_state.getCurrentState());
					textToSpeech.sayPokemonInformation(*app_state.getSelectedPokemon());
					break;

				case SEARCH_MODE:
					// Initialize keyboard with default type (QWERTY)
					swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, -1);
					swkbdSetHintText(&swkbd, "Enter your Pokemon name");

					// Open the keyboard and store result in mybuf
					swkbdInputText(&swkbd, keyboardInput.data(), 64);
					app_state.setSearchText(keyboardInput.data());
					app_state.performSearch();
					display.drawPokemonDetailsTop(*app_state.getFilteredSelectedPokemon(), app_state.getCurrentState());
					display.drawSearchModeBottom(&app_state);
					break;

				case VIEWFINDER:
					classifier.renderViewfinder();
					
					// Instruction on bottom screen (only draw once on state change)
					if (previous_state != VIEWFINDER) {
						consoleSelect(&bottomScreen);
						printf("\x1b[2J\x1b[10;5H" COLOR_BRIGHT_WHITE "Point at a Pokemon and press (A)" COLOR_RESET);
						printf("\x1b[12;10H" COLOR_YELLOW "Press (B) to Cancel" COLOR_RESET);
					}
					break;

				case CLASSIFYING:
					display.clearTopScreen();
					display.clearBottomScreen();
					printf("\x1b[10;10H" COLOR_YELLOW "Classifying Pokemon..." COLOR_RESET);
					display.swapBuffers(); // Show the message

					Pokemon result;
					if (classifier.captureCurrentFrame(result)) {
						classifier.stopViewfinder();
						app_state.addPokemon(result);
						app_state.setState(DETAIL_VIEW);
					} else {
						printf("\x1b[12;10H" COLOR_RED "Classification Failed!" COLOR_RESET);
						display.swapBuffers();
						svcSleepThread(2000000000ULL); // Wait 2 seconds
						app_state.setState(VIEWFINDER); // Go back to viewfinder
					}
					break;
			}
		}

		// Swap buffers and wait for VBlank
		display.swapBuffers();
	}

	display.exit();
	classifier.exit();
	socExit();
	if (soc_buffer) free(soc_buffer);
	ndspExit();
	return 0;
}
