#include "main.h"

using std::string;
using enum AppState;

int main(int argc, char *argv[]) {
	// Initialize subsystems
	DisplayManager display;
	display.init();

	InputHandler input;
	ApplicationState app_state;

	TextToSpeech textToSpeech;
	ndspInit();

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
				display.drawPokemonListBottom(app_state.getPokemonList(), app_state.getPokemonCount(),
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
				display.drawPokemonListBottom(app_state.getPokemonList(), app_state.getPokemonCount(),
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

		// START button: exit application
		if (input.isStartPressed())
			break;

		if (previous_state != app_state.getCurrentState()) {
			// Clear screens
			display.clearTopScreen();
			display.clearBottomScreen();

			// Render based on current state
			switch (app_state.getCurrentState()) {
				case LIST_VIEW:
					display.drawPokemonDetailsTop(*app_state.getSelectedPokemon(), app_state.getCurrentState());
					display.drawPokemonListBottom(app_state.getPokemonList(), app_state.getPokemonCount(),
					                              app_state.getSelectedIndex());
					break;

				case DETAIL_VIEW:
					display.drawPokemonDetailsTop(*app_state.getSelectedPokemon(), app_state.getCurrentState());
					textToSpeech.sayPokemonInformation(*app_state.getSelectedPokemon());
					// textToSpeech.processText(*app_state.getSelectedPokemon());
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
			}
		}

		// Swap buffers and wait for VBlank
		display.swapBuffers();
	}

	display.exit();
	ndspExit();
	return 0;
}
