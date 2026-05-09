#include "renderer.h"
#include <sstream>

Renderer::Renderer() : topTarget(nullptr), bottomTarget(nullptr) {
    // Initialized in init()
}

Renderer::~Renderer() {
    exit();
}

void Renderer::init() {
    if (topTarget) return; // Already initialized

    gfxInitDefault();
    gfxSet3D(false);

    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    staticTextBuf = C2D_TextBufNew(4096);
    dynamicTextBuf = C2D_TextBufNew(4096);
}

void Renderer::exit() {
    if (!topTarget) return;

    C2D_TextBufDelete(staticTextBuf);
    C2D_TextBufDelete(dynamicTextBuf);

    C2D_Fini();
    C3D_Fini();
    gfxExit();

    topTarget = nullptr;
    bottomTarget = nullptr;
}

void Renderer::beginFrame() {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(topTarget, C2D_Color32(30, 30, 30, 255));
    C2D_TargetClear(bottomTarget, C2D_Color32(30, 30, 30, 255));
    C2D_TextBufClear(dynamicTextBuf);
}

void Renderer::endFrame() {
    C3D_FrameEnd(0);
}

void Renderer::drawRect(float x, float y, float w, float h, u32 color, bool top) {
    C2D_SceneBegin(top ? topTarget : bottomTarget);
    C2D_DrawRectSolid(x, y, 0.5f, w, h, color);
}

void Renderer::drawRectOutline(float x, float y, float w, float h, float thickness, u32 color, bool top) {
    C2D_SceneBegin(top ? topTarget : bottomTarget);
    // Top
    C2D_DrawLine(x, y, color, x + w, y, color, thickness, 0.5f);
    // Bottom
    C2D_DrawLine(x, y + h, color, x + w, y + h, color, thickness, 0.5f);
    // Left
    C2D_DrawLine(x, y, color, x, y + h, color, thickness, 0.5f);
    // Right
    C2D_DrawLine(x + w, y, color, x + w, y + h, color, thickness, 0.5f);
}

void Renderer::drawText(float x, float y, float scale, u32 color, const char* text, bool top) {
    C2D_SceneBegin(top ? topTarget : bottomTarget);
    
    C2D_Text gtext;
    C2D_TextParse(&gtext, dynamicTextBuf, text);
    C2D_TextOptimize(&gtext);
    C2D_DrawText(&gtext, C2D_WithColor, x, y, 0.5f, scale, scale, color);
}

void Renderer::drawTextWrapped(float x, float y, float scale, float wrapWidth, u32 color, const char* text, bool top) {
    C2D_SceneBegin(top ? topTarget : bottomTarget);

    std::string input(text);
    std::string currentLine;
    std::string word;
    float currentY = y;
    float spaceWidth;

    // Measure a space character
    C2D_Text spaceText;
    C2D_TextParse(&spaceText, dynamicTextBuf, " ");
    spaceWidth = spaceText.width * scale;

    auto ss = std::stringstream(input);
    while (ss >> word) {
        C2D_Text wordText;
        C2D_TextParse(&wordText, dynamicTextBuf, word.c_str());
        float wordWidth = wordText.width * scale;

        C2D_Text lineText;
        C2D_TextParse(&lineText, dynamicTextBuf, (currentLine + word).c_str());
        float lineWidth = lineText.width * scale;

        if (lineWidth > wrapWidth && !currentLine.empty()) {
            // Draw current line and start new one
            C2D_Text finalLineText;
            C2D_TextParse(&finalLineText, dynamicTextBuf, currentLine.c_str());
            C2D_TextOptimize(&finalLineText);
            C2D_DrawText(&finalLineText, C2D_WithColor, x, currentY, 0.5f, scale, scale, color);
            
            currentY += 30.0f * scale; // Line height
            currentLine = word + " ";
        } else {
            currentLine += word + " ";
        }
    }

    if (!currentLine.empty()) {
        C2D_Text finalLineText;
        C2D_TextParse(&finalLineText, dynamicTextBuf, currentLine.c_str());
        C2D_TextOptimize(&finalLineText);
        C2D_DrawText(&finalLineText, C2D_WithColor, x, currentY, 0.5f, scale, scale, color);
    }
}
