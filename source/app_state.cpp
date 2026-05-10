#include "app_state.h"

#include <string>

ApplicationState::ApplicationState()
    : current_state(AppState::LIST_VIEW),
      current_sort_mode(SortMode::NUMERICAL),
      selected_index(0),
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

SortMode ApplicationState::getSortMode() const {
    return current_sort_mode;
}

void ApplicationState::toggleSortMode() {
    // 1. Remember which Pokemon is currently selected (by ID)
    uint16_t currentId = 0;
    const Pokemon* selected = getSelectedPokemon();
    if (selected) {
        currentId = selected->id;
    }

    // 2. Toggle the mode
    if (current_sort_mode == SortMode::NUMERICAL) {
        current_sort_mode = SortMode::ALPHABETICAL;
    } else {
        current_sort_mode = SortMode::NUMERICAL;
    }

    // 3. Apply the sort
    sortList();

    // 4. Find where that Pokemon moved to in the new order
    if (currentId != 0) {
        for (int i = 0; i < (int)pokemon_list.size(); i++) {
            if (pokemon_list[i].id == currentId) {
                selected_index = i;
                break;
            }
        }
    }

    performSearch(); // Re-index search results based on new sort
}

void ApplicationState::sortList() {
    if (current_sort_mode == SortMode::NUMERICAL) {
        std::sort(pokemon_list.begin(), pokemon_list.end(), [](const Pokemon& a, const Pokemon& b) {
            return a.id < b.id;
        });
    } else {
        std::sort(pokemon_list.begin(), pokemon_list.end(), [](const Pokemon& a, const Pokemon& b) {
            return std::string(a.name) < std::string(b.name);
        });
    }
}

int ApplicationState::getSelectedIndex() const {
    return selected_index;
}

void ApplicationState::setSelectedIndex(int index) {
    if (index >= 0 && index < (int)pokemon_list.size()) {
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
    pokemon_list.clear();
    for(int i = 0; i < count; i++) {
        pokemon_list.push_back(list[i]);
    }
    sortList();
    selected_index = 0;
    performSearch();
}

void ApplicationState::addPokemon(const Pokemon& pokemon) {
    // Check if Pokemon already exists in the list (by ID)
    for (int i = 0; i < (int)pokemon_list.size(); i++) {
        if (pokemon_list[i].id == pokemon.id) {
            // Already exists, select it
            selected_index = i;
            // Note: Since index is position in list, and list is sorted, 
            // the ID might have moved if we re-sort.
            // But if it already existed, it's already in the right sorted spot.
            return;
        }
    }
    
    // Doesn't exist, add it
    pokemon_list.push_back(pokemon);
    sortList(); // Keep the list sorted
    
    // Find the new index of the added Pokemon
    for (int i = 0; i < (int)pokemon_list.size(); i++) {
        if (pokemon_list[i].id == pokemon.id) {
            selected_index = i;
            break;
        }
    }
    performSearch();
}

const std::vector<Pokemon>& ApplicationState::getPokemonList() const {
    return pokemon_list;
}

int ApplicationState::getPokemonCount() const {
    return (int)pokemon_list.size();
}

const Pokemon* ApplicationState::getSelectedPokemon() const {
    return getPokemon(selected_index);
}

const Pokemon* ApplicationState::getFilteredSelectedPokemon() const {
    if (selected_index >= getFilteredCount() || selected_index < 0) {
        return nullptr;
    }
    return getPokemon(filtered_indices[selected_index]);
}

const Pokemon* ApplicationState::getPokemon(int index) const {
    if (index >= 0 && index < (int)pokemon_list.size()) {
        return &pokemon_list[index];
    }
    return nullptr;
}

void ApplicationState::performSearch() {
    // 1. Remember the ID of the currently selected Pokemon
    uint16_t currentId = 0;
    const Pokemon* selected = getFilteredSelectedPokemon();
    if (selected) {
        currentId = selected->id;
    }

    filtered_count = 0;
    int count = (int)pokemon_list.size();

    if (search_text_length == 0) {
        // No search, show all Pokemon
        for (int i = 0; i < count; i++) {
            filtered_indices[filtered_count++] = i;
        }
    } else {
        // Search by name (case-insensitive partial match)
        for (int i = 0; i < count; i++) {
            const char* name = pokemon_list[i].name;
            bool matches = true;
            for (int j = 0; search_text[j] != '\0' && j < search_text_length; j++) {
                char search_char = (search_text[j] >= 'A' && search_text[j] <= 'Z') ? search_text[j] - 'A' + 'a' : search_text[j];
                char name_char = (name[j] >= 'A' && name_char <= 'Z') ? name[j] - 'A' + 'a' : name[j];
                
                if (search_char != name_char) {
                    matches = false;
                    break;
                }
            }

            if (matches) {
                filtered_indices[filtered_count++] = i;
            }
        }
    }

    // 2. Try to find the previously selected Pokemon in the new filtered list
    bool found = false;
    if (currentId != 0) {
        for (int i = 0; i < filtered_count; i++) {
            if (pokemon_list[filtered_indices[i]].id == currentId) {
                selected_index = i;
                found = true;
                break;
            }
        }
    }

    // 3. If not found or wasn't selected, default to the top of the list
    if (!found) {
        selected_index = (filtered_count > 0) ? 0 : -1;
    }
}

int ApplicationState::getFilteredCount() const {
    return filtered_count;
}

const Pokemon* ApplicationState::getFilteredPokemon(int index) const {
    if (index >= 0 && index < filtered_count) {
        return &pokemon_list[filtered_indices[index]];
    }
    return nullptr;
}
