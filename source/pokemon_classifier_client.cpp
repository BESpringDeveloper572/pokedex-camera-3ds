#include "pokemon_classifier_client.h"
#include "display_manager.h"
#include <cstring>
#include <cstdio>
#include <cinttypes>

#include <3ds.h>

Handle y2rEvent;

#define STACKSIZE (4 * 1024)

volatile bool cameraThreadExit = false;
volatile u8 *cameraSend, *cameraRecv;

#define WAIT_TIMEOUT 1000000000ULL
#define WIDTH 400
#define HEIGHT 240
#define SCREEN_SIZE WIDTH * HEIGHT * 2
#define BUF_SIZE SCREEN_SIZE * 2

PokemonClassifierClient::PokemonClassifierClient() {
    Result ret = y2rInit();
    if (R_FAILED(ret)) {
        throw std::runtime_error("y2rInit failed");
    }
}

PokemonClassifierClient::~PokemonClassifierClient() {
    y2rExit();
}

void cameraThread(void* arg) {
    acInit();

    gfxSetDoubleBuffering(GFX_TOP, true);
    gfxSetDoubleBuffering(GFX_BOTTOM, false);

    camInit();

    if(!cameraSend) {
        printf("Failed to allocate memory!");
        threadExit(-1);
    }

    u32 bufSize;

    Handle camReceiveEvent = 0;
    Handle camReceiveEvent2 = 0;

    while(!cameraThreadExit) {
        CAMU_SetReceiving(&camReceiveEvent, (void*) cameraSend, PORT_CAM1, SCREEN_SIZE, (s16) bufSize);
        CAMU_SetReceiving(&camReceiveEvent2, (void*)(cameraSend + SCREEN_SIZE), PORT_CAM2, SCREEN_SIZE, (s16) bufSize);

        svcWaitSynchronization(camReceiveEvent, WAIT_TIMEOUT);
        svcWaitSynchronization(camReceiveEvent2, WAIT_TIMEOUT);

        svcCloseHandle(camReceiveEvent);
        svcCloseHandle(camReceiveEvent2);
    }

    CAMU_StopCapture(PORT_BOTH);
    CAMU_Activate(SELECT_NONE);

    free((void*) cameraSend);
    camExit();
    acExit();
    threadExit(0);
}

// TODO: Figure out how to use CAMU_GetStereoCameraCalibrationData
void takePicture3D(u8 *buf) {
    u32 bufSize;
    CAMU_GetMaxBytes(&bufSize, WIDTH, HEIGHT);
    CAMU_SetTransferBytes(PORT_BOTH, bufSize, WIDTH, HEIGHT);
    CAMU_Activate(SELECT_OUT1_OUT2);

    Handle camReceiveEvent = 0;
    Handle camReceiveEvent2 = 0;

    CAMU_ClearBuffer(PORT_BOTH);
    CAMU_SynchronizeVsyncTiming(SELECT_OUT1, SELECT_OUT2);

    CAMU_StartCapture(PORT_BOTH);

    CAMU_SetReceiving(&camReceiveEvent, buf, PORT_CAM1, SCREEN_SIZE, (s16) bufSize);
    CAMU_SetReceiving(&camReceiveEvent2, buf + SCREEN_SIZE, PORT_CAM2, SCREEN_SIZE, (s16) bufSize);
    svcWaitSynchronization(camReceiveEvent, WAIT_TIMEOUT);
    svcWaitSynchronization(camReceiveEvent2, WAIT_TIMEOUT);
    CAMU_PlayShutterSound(SHUTTER_SOUND_TYPE_NORMAL);

    CAMU_StopCapture(PORT_BOTH);

    svcCloseHandle(camReceiveEvent);
    svcCloseHandle(camReceiveEvent2);

    CAMU_Activate(SELECT_NONE);
}

std::unique_ptr<Pokemon> PokemonClassifierClient::identifyPokemon() {
    consoleSelect(&bottomScreen);
    printf("\x1b[2J\x1b[10;5H" COLOR_BRIGHT_WHITE "Point at a Pokemon and press (A)" COLOR_RESET);
    printf("\x1b[12;10H" COLOR_YELLOW "Press (B) to Cancel" COLOR_RESET);

    Thread cameraViewThread = threadCreate(cameraThread, 0, STACKSIZE, 0x2F, 0, true);

    cameraSend = (u8*)malloc(BUF_SIZE);               // the buffer for the camera
    cameraRecv = (u8*)malloc((SCREEN_SIZE / 2) * 3);  // the necessary space for the rgb 24 datas of the image

    Y2RU_SetTransferEndInterrupt(true);
    Y2RU_GetTransferEndEvent(&y2rEvent);

    auto cameraBuf = (u8*)malloc(BUF_SIZE);

    // Main loop
    while (aptMainLoop()) {
        hidScanInput();

        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        if(kDown & KEY_START)
            break; // break in order to return to hbmenu

        if (kHeld & KEY_R) {
            printf("\x1b[10;10H" COLOR_YELLOW "Classifying Pokemon..." COLOR_RESET);
            gfxFlushBuffers();
            gspWaitForVBlank();
            gfxSwapBuffers();
            takePicture3D(cameraBuf);
            break;
        }

        u8 *fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, nullptr, nullptr); // get the current framebuffer

        //TODO: Explain the rotation
        Y2RU_SetSendingYUYV((u8 *) & cameraSend[0], WIDTH * 2 * HEIGHT, WIDTH * 2, 0);    // to send the YUV datas of the camera
        Y2RU_SetReceiving((void *) &cameraRecv[0], WIDTH * 3 * HEIGHT, 3 * WIDTH, 0);     // to receive the new rgb 24 datas
        Y2RU_StartConversion();                                                     // to start the conversion
        svcWaitSynchronization(y2rEvent, 1000 * 1000 * 10);                         // wait the end of the conversion

        writePictureToFramebufferRGB24_Y2R(fb, (void*) cameraRecv, 0, 0, WIDTH, HEIGHT);  // draw to the framebuffer

        // Flush and swap framebuffers
        gfxFlushBuffers();
        gspWaitForVBlank();
        gfxSwapBuffers();
    }

    cameraThreadExit = true; // tell thread to exit

    threadJoin(cameraViewThread, WAIT_TIMEOUT);
    threadFree(cameraViewThread);

    free((void*) cameraRecv);

    std::unique_ptr<Pokemon> pokemon = PokemonApi::getInstance().classifyImage(cameraBuf, SCREEN_SIZE);

    free(cameraBuf);

    return pokemon;
}

void PokemonClassifierClient::writePictureToFramebufferRGB24_Y2R(void *fb, void *img, u16 x, u16 y, u16 width, u16 height) {
    auto fb_8 = (u8*) fb;
    auto img_8 = (u8*) img;

    fb_8+= HEIGHT*3 - 8*3;
    for(int j = 0; j < HEIGHT/8; j++, fb_8-=HEIGHT*WIDTH*3+8*3)
        for(int i = 0; i < WIDTH; i++, img_8+=3*8, fb_8+=HEIGHT*3)
            memcpy(fb_8, img_8, 1*8*3);
}
