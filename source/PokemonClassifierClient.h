#ifndef POKEMON_CLASSIFIER_CLIENT_H
#define POKEMON_CLASSIFIER_CLIENT_H

#include <3ds.h>
#include <string>
#include "pokemon_data.h"

class PokemonClassifierClient {
public:
    PokemonClassifierClient(const std::string& baseUrl, const std::string& apiKey, const std::string& macAddress);
    ~PokemonClassifierClient();

    // Initialize network and camera services
    bool init();
    void exit();

    // Capture an image from the camera and classify it
    bool classifyCapturedImage(Pokemon& outResult);

    // Live viewfinder support
    void startViewfinder();
    void stopViewfinder();
    void renderViewfinder();
    bool captureCurrentFrame(Pokemon& outResult);

private:
    std::string baseUrl;
    std::string apiKey;
    std::string macAddress;

    bool servicesInitialized;
    bool viewfinderRunning;
    uint32_t camBufSize;
    uint8_t* viewfinderBuf;
    
    Handle camReceiveEvent;
    bool transferInProgress;

    // Internal helper to perform the HTTP POST request
    bool sendImageToApi(const uint8_t* imageData, uint32_t imageSize, std::string& outJsonResponse);
    
    // Minimal JSON parser for the specific API response
    bool parseApiResponse(const std::string& json, Pokemon& outResult);

    // Camera capture helper
    bool captureImage(uint8_t** outBuffer, uint32_t* outSize);
    
    // Display helper to write RGB565 to framebuffer
    void writeRGB565ToFramebuffer(void* fb, void* img, uint16_t width, uint16_t height, uint32_t stride);
};

#endif // POKEMON_CLASSIFIER_CLIENT_H
