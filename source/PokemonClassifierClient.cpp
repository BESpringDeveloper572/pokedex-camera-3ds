#include "PokemonClassifierClient.h"
#include "display_manager.h"
#include <malloc.h>
#include <cstring>
#include <cstdio>
#include <cinttypes>

PokemonClassifierClient::PokemonClassifierClient(const std::string& baseUrl, const std::string& apiKey, const std::string& macAddress)
    : baseUrl(baseUrl), apiKey(apiKey), macAddress(macAddress), 
      servicesInitialized(false), viewfinderRunning(false), camBufSize(0), viewfinderBuf(nullptr),
      camReceiveEvent(0), transferInProgress(false) {}

PokemonClassifierClient::~PokemonClassifierClient() {
    exit();
}

bool PokemonClassifierClient::init() {
    if (servicesInitialized) return true;

    Result ret = httpcInit(0x400000); 
    if (R_FAILED(ret)) return false;

    ret = camInit();
    if (R_FAILED(ret)) {
        httpcExit();
        return false;
    }

    servicesInitialized = true;
    return true;
}

void PokemonClassifierClient::exit() {
    if (!servicesInitialized) return;
    stopViewfinder();
    camExit();
    httpcExit();
    servicesInitialized = false;
}

void PokemonClassifierClient::startViewfinder() {
    if (viewfinderRunning) return;

    const uint16_t width = 400;
    const uint16_t height = 240;
    
    // Use standard 400x240 RGB565 size
    camBufSize = width * height * 2;
    
    viewfinderBuf = (uint8_t*)linearAlloc(camBufSize);
    if (!viewfinderBuf) return;

    consoleSelect(&bottomScreen);
    printf("\x1b[15;0HInitializing Cam... ");

    gfxSet3D(false);

    CAMU_Activate(SELECT_IN1);
    svcSleepThread(50000000ULL); 

    CAMU_SetSize(SELECT_IN1, SIZE_CTR_TOP_LCD, CONTEXT_A);
    CAMU_SetOutputFormat(SELECT_IN1, OUTPUT_RGB_565, CONTEXT_A);
    CAMU_SetFrameRate(SELECT_IN1, FRAME_RATE_30);
    CAMU_SetNoiseFilter(SELECT_IN1, true);
    CAMU_SetAutoExposure(SELECT_IN1, true);
    CAMU_SetAutoWhiteBalance(SELECT_IN1, true);

    CAMU_SetTransferBytes(PORT_CAM1, camBufSize, width, height);
    CAMU_SynchronizeVsyncTiming(SELECT_IN1, SELECT_NONE);
    CAMU_ClearBuffer(PORT_CAM1);
    CAMU_StartCapture(PORT_CAM1);

    printf("Done!               ");
    viewfinderRunning = true;
}

void PokemonClassifierClient::stopViewfinder() {
    if (!viewfinderRunning) return;

    CAMU_StopCapture(PORT_CAM1);
    CAMU_Activate(SELECT_NONE);
    
    if (viewfinderBuf) {
        linearFree(viewfinderBuf);
        viewfinderBuf = nullptr;
    }

    viewfinderRunning = false;
}

void PokemonClassifierClient::renderViewfinder() {
    if (!viewfinderRunning) return;

    Handle event = 0;
    // 3200 bytes is safe for s16 and perfectly aligned to 4 lines
    Result res = CAMU_SetReceiving(&event, viewfinderBuf, PORT_CAM1, camBufSize, 3200);
    
    if (R_SUCCEEDED(res)) {
        // Wait for frame (500ms max)
        Result waitRes = svcWaitSynchronization(event, 500000000ULL);
        svcCloseHandle(event);
        
        if (R_SUCCEEDED(waitRes)) {
            GSPGPU_InvalidateDataCache(viewfinderBuf, camBufSize);
            
            // Sync with emulator display cycle
            gspWaitForVBlank();

            u8* fbL = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
            u8* fbR = gfxGetFramebuffer(GFX_TOP, GFX_RIGHT, NULL, NULL);
            
            // Draw identical frame to BOTH eyes to eliminate ghosting/doubling in Azahar
            if (fbL) writeRGB565ToFramebuffer(fbL, viewfinderBuf, 400, 240, 400);
            if (fbR) writeRGB565ToFramebuffer(fbR, viewfinderBuf, 400, 240, 400);
        }
    }
}

bool PokemonClassifierClient::captureCurrentFrame(Pokemon& outResult) {
    if (!viewfinderRunning || !viewfinderBuf) return false;

    std::string jsonResponse;
    bool success = sendImageToApi(viewfinderBuf, camBufSize, jsonResponse);
    if (!success) return false;
    
    return parseApiResponse(jsonResponse, outResult);
}

void PokemonClassifierClient::writeRGB565ToFramebuffer(void* fb, void* img, uint16_t width, uint16_t height, uint32_t stride) {
    u8* fb_8 = (u8*)fb;
    u16* img_16 = (u16*)img;
    
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            // Coordinate transformation for 3DS rotated screen
            int draw_y = (height - 1) - j;
            int draw_x = i;
            uint32_t v = (draw_y + draw_x * height) * 3;
            
            uint16_t data = img_16[j * stride + i];
            
            // RGB565 to BGR888 with proper bit extension
            uint8_t r = (data >> 11) & 0x1F;
            uint8_t g = (data >> 5) & 0x3F;
            uint8_t b = data & 0x1F;
            
            fb_8[v]   = (b << 3) | (b >> 2); // Blue
            fb_8[v+1] = (g << 2) | (g >> 4); // Green
            fb_8[v+2] = (r << 3) | (r >> 2); // Red
        }
    }
}

bool PokemonClassifierClient::sendImageToApi(const uint8_t* imageData, uint32_t imageSize, std::string& outJsonResponse) {
    httpcContext context;
    std::string url = baseUrl + "/classify";
    
    Result ret = httpcOpenContext(&context, HTTPC_METHOD_POST, url.c_str(), 0);
    if (R_FAILED(ret)) return false;

    httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);
    httpcAddRequestHeaderField(&context, "X-API-KEY", apiKey.c_str());
    httpcAddRequestHeaderField(&context, "X-3DS-MAC", macAddress.c_str());
    
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
        return false;
    }

    memcpy(fullBuffer, bodyStart.c_str(), bodyStart.length());
    memcpy(fullBuffer + bodyStart.length(), imageData, imageSize);
    memcpy(fullBuffer + bodyStart.length() + imageSize, bodyEnd.c_str(), bodyEnd.length());

    ret = httpcAddPostDataRaw(&context, (u32*)fullBuffer, totalSize);
    if (R_FAILED(ret)) {
        free(fullBuffer);
        httpcCloseContext(&context);
        return false;
    }

    ret = httpcBeginRequest(&context);
    free(fullBuffer);
    if (R_FAILED(ret)) {
        httpcCloseContext(&context);
        return false;
    }

    u32 statuscode = 0;
    httpcGetResponseStatusCode(&context, &statuscode);
    if (statuscode != 200) {
        httpcCloseContext(&context);
        return false;
    }

    u32 contentsize = 0;
    httpcGetDownloadSizeState(&context, NULL, &contentsize);
    
    char* responseBuf = (char*)malloc(contentsize + 1);
    if (!responseBuf) {
        httpcCloseContext(&context);
        return false;
    }

    u32 readsize = 0;
    ret = httpcDownloadData(&context, (u8*)responseBuf, contentsize, &readsize);
    responseBuf[readsize] = '\0';
    outJsonResponse = responseBuf;

    free(responseBuf);
    httpcCloseContext(&context);
    return R_SUCCEEDED(ret);
}

std::string PokemonClassifierClient_azahar_v3_getJsonValue(const std::string& json, const std::string& key) {
    size_t keyPos = json.find("\"" + key + "\"");
    if (keyPos == std::string::npos) return "";

    size_t colonPos = json.find(":", keyPos);
    if (colonPos == std::string::npos) return "";

    size_t startPos = json.find_first_of("\"0123456789", colonPos);
    if (startPos == std::string::npos) return "";

    if (json[startPos] == '\"') {
        startPos++;
        size_t endPos = json.find("\"", startPos);
        return json.substr(startPos, endPos - startPos);
    } else {
        size_t endPos = json.find_first_of(",}", startPos);
        return json.substr(startPos, endPos - startPos);
    }
}

bool PokemonClassifierClient::parseApiResponse(const std::string& json, Pokemon& outResult) {
    std::string name = PokemonClassifierClient_azahar_v3_getJsonValue(json, "pokemon");
    if (name.empty() || name == "Unknown") return false;

    memset(&outResult, 0, sizeof(Pokemon));
    strncpy(outResult.name, name.c_str(), MAX_NAME_LENGTH - 1);
    
    std::string idStr = PokemonClassifierClient_azahar_v3_getJsonValue(json, "id");
    outResult.id = idStr.empty() ? 0 : std::stoi(idStr);

    std::string desc = PokemonClassifierClient_azahar_v3_getJsonValue(json, "description");
    strncpy(outResult.description, desc.c_str(), MAX_DESC_LENGTH - 1);

    size_t typesStart = json.find("\"types\"");
    if (typesStart != std::string::npos) {
        int typeIdx = 0;
        for (int i = 0; i < 18 && typeIdx < MAX_TYPES; i++) {
            if (json.find(type_names[i], typesStart) != std::string::npos) {
                outResult.types[typeIdx++] = static_cast<PokemonType>(i);
            }
        }
        outResult.type_count = typeIdx;
    }

    return true;
}

bool PokemonClassifierClient::classifyCapturedImage(Pokemon& outResult) {
    uint8_t* imageBuf = nullptr;
    uint32_t imageSize = 400 * 240 * 2;
    
    imageBuf = (uint8_t*)linearAlloc(imageSize);
    if (!imageBuf) return false;

    CAMU_Activate(SELECT_IN1);
    CAMU_SetSize(SELECT_IN1, SIZE_CTR_TOP_LCD, CONTEXT_A);
    CAMU_SetOutputFormat(SELECT_IN1, OUTPUT_RGB_565, CONTEXT_A);
    CAMU_SetTransferBytes(PORT_CAM1, imageSize, 400, 240);
    
    CAMU_ClearBuffer(PORT_CAM1);
    CAMU_StartCapture(PORT_CAM1);
    
    Handle event = 0;
    Result res = CAMU_SetReceiving(&event, imageBuf, PORT_CAM1, imageSize, 3200);
    if (R_SUCCEEDED(res)) {
        svcWaitSynchronization(event, 1000000000ULL);
        svcCloseHandle(event);
    }
    
    CAMU_StopCapture(PORT_CAM1);
    CAMU_Activate(SELECT_NONE);

    std::string jsonResponse;
    bool success = sendImageToApi(imageBuf, imageSize, jsonResponse);
    linearFree(imageBuf);

    if (!success) return false;
    return parseApiResponse(jsonResponse, outResult);
}