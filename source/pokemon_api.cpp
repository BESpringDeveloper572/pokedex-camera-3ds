//
// Created by Ivan Huynh on 5/1/26.
//

#include "pokemon_api.h"
#include <memory>
#include <cstring>
#include <cstdlib>
#include <format>
#include <vector>

#include "display_manager.h"

extern "C" {
    #include "jsmn.h"
}

PokemonApi::PokemonApi(const std::string& baseUrl, const std::string& apiKey)
    : baseUrl(baseUrl), apiKey(apiKey) {
    Result ret = httpcInit(0x400000); // 4MB buffer
    if (R_FAILED(ret)) throw std::runtime_error("Failed to initialize");
}

PokemonApi::~PokemonApi() {
    httpcExit();
}

std::unique_ptr<Pokemon>  PokemonApi::classifyImage(const uint8_t* imageData, uint32_t imageSize) {
    httpcContext context;
    std::string url = baseUrl + "/classify";

    Result ret = httpcOpenContext(&context, HTTPC_METHOD_POST, url.c_str(), 0);
    if (R_FAILED(ret)) return nullptr;

    httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);
    httpcAddRequestHeaderField(&context, "X-API-KEY", apiKey.c_str());

    const char* boundary = "----3DSPokedexBoundary";
    std::string bodyStart = "--";
    bodyStart += boundary;
    bodyStart += "\r\nContent-Disposition: form-data; name=\"file\"; filename=\"capture.raw\"\r\n";
    bodyStart += "Content-Type: application/octet-stream\r\n\r\n";

    std::string bodyEnd = "\r\n--";
    bodyEnd += boundary;
    bodyEnd += "--\r\n";

    uint32_t totalSize = bodyStart.length() + imageSize + bodyEnd.length();

    std::string contentType = "multipart/form-data; boundary=";
    contentType += boundary;
    httpcAddRequestHeaderField(&context, "Content-Type", contentType.c_str());

    uint8_t* fullBuffer = (uint8_t*)malloc(totalSize);
    if (!fullBuffer) {
        httpcCloseContext(&context);
        return nullptr;
    }

    memcpy(fullBuffer, bodyStart.c_str(), bodyStart.length());
    memcpy(fullBuffer + bodyStart.length(), imageData, imageSize);
    memcpy(fullBuffer + bodyStart.length() + imageSize, bodyEnd.c_str(), bodyEnd.length());

    ret = httpcAddPostDataRaw(&context, (u32*)fullBuffer, totalSize);
    if (R_FAILED(ret)) {
        free(fullBuffer);
        httpcCloseContext(&context);
        return nullptr;
    }

    ret = httpcBeginRequest(&context);
    free(fullBuffer);
    if (R_FAILED(ret)) {
        httpcCloseContext(&context);
        return nullptr;
    }

    u32 statuscode = 0;
    httpcGetResponseStatusCode(&context, &statuscode);
    if (statuscode != 200) {
        httpcCloseContext(&context);
        return nullptr;
    }

    u32 contentsize = 0;
    httpcGetDownloadSizeState(&context, NULL, &contentsize);

    char* responseBuf = (char*)malloc(contentsize + 1);
    if (!responseBuf) {
        httpcCloseContext(&context);
        return nullptr;
    }

    u32 readsize = 0;
    ret = httpcDownloadData(&context, (u8*)responseBuf, contentsize, &readsize);
    responseBuf[readsize] = '\0';
    std::string jsonResponse = responseBuf;

    free(responseBuf);
    httpcCloseContext(&context);

    if (R_FAILED(ret)) return nullptr;

    auto pokemon = std::make_unique<Pokemon>();
    parseApiResponse(jsonResponse, *pokemon);
    return pokemon;
}

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

std::unique_ptr<Pokemon> PokemonApi::getPokemon(const std::string &pokemonName) {
    httpcContext context;
    std::string url = baseUrl + "/pokemon/" + pokemonName;

    Result ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url.c_str(), 0);
    if (R_FAILED(ret)) return nullptr;

    httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);
    httpcAddRequestHeaderField(&context, "X-API-KEY", apiKey.c_str());

    ret = httpcBeginRequest(&context);
    if (R_FAILED(ret)) {
        printf(COLOR_RED "Failed to make request!\n" COLOR_RESET);
        httpcCloseContext(&context);
        return nullptr;
    }

    u32 statuscode = 0;
    httpcGetResponseStatusCode(&context, &statuscode);
    if (statuscode != 200) {
        printf(COLOR_RED "Request failed with %u !\n" COLOR_RESET, statuscode);
        httpcCloseContext(&context);
        return nullptr;
    }
    u32 contentsize = 0;
    httpcGetDownloadSizeState(&context, nullptr, &contentsize);

    char* responseBuf = (char*)malloc(contentsize + 1);
    if (!responseBuf) {
        httpcCloseContext(&context);
        return nullptr;
    }

    u32 readsize = 0;
    ret = httpcDownloadData(&context, (u8*)responseBuf, contentsize, &readsize);
    responseBuf[readsize] = '\0';
    std::string jsonResponse = responseBuf;

    free(responseBuf);
    httpcCloseContext(&context);

    if (R_FAILED(ret)) return nullptr;

    auto pokemon = std::make_unique<Pokemon>();
    parseApiResponse(jsonResponse, *pokemon);
    return pokemon;
}

bool PokemonApi::parseApiResponse(const std::string& json, Pokemon& outResult) {
    jsmn_parser p;
    jsmntok_t t[128]; // We expect less than 128 tokens
    jsmn_init(&p);
    int r = jsmn_parse(&p, json.c_str(), json.length(), t, sizeof(t) / sizeof(t[0]));
    if (r < 0) {
        return false;
    }

    // Assume the top-level element is an object
    if (r < 1 || t[0].type != JSMN_OBJECT) {
        return false;
    }

    memset(&outResult, 0, sizeof(Pokemon));

    for (int i = 1; i < r; i++) {
        if (jsoneq(json.c_str(), &t[i], "pokemon") == 0) {
            int len = t[i + 1].end - t[i + 1].start;
            int max_len = MAX_NAME_LENGTH - 1;
            int copy_len = len < max_len ? len : max_len;
            strncpy(outResult.name, json.c_str() + t[i + 1].start, copy_len);
            outResult.name[copy_len] = '\0';
            i++;
        } else if (jsoneq(json.c_str(), &t[i], "id") == 0) {
            std::string id_str(json.c_str() + t[i + 1].start, t[i + 1].end - t[i + 1].start);
            outResult.id = std::stoi(id_str);
            i++;
        } else if (jsoneq(json.c_str(), &t[i], "description") == 0) {
            int len = t[i + 1].end - t[i + 1].start;
            int max_len = MAX_DESC_LENGTH - 1;
            int copy_len = len < max_len ? len : max_len;
            strncpy(outResult.description, json.c_str() + t[i + 1].start, copy_len);
            outResult.description[copy_len] = '\0';
            i++;
        } else if (jsoneq(json.c_str(), &t[i], "types") == 0) {
            if (t[i + 1].type == JSMN_ARRAY) {
                int array_len = t[i + 1].size;
                int type_count = 0;
                int j = i + 2;
                while (type_count < array_len && type_count < MAX_TYPES) {
                    std::string type_name(json.c_str() + t[j].start, t[j].end - t[j].start);
                    // Match type_name with PokemonType enum
                    for (int k = 0; k < 18; k++) {
                        if (type_name == type_names[k]) {
                            outResult.types[type_count] = static_cast<PokemonType>(k);
                            type_count++;
                            break;
                        }
                    }
                    j++;
                }
                outResult.type_count = type_count;
                i = j - 1;
            }
        }
    }

    return true;
}
