#include "main.h"
#include "display_manager.h"
#include "input_handler.h"
#include "app_state.h"
#include "pokemon_data.h"

int main(int argc, char* argv[])
{
	// Initialize subsystems
	DisplayManager display;
	display.init();

	InputHandler input;
	ApplicationState app_state;

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
	app_state.setState(AppState::LIST_VIEW);

	// Main application loop
	while (aptMainLoop())
	{
		// Update input state
		input.update();

		// Handle input based on current state
		if (input.isUpPressed()) {
			if (app_state.getCurrentState() == AppState::LIST_VIEW) {
				app_state.moveSelection(-1, app_state.getPokemonCount());
			} else if (app_state.getCurrentState() == AppState::SEARCH_MODE) {
				app_state.moveSelection(-1, app_state.getFilteredCount());
			}
		}

		if (input.isDownPressed()) {
			if (app_state.getCurrentState() == AppState::LIST_VIEW) {
				app_state.moveSelection(1, app_state.getPokemonCount());
			} else if (app_state.getCurrentState() == AppState::SEARCH_MODE) {
				app_state.moveSelection(1, app_state.getFilteredCount());
			}
		}

		// A button: select and go to detail view
		if (input.isAPressed()) {
			if (app_state.getCurrentState() == AppState::LIST_VIEW) {
				app_state.setState(AppState::DETAIL_VIEW);
			} else if (app_state.getCurrentState() == AppState::SEARCH_MODE) {
				app_state.setState(AppState::DETAIL_VIEW);
			}
		}

		// B button: back to list view
		if (input.isBPressed()) {
			if (app_state.getCurrentState() == AppState::DETAIL_VIEW) {
				app_state.setState(AppState::LIST_VIEW);
			} else if (app_state.getCurrentState() == AppState::SEARCH_MODE) {
				app_state.setState(AppState::LIST_VIEW);
			}
		}

		// X button: toggle search mode
		if (input.isKeyDown(KEY_X)) {
			if (app_state.getCurrentState() == AppState::LIST_VIEW) {
				app_state.setState(AppState::SEARCH_MODE);
				app_state.clearSearchText();
			} else if (app_state.getCurrentState() == AppState::SEARCH_MODE) {
				app_state.setState(AppState::LIST_VIEW);
			}
		}

		// START button: exit application
		if (input.isStartPressed())
			break;

		// Clear screens
		display.clearTopScreen();
		display.clearBottomScreen();

		// Render based on current state
		switch (app_state.getCurrentState()) {
			case AppState::LIST_VIEW:
				display.drawPokemonDetailsTop(*app_state.getSelectedPokemon(), app_state.getCurrentState());
				display.drawPokemonListBottom(app_state.getPokemonList(), app_state.getPokemonCount(), app_state.getSelectedIndex());
				break;

			case AppState::DETAIL_VIEW:
				display.drawPokemonDetailsTop(*app_state.getSelectedPokemon(), app_state.getCurrentState());
				display.drawPokemonListBottom(app_state.getPokemonList(), app_state.getPokemonCount(), app_state.getSelectedIndex());
				break;

			case AppState::SEARCH_MODE:
				display.drawPokemonDetailsTop(*app_state.getSelectedPokemon(), app_state.getCurrentState());
				display.drawSearchModeBottom(&app_state);
				break;
		}

		// Swap buffers and wait for VBlank
		display.swapBuffers();
	}

	display.exit();
	return 0;
}
