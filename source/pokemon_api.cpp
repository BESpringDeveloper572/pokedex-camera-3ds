//
// Created by Ivan Huynh on 5/1/26.
//

#include "pokemon_api.h"

#include <memory>
#include <cstring>
#include <cstdlib>
#include <format>
#include <malloc.h>
#include <stdexcept>
#include <3ds.h>

#include "json-c/json.h"
#include <curl/curl.h>

#include "display_manager.h"

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

static u32* soc_buffer = nullptr;
constexpr size_t SOC_ALIGN = 0x1000;
constexpr size_t SOC_BUFFERSIZE = 0x100000;

PokemonApi::PokemonApi(const std::string& baseUrl, const std::string& apiKey)
    : baseUrl(baseUrl), apiKey(apiKey) {
    
    soc_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
    if (!soc_buffer) throw std::runtime_error("Failed to allocate SOC buffer");

    Result ret = socInit(soc_buffer, SOC_BUFFERSIZE);
    if (R_FAILED(ret)) {
        free(soc_buffer);
        throw std::runtime_error("socInit failed");
    }

    curl_global_init(CURL_GLOBAL_ALL);
}

PokemonApi::~PokemonApi() {
    curl_global_cleanup();
    socExit();
    free(soc_buffer);
}

std::unique_ptr<Pokemon> PokemonApi::classifyImage(const uint8_t* imageData, uint32_t imageSize) {
    CURL* curl = curl_easy_init();
    if (!curl) return nullptr;

    std::string url = baseUrl + "/classify";
    std::string response_string;

    curl_mime* mime = curl_mime_init(curl);
    curl_mimepart* part = curl_mime_addpart(mime);
    curl_mime_name(part, "file");
    curl_mime_data(part, (const char*)imageData, imageSize);
    curl_mime_filename(part, "capture.raw");
    curl_mime_type(part, "application/octet-stream");

    struct curl_slist* headers = nullptr;
    std::string apiHeader = "X-API-KEY: " + apiKey;
    headers = curl_slist_append(headers, apiHeader.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    CURLcode res = curl_easy_perform(curl);
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_mime_free(mime);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || response_code != 200) {
        return nullptr;
    }

    auto pokemon = std::make_unique<Pokemon>();
    if (parseApiResponse(response_string, *pokemon)) {
        return pokemon;
    }
    return nullptr;
}

std::unique_ptr<Pokemon> PokemonApi::getPokemon(const std::string &pokemonName) {
    CURL* curl = curl_easy_init();
    if (!curl) return nullptr;

    std::string url = baseUrl + "/pokemon/" + pokemonName;
    std::string response_string;

    struct curl_slist* headers = nullptr;
    std::string apiHeader = "X-API-KEY: " + apiKey;
    headers = curl_slist_append(headers, apiHeader.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    CURLcode res = curl_easy_perform(curl);
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || response_code != 200) {
        return nullptr;
    }

    auto pokemon = std::make_unique<Pokemon>();
    if (parseApiResponse(response_string, *pokemon)) {
        return pokemon;
    }
    return nullptr;
}

bool PokemonApi::parseApiResponse(const std::string& json, Pokemon& outResult) {
    json_object *root = json_tokener_parse(json.c_str());
    if (!root) {
        return false;
    }

    memset(&outResult, 0, sizeof(Pokemon));

    json_object *pokemon_obj;
    if (json_object_object_get_ex(root, "pokemon", &pokemon_obj)) {
        strncpy(outResult.name, json_object_get_string(pokemon_obj), MAX_NAME_LENGTH - 1);
        outResult.name[MAX_NAME_LENGTH - 1] = '\0';
    }

    json_object *id_obj;
    if (json_object_object_get_ex(root, "id", &id_obj)) {
        outResult.id = json_object_get_int(id_obj);
    }

    json_object *desc_obj;
    if (json_object_object_get_ex(root, "description", &desc_obj)) {
        strncpy(outResult.description, json_object_get_string(desc_obj), MAX_DESC_LENGTH - 1);
        outResult.description[MAX_DESC_LENGTH - 1] = '\0';
    }

    json_object *types_array;
    if (json_object_object_get_ex(root, "types", &types_array) && json_object_get_type(types_array) == json_type_array) {
        int array_len = json_object_array_length(types_array);
        int type_count = 0;
        for (int i = 0; i < array_len && type_count < MAX_TYPES; i++) {
            json_object *type_obj = json_object_array_get_idx(types_array, i);
            const char *type_name = json_object_get_string(type_obj);
            for (int k = 0; k < 18; k++) {
                if (strcmp(type_name, type_names[k]) == 0) {
                    outResult.types[type_count] = static_cast<PokemonType>(k);
                    type_count++;
                    break;
                }
            }
        }
        outResult.type_count = type_count;
    }

    json_object_put(root);
    return true;
}
