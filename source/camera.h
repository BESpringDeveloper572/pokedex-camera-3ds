#ifndef CAMERA_H
#define CAMERA_H

#include <3ds.h>

constexpr size_t CAM_WIDTH = 400;
constexpr size_t CAM_HEIGHT = 240;
constexpr size_t CAM_BUF_SIZE = (CAM_WIDTH * CAM_HEIGHT * 2);

class Camera {
public:
    static Camera& getInstance() {
        static Camera instance;
        return instance;
    }

    ~Camera();

    bool init();
    void exit();

    // Safe copy of the current frame using LightLock
    void copyFrame(u16* outBuffer);
    
    // Direct lock access for preview
    void lockBuffer() { LightLock_Lock(&lock); }
    void unlockBuffer() { LightLock_Unlock(&lock); }

    u16* getSharedBuffer() { return sharedBuffer; }
    bool isFrameReady() { return frameReady; }
    void clearFrameReady() { frameReady = false; }

private:
    Camera();
    bool initialized;
    
    Thread thread;
    Handle stopEvent;
    u16* sharedBuffer;
    volatile bool frameReady;
    
    LightLock lock;
    LightLock stateLock;

    friend void cameraThreadFunc(void* arg);
};

#endif // CAMERA_H
