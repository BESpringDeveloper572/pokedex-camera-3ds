#ifndef CAMERA_H
#define CAMERA_H

#include <3ds.h>

class Camera {
public:
    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;

    static Camera& getInstance() {
        static Camera instance;
        return instance;
    }

    ~Camera();

    // Camera viewfinder and capture management
    void startCapture();
    void stopCapture();
    void getLatestFrame(u8* outRGB24);

    void writePictureToFramebufferRGB24_Y2R(void *fb, void *img, u16 x, u16 y, u16 width, u16 height);

private:
    Camera();

    bool initialized;
};

#endif // CAMERA_H
