//
// Created by Ivan Huynh on 5/1/26.
//

#ifndef INC_3DS_APP_POKEMON_API_H
#define INC_3DS_APP_POKEMON_API_H


#include <string>
#include <bits/unique_ptr.h>

#include "pokemon_data.h"
#include "secrets.h"

class PokemonApi {
public:
    PokemonApi(const PokemonApi&) = delete;
    PokemonApi& operator=(const PokemonApi&) = delete;

    static PokemonApi& getInstance() {
        static PokemonApi instance(API_BASE_URL, API_KEY);
        return instance;
    }

    ~PokemonApi();

    // Perform the HTTP POST request with image data
    std::unique_ptr<Pokemon> classifyImage(const uint8_t *imageData, uint32_t imageSize);

    std::unique_ptr<Pokemon> getPokemon(const std::string &pokemonName);
private:
    PokemonApi(const std::string &baseUrl, const std::string &apiKey);
    std::string baseUrl;
    std::string apiKey;

    // JSON parser using libjson-c
    bool parseApiResponse(const std::string& json, Pokemon& outResult);
};

#endif //INC_3DS_APP_POKEMON_API_H
