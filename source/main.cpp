#include "main.h"

#include <malloc.h>

#include "pokemon_classifier_client.h"
#include "secrets.h"

using std::string;
using enum AppState;

constexpr size_t SOC_ALIGN = 0x1000;
constexpr size_t SOC_BUFFERSIZE = 0x100000;

int main(int argc, char *argv[]) {
	auto &display = DisplayManager::getInstance();
	auto &pokemonApi = PokemonApi::getInstance();
	auto &classifier = PokemonClassifierClient::getInstance();

	InputHandler input;
	ApplicationState app_state;

	TextToSpeech textToSpeech;
	ndspInit();

	SwkbdState swkbd;
	string keyboardInput;
	keyboardInput.reserve(64);

	Pokemon detail;

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
			} else if (app_state.getCurrentState() == ERROR) {
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
		if (input.isKeyDown(KEY_Y) && (app_state.getCurrentState() == LIST_VIEW || app_state.getCurrentState() ==
		                               DETAIL_VIEW)) {
			app_state.setState(CLASSIFYING);
		}

		// START button: exit application
		if (input.isStartPressed())
			break;

		if (previous_state != app_state.getCurrentState()) {
			// Clear screens (only if state changed, or special handling for viewfinder)
			if (previous_state != app_state.getCurrentState()) {
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
				case DETAIL_VIEW: {
					if (auto pokemon = pokemonApi.getPokemon(app_state.getSelectedPokemon()->name);
						pokemon == nullptr) {
						app_state.setState(ERROR);
					} else {
						detail = *pokemon;
						display.drawPokemonDetailsTop(detail, app_state.getCurrentState());
						textToSpeech.sayPokemonInformation(detail);
					}
					break;
				}
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
				case CLASSIFYING:
					classifier.identifyPokemon();
					break;
				case ERROR:
					printf(COLOR_RED "ERROR PAGE!\n" COLOR_RESET);
					break;
			}
		}
		// Swap buffers and wait for VBlank
		display.swapBuffers();
	}

	ndspExit();
	return 0;
}
