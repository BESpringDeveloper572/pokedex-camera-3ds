#ifndef APP_STATE_H
#define APP_STATE_H

#include "pokemon_data.h"
#include <cstring>
#include <vector>

// Application states
enum class AppState {
    LIST_VIEW,      // Browsable Pokemon list
    DETAIL_VIEW,    // Selected Pokemon details
    SEARCH_MODE,    // Search for Pokemon by name or ID
    VIEWFINDER,     // Camera preview for capture
    CLASSIFYING     // Identifying a Pokemon via camera
};

class ApplicationState {
public:
    ApplicationState();
    ~ApplicationState();

    // State management
    AppState getCurrentState() const;
    void setState(AppState new_state);

    // Pokemon selection
    int getSelectedIndex() const;
    void setSelectedIndex(int index);
    void moveSelection(int delta, int max_pokemon);

    // Search functionality
    void setSearchText(const char* text);
    const char* getSearchText() const;
    void clearSearchText();
    void addCharToSearch(char c);
    void removeCharFromSearch();

    // Pokemon list management
    void setPokemonList(const Pokemon* list, int count);
    void addPokemon(const Pokemon& pokemon);
    const std::vector<Pokemon>& getPokemonList() const;
    int getPokemonCount() const;
    const Pokemon* getSelectedPokemon() const;

    const Pokemon *getFilteredSelectedPokemon() const;

    // Search filtering
    void performSearch();
    int getFilteredCount() const;
    const Pokemon* getFilteredPokemon(int index) const;

private:
    AppState current_state;
    int selected_index;

    std::vector<Pokemon> pokemon_list;

    char search_text[MAX_NAME_LENGTH];
    int search_text_length;

    // Filtered results for search
    int filtered_indices[151];  // Max Gen 1 Pokemon
    int filtered_count;
    const Pokemon* getPokemon(int index) const;
};

#endif // APP_STATE_H

