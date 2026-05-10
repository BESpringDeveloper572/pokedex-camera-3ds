#ifndef APP_STATE_H
#define APP_STATE_H

#include "pokemon_data.h"

#include <vector>
#include <algorithm>

// Application states
enum class AppState {
    LIST_VIEW,      // Browsable Pokemon list
    DETAIL_VIEW,    // Selected Pokemon details
    SEARCH_MODE,    // Search for Pokemon by name or ID
    CLASSIFYING,
    VIEWFINDER,
    ERROR
};

enum class SortMode {
    NUMERICAL,
    ALPHABETICAL
};

class ApplicationState {
public:
    ApplicationState(const AppState& state) = delete;
    ApplicationState& operator=(const AppState& state) = delete;

    static ApplicationState& getInstance() {
        static ApplicationState instance;
        return instance;
    }

    ~ApplicationState();

    // State management
    AppState getCurrentState() const;
    void setState(AppState new_state);

    // Sorting management
    SortMode getSortMode() const;
    void toggleSortMode();

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
    ApplicationState();
    void sortList();

    AppState current_state;
    SortMode current_sort_mode;
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
