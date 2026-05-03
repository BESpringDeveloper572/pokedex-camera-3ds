#include "camera.h"
#include <cstring>
#include <cinttypes>
#include <stdexcept>
#include <malloc.h>

#include <3ds.h>

static Handle y2rEvent;
static Thread cameraViewThread;
static constexpr int STACKSIZE = 4 * 1024;

static volatile bool cameraThreadExit = false;
static u8 *cameraSend = nullptr;
static u8 *cameraRecv = nullptr;

#define WAIT_TIMEOUT 1000000000ULL
#define WIDTH 400
#define HEIGHT 240
#define SCREEN_SIZE WIDTH * HEIGHT * 2
#define BUF_SIZE SCREEN_SIZE * 2

void cameraThreadFunc(void *arg) {
    acInit();
    camInit();

    CAMU_SetSize(SELECT_OUT1_OUT2, SIZE_CTR_TOP_LCD, CONTEXT_A);
    CAMU_SetOutputFormat(SELECT_OUT1_OUT2, OUTPUT_YUV_422, CONTEXT_A);
    CAMU_SetFrameRate(SELECT_OUT1_OUT2, FRAME_RATE_30);
    CAMU_SetNoiseFilter(SELECT_OUT1_OUT2, true);
    CAMU_SetAutoExposure(SELECT_OUT1_OUT2, true);
    CAMU_SetAutoWhiteBalance(SELECT_OUT1_OUT2, true);
    CAMU_SetTrimming(PORT_CAM1, false);
    CAMU_SetTrimming(PORT_CAM2, false);

    u32 bufSize;
    CAMU_GetMaxBytes(&bufSize, WIDTH, HEIGHT);
    CAMU_SetTransferBytes(PORT_BOTH, bufSize, WIDTH, HEIGHT);

    CAMU_Activate(SELECT_OUT1_OUT2);

    Handle camReceiveEvents[2] = {0, 0};

    CAMU_ClearBuffer(PORT_BOTH);
    CAMU_SynchronizeVsyncTiming(SELECT_OUT1, SELECT_OUT2);
    CAMU_StartCapture(PORT_BOTH);

    while (!cameraThreadExit) {
        CAMU_SetReceiving(&camReceiveEvents[0], (void *) cameraSend, PORT_CAM1, SCREEN_SIZE, (s16) bufSize);
        CAMU_SetReceiving(&camReceiveEvents[1], (void *) (cameraSend + SCREEN_SIZE), PORT_CAM2, SCREEN_SIZE, (s16) bufSize);

        svcWaitSynchronization(camReceiveEvents[0], WAIT_TIMEOUT);
        svcWaitSynchronization(camReceiveEvents[1], WAIT_TIMEOUT);

        svcCloseHandle(camReceiveEvents[0]);
        svcCloseHandle(camReceiveEvents[1]);
    }

    CAMU_StopCapture(PORT_BOTH);
    CAMU_Activate(SELECT_NONE);

    camExit();
    acExit();
    threadExit(0);
}

Camera::Camera() : initialized(false) {
    Result ret = y2rInit();
    if (R_FAILED(ret)) {
        throw std::runtime_error("y2rInit failed");
    }
    Y2RU_SetTransferEndInterrupt(true);
    Y2RU_GetTransferEndEvent(&y2rEvent);
    initialized = true;
}

Camera::~Camera() {
    stopCapture();
    y2rExit();
}

void Camera::startCapture() {
    if (cameraSend) return;

    cameraThreadExit = false;
    cameraSend = (u8 *) memalign(0x1000, BUF_SIZE);
    cameraRecv = (u8 *) memalign(0x1000, (SCREEN_SIZE / 2) * 3);
    
    cameraViewThread = threadCreate(cameraThreadFunc, 0, STACKSIZE, 0x2F, 0, true);
}

void Camera::stopCapture() {
    if (!cameraSend) return;

    cameraThreadExit = true;
    threadJoin(cameraViewThread, WAIT_TIMEOUT);
    threadFree(cameraViewThread);

    free(cameraSend);
    free(cameraRecv);
    cameraSend = nullptr;
    cameraRecv = nullptr;
}

void Camera::getLatestFrame(u8* outRGB24) {
    if (!cameraSend || !cameraRecv) return;

    Y2RU_SetSendingYUYV((u8 *) &cameraSend[0], WIDTH * 2 * HEIGHT, WIDTH * 2, 0);
    Y2RU_SetReceiving((void *) cameraRecv, WIDTH * 3 * HEIGHT, 3 * WIDTH, 0);
    Y2RU_StartConversion();
    svcWaitSynchronization(y2rEvent, 1000 * 1000 * 10);

    if (outRGB24) {
        memcpy(outRGB24, cameraRecv, (SCREEN_SIZE / 2) * 3);
    }
}

void Camera::writePictureToFramebufferRGB24_Y2R(void *fb, void *img, u16 x, u16 y, u16 width, u16 height) {
    auto fb_8 = (u8 *) fb;
    auto img_8 = (u8 *) img;

    fb_8 += HEIGHT * 3 - 8 * 3;
    for (int j = 0; j < HEIGHT / 8; j++, fb_8 -= HEIGHT * WIDTH * 3 + 8 * 3)
        for (int i = 0; i < WIDTH; i++, img_8 += 3 * 8, fb_8 += HEIGHT * 3)
            memcpy(fb_8, img_8, 1 * 8 * 3);
}
