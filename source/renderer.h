#ifndef RENDERER_H
#define RENDERER_H

#include <citro2d.h>
#include <string>
#include <vector>

class Renderer {
public:
    static Renderer& getInstance() {
        static Renderer instance;
        return instance;
    }

    ~Renderer();

    void init();
    void exit();

    void beginFrame();
    void endFrame();

    // Screen targets
    C3D_RenderTarget* getTopTarget() { return topTarget; }
    C3D_RenderTarget* getBottomTarget() { return bottomTarget; }

    // Drawing primitives
    void drawRect(float x, float y, float w, float h, u32 color, bool top = true);
    void drawRectOutline(float x, float y, float w, float h, float thickness, u32 color, bool top = true);
    void drawText(float x, float y, float scale, u32 color, const char* text, bool top = true);
    void drawTextWrapped(float x, float y, float scale, float wrapWidth, u32 color, const char* text, bool top = true);

    // Color helper
    static u32 Color(u8 r, u8 g, u8 b, u8 a = 255) { return C2D_Color32(r, g, b, a); }

private:
    Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    C3D_RenderTarget* topTarget;
    C3D_RenderTarget* bottomTarget;

    C2D_TextBuf staticTextBuf;
    C2D_TextBuf dynamicTextBuf;
};

#endif // RENDERER_H
