#pragma once
#include "GLHeaders.h"
#include "ShaderProgram.h"
#include "GPUBuffer.h"

// ---------------------------------------------------------------------------
// PostProcessor
// Reads the scene FBO texture and blits it to the default framebuffer,
// optionally applying CRT effects (barrel distortion, scanlines, vignette,
// bloom). When all effects are disabled it is a straight blit — zero cost.
// Phase 1 (monitor): filter = None, plain blit.
// Phase 2 (CRT TV):  filter = CRT, dial in barrelStrength etc.
// ---------------------------------------------------------------------------
class PostProcessor
{
public:
    enum class FilterMode
    {
        None,
        CRT,
        Fisheye
    };

    struct Config
    {
        FilterMode filter = FilterMode::None;
        float barrelStrength = 0.28f;  // 0 = flat, ~0.2 = strong CRT curve
        float scanlineAlpha = 0.10f;   // 0 = off
        float vignetteRadius = 0.75f;  // 0 = off, lower = darker corners
        float bloomStrength = 0.15f;   // 0 = off
        float fisheyeStrength = 0.55f; // 0 = off
    };

    bool init(const std::string &shaderDir, int width, int height);
    void resize(int width, int height);
    void render(GLuint sceneTex, const Config &cfg);
    ~PostProcessor();

private:
    ShaderProgram blitShader_;
    ShaderProgram crtShader_;
    ShaderProgram fisheyeShader_;
    FullscreenQuad quad_;

    // Two-pass bloom: downsample FBO + blur FBO
    GLuint bloomFBO_ = 0;
    GLuint bloomTex_ = 0;
    int width_ = 0;
    int height_ = 0;
};
