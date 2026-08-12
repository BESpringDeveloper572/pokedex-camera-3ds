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
#include "json-c/json.h"
#include <curl/curl.h>
#include <malloc.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

struct CurlUserData {
    std::string* str;
    std::vector<uint8_t>* vec;
};

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    auto* data = static_cast<CurlUserData*>(userp);
    size_t totalSize = size * nmemb;
    if (data->str) {
        data->str->append(static_cast<char*>(contents), totalSize);
    } else if (data->vec) {
        data->vec->insert(data->vec->end(), static_cast<uint8_t*>(contents), static_cast<uint8_t*>(contents) + totalSize);
    }
    return totalSize;
}

static int ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    (void)clientp; (void)dltotal; (void)dlnow; (void)ultotal; (void)ulnow;
    auto& display = DisplayManager::getInstance();
    display.beginFrame();
    display.drawClassifyingUI();
    display.drawProgressBar(0, 0, 0, 0, 0.5f);
    display.swapBuffers();
    return 0;
}

static int LegacyProgressCallback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
    (void)clientp; (void)dltotal; (void)dlnow; (void)ultotal; (void)ulnow;
    auto& display = DisplayManager::getInstance();
    display.beginFrame();
    display.drawClassifyingUI();
    display.drawProgressBar(0, 0, 0, 0, 0.5f);
    display.swapBuffers();
    return 0;
}

static u32* soc_buffer = nullptr;
constexpr size_t SOC_ALIGN = 0x1000;
constexpr size_t SOC_BUFFERSIZE = 0x100000;

PokemonApi::PokemonApi(const std::string& api_hostname, const std::string& apiKey)
    : api_hostname(api_hostname), apiKey(apiKey) {
    
    // Determine if IP address
    std::string ip_part = api_hostname;
    size_t colonPos = api_hostname.find(':');
    if (colonPos != std::string::npos) {
        ip_part = api_hostname.substr(0, colonPos);
    }
    struct in_addr addr;
    is_ip_address = (inet_pton(AF_INET, ip_part.c_str(), &addr) == 1);

    // Set base URL
    api_url = (is_ip_address ? "http://" : "https://") + api_hostname;
}

PokemonApi::~PokemonApi() {
}

bool PokemonApi::initNetworking() {
    if (soc_buffer) return true;
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
    if (!curl) return nullptr;

    std::string url = api_url + "/classify";
    std::string response_string;
    CurlUserData userData = { &response_string, nullptr };

    curl_slist* resolve_list = nullptr;
    if (!is_ip_address) {
        std::string ip = resolveHost(api_hostname);
        if (!ip.empty()) {
            std::string resolve_str = api_hostname + ":443:" + ip;
            resolve_list = curl_slist_append(NULL, resolve_str.c_str());
            curl_easy_setopt(curl, CURLOPT_RESOLVE, resolve_list);
        }
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
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &userData);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, LegacyProgressCallback);

    CURLcode res = curl_easy_perform(curl);
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_mime_free(mime);
    curl_slist_free_all(headers);
    if (resolve_list) curl_slist_free_all(resolve_list);
    curl_easy_cleanup(curl);

    if (res == CURLE_OK && response_code == 200) {
        auto pokemon = std::make_unique<Pokemon>();
        if (parseApiResponse(response_string, *pokemon)) return pokemon;
    }
    return nullptr;
}

std::unique_ptr<Pokemon> PokemonApi::getPokemon(const std::string &pokemonName) {
    if (!initNetworking()) return nullptr;

    CURL* curl = curl_easy_init();
    if (!curl) return nullptr;

    std::string url = api_url + "/pokemon/" + pokemonName;
    std::string response_string;
    CurlUserData userData = { &response_string, nullptr };

    curl_slist* resolve_list = nullptr;
    if (!is_ip_address) {
        std::string ip = resolveHost(api_hostname);
        if (!ip.empty()) {
            std::string resolve_str = api_hostname + ":443:" + ip;
            resolve_list = curl_slist_append(NULL, resolve_str.c_str());
            curl_easy_setopt(curl, CURLOPT_RESOLVE, resolve_list);
        }
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("X-API-KEY: " + apiKey).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &userData);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    CURLcode res = curl_easy_perform(curl);
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_slist_free_all(headers);
    if (resolve_list) curl_slist_free_all(resolve_list);
    curl_easy_cleanup(curl);

    if (res == CURLE_OK && response_code == 200) {
        auto pokemon = std::make_unique<Pokemon>();
        if (parseApiResponse(response_string, *pokemon)) return pokemon;
    }
    return nullptr;
}

std::vector<uint8_t> PokemonApi::getPokemonSprite(const std::string &pokemonName, int size) {
    std::vector<uint8_t> buffer;
    if (!initNetworking()) return buffer;

    CURL* curl = curl_easy_init();
    if (!curl) return buffer;

    std::string url = api_url + "/pokemon/" + pokemonName + "/sprite?size=" + std::to_string(size);
    CurlUserData userData = { nullptr, &buffer };

    curl_slist* resolve_list = nullptr;
    if (!is_ip_address) {
        std::string ip = resolveHost(api_hostname);
        if (!ip.empty()) {
            std::string resolve_str = api_hostname + ":443:" + ip;
            resolve_list = curl_slist_append(NULL, resolve_str.c_str());
            curl_easy_setopt(curl, CURLOPT_RESOLVE, resolve_list);
        }
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("X-API-KEY: " + apiKey).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &userData);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    CURLcode res = curl_easy_perform(curl);
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_slist_free_all(headers);
    if (resolve_list) curl_slist_free_all(resolve_list);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || response_code != 200) {
        buffer.clear();
    }
    return buffer;
}

bool PokemonApi::parseApiResponse(const std::string& json, Pokemon& outResult) {
    json_object *root = json_tokener_parse(json.c_str());
    if (!root) return false;

    memset(&outResult, 0, sizeof(Pokemon));

    json_object *pokemon_obj;
    if (json_object_object_get_ex(root, "name", &pokemon_obj)) {
        strncpy(outResult.name, json_object_get_string(pokemon_obj), MAX_NAME_LENGTH - 1);
    }

    json_object *pron_obj;
    if (json_object_object_get_ex(root, "pronunciation", &pron_obj)) {
        strncpy(outResult.pronunciation, json_object_get_string(pron_obj), MAX_PRONUNCIATION_LENGTH - 1);
    }

    if (json_object_object_get_ex(root, "species", &pokemon_obj)) {
        strncpy(outResult.species, json_object_get_string(pokemon_obj), MAX_SPECIES_LENGTH - 1);
    }

    json_object *id_obj;
    if (json_object_object_get_ex(root, "id", &id_obj)) {
        outResult.id = json_object_get_int(id_obj);
    }

    json_object *height_obj;
    if (json_object_object_get_ex(root, "height", &height_obj)) {
        outResult.height = json_object_get_int(height_obj);
    }

    if (json_object_object_get_ex(root, "weight", &height_obj)) {
        outResult.weight = json_object_get_int(height_obj);
    }

    json_object *desc_obj;
    if (json_object_object_get_ex(root, "description", &desc_obj)) {
        strncpy(outResult.description, json_object_get_string(desc_obj), MAX_DESC_LENGTH - 1);
    }

    json_object *types_array;
    if (json_object_object_get_ex(root, "types", &types_array)) {
        int array_len = json_object_array_length(types_array);
        int type_count = 0;
        for (int i = 0; i < array_len && type_count < MAX_TYPES; i++) {
            const char *type_name = json_object_get_string(json_object_array_get_idx(types_array, i));
            for (int k = 0; k < 18; k++) {
                if (strcmp(type_name, type_names[k]) == 0) {
                    outResult.types[type_count++] = static_cast<PokemonType>(k);
                    break;
                }
            }
        }
        outResult.type_count = type_count;
    }

    json_object_put(root);
    return true;
}
