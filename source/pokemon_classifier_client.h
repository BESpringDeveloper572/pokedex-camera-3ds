#ifndef POKEMON_CLASSIFIER_CLIENT_H
#define POKEMON_CLASSIFIER_CLIENT_H

#include <3ds.h>
#include <string>

#include "display_manager.h"
#include "pokemon_data.h"
#include "pokemon_api.h"

class PokemonClassifierClient {
public:
    PokemonClassifierClient(const PokemonClassifierClient&) = delete;
    PokemonClassifierClient& operator=(const PokemonClassifierClient&) = delete;

    static PokemonClassifierClient& getInstance() {
        static PokemonClassifierClient instance;
        return instance;
    }

    ~PokemonClassifierClient();

    // Camera viewfinder management
    std::unique_ptr<Pokemon> identifyPokemon();

    void writePictureToFramebufferRGB24_Y2R(void *fb, void *img, u16 x, u16 y, u16 width, u16 height);

    // Old method for one-shot classification (still supported)
    bool classifyCapturedImage(Pokemon& outResult);

private:
    PokemonClassifierClient();

    // Camera state
    uint32_t camBufSize;
    uint8_t* viewfinderBuf;
    Handle camReceiveEvent;
    bool transferInProgress;

    // Helper for frame rendering
    void writeRGB565ToFramebuffer(void* fb, void* img, uint16_t width, uint16_t height, uint32_t stride);
};

#endif // POKEMON_CLASSIFIER_CLIENT_H
