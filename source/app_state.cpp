#include "app_state.h"

ApplicationState::ApplicationState()
    : current_state(AppState::LIST_VIEW),
      selected_index(0),
      pokemon_list(nullptr),
      pokemon_count(0),
      search_text_length(0),
      filtered_count(0) {
    search_text[0] = '\0';
    for (int i = 0; i < 151; i++) {
        filtered_indices[i] = 0;
    }
}

ApplicationState::~ApplicationState() = default;

AppState ApplicationState::getCurrentState() const {
    return current_state;
}

void ApplicationState::setState(AppState new_state) {
    current_state = new_state;
}

int ApplicationState::getSelectedIndex() const {
    return selected_index;
}

void ApplicationState::setSelectedIndex(int index) {
    if (index >= 0 && index < pokemon_count) {
        selected_index = index;
    }
}

void ApplicationState::moveSelection(int delta, int max_pokemon) {
    int new_index = selected_index + delta;
    if (new_index >= 0 && new_index < max_pokemon) {
        selected_index = new_index;
    }
}

void ApplicationState::setSearchText(const char* text) {
    if (text) {
        strncpy(search_text, text, MAX_NAME_LENGTH - 1);
        search_text[MAX_NAME_LENGTH - 1] = '\0';
        search_text_length = static_cast<int>(strlen(search_text));
    } else {
        clearSearchText();
    }
}

const char* ApplicationState::getSearchText() const {
    return search_text;
}

void ApplicationState::clearSearchText() {
    search_text[0] = '\0';
    search_text_length = 0;
}

void ApplicationState::addCharToSearch(char c) {
    if (search_text_length < MAX_NAME_LENGTH - 1) {
        search_text[search_text_length] = c;
        search_text[search_text_length + 1] = '\0';
        search_text_length++;
        performSearch();
    }
}

void ApplicationState::removeCharFromSearch() {
    if (search_text_length > 0) {
        search_text_length--;
        search_text[search_text_length] = '\0';
        performSearch();
    }
}

void ApplicationState::setPokemonList(const Pokemon* list, int count) {
    pokemon_list = list;
    pokemon_count = count;
    selected_index = 0;
}

const Pokemon* ApplicationState::getPokemonList() const {
    return pokemon_list;
}

int ApplicationState::getPokemonCount() const {
    return pokemon_count;
}

const Pokemon* ApplicationState::getSelectedPokemon() const {
    if (pokemon_list && selected_index >= 0 && selected_index < pokemon_count) {
        return &pokemon_list[selected_index];
    }
    return nullptr;
}

void ApplicationState::performSearch() {
    filtered_count = 0;

    if (search_text_length == 0) {
        // No search, show all Pokemon
        for (int i = 0; i < pokemon_count; i++) {
            filtered_indices[filtered_count++] = i;
        }
        return;
    }

    // Search by name (case-insensitive partial match)
    for (int i = 0; i < pokemon_count; i++) {
        const char* name = pokemon_list[i].name;

        // Simple case-insensitive partial match
        bool matches = true;
        for (int j = 0; search_text[j] != '\0' && j < search_text_length; j++) {
            char search_char = search_text[j];
            // Convert to lowercase for comparison
            if (search_char >= 'A' && search_char <= 'Z') {
                search_char = search_char - 'A' + 'a';
            }

            char name_char = name[j];
            if (name_char >= 'A' && name_char <= 'Z') {
                name_char = name_char - 'A' + 'a';
            }

            if (search_char != name_char) {
                matches = false;
                break;
            }
        }

        if (matches) {
            filtered_indices[filtered_count++] = i;
        }
    }

    // Reset selection if needed
    if (selected_index >= filtered_count) {
        selected_index = (filtered_count > 0) ? 0 : -1;
    }
}

int ApplicationState::getFilteredCount() const {
    return filtered_count;
}

const Pokemon* ApplicationState::getFilteredPokemon(int index) const {
    if (index >= 0 && index < filtered_count && pokemon_list) {
        return &pokemon_list[filtered_indices[index]];
    }
    return nullptr;
}



