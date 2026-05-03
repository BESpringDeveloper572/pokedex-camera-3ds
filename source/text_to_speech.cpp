//
// Created by Ivan Huynh on 4/19/26.
//

#include "text_to_speech.h"

#include <utility>

TextToSpeech::TextToSpeech() {
    voice = register_cmu_us_kal(nullptr);
}

TextToSpeech::~TextToSpeech() = default;

void TextToSpeech::sayPokemonInformation(const Pokemon &pokemon) {
    processText(pokemon.name);
    std::ostringstream oss;
    oss << type_names[static_cast<int>(std::to_underlying(pokemon.types.at(0)))];
    if (pokemon.type_count > 1) {
            oss << "and ";
            oss << type_names[static_cast<int>(std::to_underlying(pokemon.types.at(1)))];
    }
    oss << " Pokemon";
    std::string typeDescription = oss.str();
    processText(typeDescription.c_str());
    processText(pokemon.description);
}

void TextToSpeech::processText(const char* text) {
    static int channel = 0;
    int dataSize;

    fliteWave = flite_text_to_wave(text, voice);
    dataSize = fliteWave->num_samples * fliteWave->num_channels * 2;

    linearFree(samples);
    samples = (u16*)linearAlloc(dataSize);
    if (!samples) svcOutputDebugString("Linear Allocation Failed!\n", 25);
    memcpy(samples, fliteWave->samples, dataSize);

    memset(&waveBuf, 0, sizeof(ndspWaveBuf));
    waveBuf.data_vaddr = samples;
    waveBuf.nsamples = fliteWave->num_samples / fliteWave->num_channels;
    waveBuf.looping = false;
    waveBuf.status = NDSP_WBUF_FREE;

    ndspChnReset(channel);
    ndspChnSetInterp(channel, NDSP_INTERP_POLYPHASE);
    ndspChnSetRate(channel, fliteWave->sample_rate);
    ndspChnSetFormat(channel, NDSP_FORMAT_MONO_PCM16);

    float mix[12];
    memset(mix, 0, sizeof(mix));
    mix[0] = 1.0f; // Left front speaker
    mix[1] = 1.0f; // Right front speaker
    ndspChnSetMix(channel, mix);

    DSP_FlushDataCache(samples, dataSize);
    ndspChnWaveBufAdd(channel, &waveBuf);

    while (waveBuf.status != NDSP_WBUF_DONE) {
        // You MUST include these to keep the 3DS from freezing
        if (!aptMainLoop()) break; // Exit if the user closes the app

        gspWaitForVBlank(); // Sync with the screen (60fps)
    }

    linearFree(samples);
    delete_wave(fliteWave);
}

void TextToSpeech::playBeep(int frequency, int duration_ms) {
    u32 sampleRate = 32000; // Standard 3DS rate
    u32 numSamples = (sampleRate * duration_ms) / 1000;
    u32 dataSize = numSamples * sizeof(s16);

    // 1. Allocate Linear Memory (MUST be linear for hardware)
    s16* beepSamples = (s16*)linearAlloc(dataSize);
    if (!beepSamples) return;

    // 2. Generate a Square Wave
    for (u32 i = 0; i < numSamples; i++) {
        // Toggle between -0x1000 and 0x1000 based on frequency
        beepSamples[i] = ((i * frequency / sampleRate) % 2) ? 0x1000 : -0x1000;
    }

    // 3. Flush Cache (CRITICAL for physical 3DS)
    DSP_FlushDataCache(beepSamples, dataSize);

    // 4. Setup Wave Buffer
    ndspWaveBuf waveBuf;
    memset(&waveBuf, 0, sizeof(ndspWaveBuf));
    waveBuf.data_vaddr = beepSamples;
    waveBuf.nsamples = numSamples;
    waveBuf.looping = false;
    waveBuf.status = NDSP_WBUF_FREE;

    // 5. Configure Channel
    int channel = 0;
    ndspChnReset(channel);
    ndspChnSetFormat(channel, NDSP_FORMAT_MONO_PCM16);
    ndspChnSetRate(channel, sampleRate);

    // 6. Set the Volume Mix (The reason for many "silent" bugs)
    float mix[12]; // Explicitly use size 12
    memset(mix, 0, sizeof(mix));
    mix[0] = 1.0f; // Left Front
    mix[1] = 1.0f; // Right Front
    // Some 3DS models/emulators require back channels filled for stereo
    mix[2] = 1.0f; // Left Back
    mix[3] = 1.0f; // Right Back
    ndspChnSetMix(0, mix);

    // 7. Play
    ndspChnWaveBufAdd(channel, &waveBuf);

    // 8. Wait for it to finish so we don't exit too early
    while (waveBuf.status != NDSP_WBUF_DONE) {
        gspWaitForVBlank();
    }

    // Clean up
    linearFree(beepSamples);
}
