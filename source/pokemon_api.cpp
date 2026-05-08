//
// Created by Ivan Huynh on 5/1/26.
//

#include "pokemon_api.h"
#include <memory>
#include <cstring>
#include <cstdlib>
#include <format>
#include <vector>
#include <stdexcept>

#include "display_manager.h"
#include "json-c/json.h"
#include <curl/curl.h>
#include <malloc.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

static u32* soc_buffer = nullptr;
constexpr size_t SOC_ALIGN = 0x1000;
constexpr size_t SOC_BUFFERSIZE = 0x100000;

PokemonApi::PokemonApi(const std::string& api_hostname, const std::string& apiKey)
    : api_hostname(api_hostname), apiKey(apiKey) {
}

PokemonApi::~PokemonApi() {
}

bool PokemonApi::initNetworking() {
    soc_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
    if (!soc_buffer) return false;

    Result ret = socInit(soc_buffer, SOC_BUFFERSIZE);
    if (R_FAILED(ret)) {
        free(soc_buffer);
        soc_buffer = nullptr;
        return false;
    }

    curl_global_init(CURL_GLOBAL_ALL);
    return true;
}

void PokemonApi::deinitNetworking() {
    curl_global_cleanup();
    socExit();
    if (soc_buffer) {
        free(soc_buffer);
        soc_buffer = nullptr;
    }
}

std::string resolveHost(const std::string& host) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host.c_str(), NULL, &hints, &res) != 0) {
        return "";
    }

    char ip[INET_ADDRSTRLEN];
    sockaddr_in* addr = (struct sockaddr_in*)res->ai_addr;
    inet_ntop(AF_INET, &addr->sin_addr, ip, INET_ADDRSTRLEN);
    
    freeaddrinfo(res);
    return std::string(ip);
}

std::unique_ptr<Pokemon> PokemonApi::classifyImage(const uint8_t* imageData, uint32_t imageSize) {
    if (!initNetworking()) return nullptr;

    CURL* curl = curl_easy_init();
    if (!curl) {
        deinitNetworking();
        return nullptr;
    }

    std::string url = "https://" + api_hostname + "/classify";
    std::string response_string;

    std::string ip = resolveHost(api_hostname);
    curl_slist* resolve_list = nullptr;
    if (!ip.empty()) {
        std::string resolve_str = api_hostname + ":443:" + ip;
        resolve_list = curl_slist_append(NULL, resolve_str.c_str());
        curl_easy_setopt(curl, CURLOPT_RESOLVE, resolve_list);
    }

    curl_mime* mime = curl_mime_init(curl);
    curl_mimepart* part = curl_mime_addpart(mime);
    curl_mime_name(part, "file");
    curl_mime_data(part, (const char*)imageData, imageSize);
    curl_mime_filename(part, "capture.raw");
    curl_mime_type(part, "application/octet-stream");

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("X-API-KEY: " + apiKey).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
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
    if (resolve_list) curl_slist_free_all(resolve_list);
    curl_easy_cleanup(curl);

    std::unique_ptr<Pokemon> pokemon = nullptr;
    if (res == CURLE_OK && response_code == 200) {
        pokemon = std::make_unique<Pokemon>();
        if (!parseApiResponse(response_string, *pokemon)) {
            pokemon = nullptr;
        }
    }

    deinitNetworking();
    return pokemon;
}

std::unique_ptr<Pokemon> PokemonApi::getPokemon(const std::string &pokemonName) {
    if (!initNetworking()) return nullptr;

    CURL* curl = curl_easy_init();
    if (!curl) {
        deinitNetworking();
        return nullptr;
    }

    std::string url = "https://" + api_hostname + "/pokemon/" + pokemonName;
    std::string response_string;

    std::string ip = resolveHost(api_hostname);
    struct curl_slist* resolve_list = nullptr;
    if (!ip.empty()) {
        std::string resolve_str = api_hostname + ":443:" + ip;
        resolve_list = curl_slist_append(NULL, resolve_str.c_str());
        curl_easy_setopt(curl, CURLOPT_RESOLVE, resolve_list);
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("X-API-KEY: " + apiKey).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    CURLcode res = curl_easy_perform(curl);
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_slist_free_all(headers);
    if (resolve_list) curl_slist_free_all(resolve_list);
    curl_easy_cleanup(curl);

    std::unique_ptr<Pokemon> pokemon = nullptr;
    if (res == CURLE_OK && response_code == 200) {
        pokemon = std::make_unique<Pokemon>();
        if (!parseApiResponse(response_string, *pokemon)) {
            pokemon = nullptr;
        }
    }

    deinitNetworking();
    return pokemon;
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
