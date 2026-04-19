#ifndef POKEMON_DATA_H
#define POKEMON_DATA_H

#include <cstdint>
#include <array>

// Maximum string lengths for compact storage on 3DS
constexpr uint16_t MAX_NAME_LENGTH = 20;
constexpr uint16_t MAX_DESC_LENGTH = 100;
constexpr uint8_t MAX_TYPES = 2;

// Pokemon type enumeration (18 types total)
enum class PokemonType : uint8_t {
    NORMAL = 0,
    FIRE = 1,
    WATER = 2,
    GRASS = 3,
    ELECTRIC = 4,
    ICE = 5,
    FIGHTING = 6,
    POISON = 7,
    GROUND = 8,
    FLYING = 9,
    PSYCHIC = 10,
    BUG = 11,
    ROCK = 12,
    GHOST = 13,
    DRAGON = 14,
    DARK = 15,
    STEEL = 16,
    FAIRY = 17
};

// Compact Pokemon data structure optimized for 3DS memory constraints
struct Pokemon {
    uint16_t id;                                    // 2 bytes
    char name[MAX_NAME_LENGTH];                     // 20 bytes
    std::array<PokemonType, MAX_TYPES> types;      // 2 bytes
    uint8_t type_count;                             // 1 byte (1 or 2 types)
    char description[MAX_DESC_LENGTH];              // 100 bytes
    // Total: ~125 bytes per Pokemon
};

#endif // POKEMON_DATA_H

