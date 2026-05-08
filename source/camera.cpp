#include "camera.h"
#include <malloc.h>
#include <cstdio>
#include <cstring>

void cameraThreadFunc(void* arg) {
    Camera* cam = (Camera*)arg;
    Result res;
    Handle receiveEvent = 0;
    u32 transferBytes;

    CAMU_SetSize(SELECT_OUT1, SIZE_CTR_TOP_LCD, CONTEXT_A);
    CAMU_SetOutputFormat(SELECT_OUT1, OUTPUT_RGB_565, CONTEXT_A);
    CAMU_SetNoiseFilter(SELECT_OUT1, true);
    CAMU_SetAutoExposure(SELECT_OUT1, true);
    CAMU_SetAutoWhiteBalance(SELECT_OUT1, true);
    
    CAMU_Activate(SELECT_OUT1);
    
    CAMU_GetMaxBytes(&transferBytes, CAM_WIDTH, CAM_HEIGHT);
    CAMU_SetTransferBytes(PORT_CAM1, transferBytes, CAM_WIDTH, CAM_HEIGHT);
    
    CAMU_ClearBuffer(PORT_CAM1);
    CAMU_StartCapture(PORT_CAM1);

    while (svcWaitSynchronization(cam->stopEvent, 0) != 0) {
        res = CAMU_SetReceiving(&receiveEvent, cam->sharedBuffer, PORT_CAM1, CAM_BUF_SIZE, (s16)transferBytes);
        
        if (R_SUCCEEDED(res)) {
            if (R_SUCCEEDED(svcWaitSynchronization(receiveEvent, 500000000ULL))) {
                // Lock while the CPU is touching the shared buffer
                LightLock_Lock(&cam->lock);
                GSPGPU_InvalidateDataCache(cam->sharedBuffer, CAM_BUF_SIZE);
                cam->frameReady = true;
                LightLock_Unlock(&cam->lock);
            }
            svcCloseHandle(receiveEvent);
        }
        svcSleepThread(1000000); 
    }

    CAMU_StopCapture(PORT_CAM1);
    CAMU_Activate(SELECT_NONE);
}

Camera::Camera() : initialized(false), thread(nullptr), stopEvent(0), sharedBuffer(nullptr), frameReady(false) {
    LightLock_Init(&lock);
    LightLock_Init(&stateLock);
}

Camera::~Camera() {
    exit();
}

bool Camera::init() {
    LightLock_Lock(&stateLock);
    if (initialized) {
        LightLock_Unlock(&stateLock);
        return true;
    }

    if (R_FAILED(camInit())) {
        LightLock_Unlock(&stateLock);
        return false;
    }

    sharedBuffer = (u16*)linearAlloc(CAM_BUF_SIZE);
    if (!sharedBuffer) {
        camExit();
        LightLock_Unlock(&stateLock);
        return false;
    }

    svcCreateEvent(&stopEvent, RESET_STICKY);
    thread = threadCreate(cameraThreadFunc, this, 0x4000, 0x1F, -2, false);
    
    if (!thread) {
        linearFree(sharedBuffer);
        svcCloseHandle(stopEvent);
        camExit();
        LightLock_Unlock(&stateLock);
        return false;
    }

    initialized = true;
    LightLock_Unlock(&stateLock);
    return true;
}

void Camera::copyFrame(u16* outBuffer) {
    if (!sharedBuffer || !outBuffer) return;
    
    // Ensure the thread isn't invalidating the cache while we copy
    LightLock_Lock(&lock);
    GSPGPU_InvalidateDataCache(sharedBuffer, CAM_BUF_SIZE);
    memcpy(outBuffer, sharedBuffer, CAM_BUF_SIZE);
    LightLock_Unlock(&lock);
}

void Camera::exit() {
    LightLock_Lock(&stateLock);
    if (!initialized) {
        LightLock_Unlock(&stateLock);
        return;
    }

    if (thread) {
        svcSignalEvent(stopEvent);
        threadJoin(thread, 1000000000ULL);
        threadFree(thread);
        thread = nullptr;
    }
    
    if (stopEvent) {
        svcCloseHandle(stopEvent);
        stopEvent = 0;
    }

    if (sharedBuffer) {
        linearFree(sharedBuffer);
        sharedBuffer = nullptr;
    }

    camExit();
    initialized = false;
    frameReady = false;
    LightLock_Unlock(&stateLock);
}
