//
// Created by Ivan Huynh on 4/19/26.
//

#ifndef INC_3DS_APP_VOICE_H
#define INC_3DS_APP_VOICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <flite/flite.h>

// Try this exact signature; it is the most common for Flite voices
cst_voice *register_cmu_us_kal(const char *voxdir);

#ifdef __cplusplus
}
#endif

#include <cstring>
#include <format>
#include <cstdio>
#include <sstream>
#include <ranges>
#include <3ds.h>

#include "pokemon_data.h"

class TextToSpeech {
public:
    TextToSpeech(const TextToSpeech&) = delete;
    TextToSpeech& operator=(const TextToSpeech&) = delete;

    static TextToSpeech& getInstance() {
        static TextToSpeech instance;
        return instance;
    }

    ~TextToSpeech();

    void sayPokemonInformation(const Pokemon &pokemon);
    void playBeep(int frequency, int duration_ms);

private:
    TextToSpeech();

    void processText(const char* text);
    std::string fixPronunciation(const std::string& text);
    ndspWaveBuf waveBuf{};
    cst_wave *fliteWave{};
    cst_voice *voice{};
    u16 *samples = nullptr;
};

#endif //INC_3DS_APP_VOICE_H
