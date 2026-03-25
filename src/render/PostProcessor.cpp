#include "PostProcessor.h"

#include <iostream>

// ---------------------------------------------------------------------------
// Destructor
// ---------------------------------------------------------------------------
PostProcessor::~PostProcessor()
{
    if (bloomTex_)
    {
        glDeleteTextures(1, &bloomTex_);
        bloomTex_ = 0;
    }
    if (bloomFBO_)
    {
        glDeleteFramebuffers(1, &bloomFBO_);
        bloomFBO_ = 0;
    }
}

// ---------------------------------------------------------------------------
// init()
// ---------------------------------------------------------------------------
bool PostProcessor::init(const std::string &shaderDir, int width, int height)
{
    width_ = width;
    height_ = height;

    // Build full paths for each shader stage.
    const std::string blitVert = shaderDir + "/blit.vert";
    const std::string blitFrag = shaderDir + "/blit.frag";
    const std::string crtVert = shaderDir + "/crt.vert";
    const std::string crtFrag = shaderDir + "/crt.frag";
    const std::string fishVert = shaderDir + "/blit.vert";
    const std::string fishFrag = shaderDir + "/fisheye.frag";

    if (!blitShader_.load(blitVert, blitFrag))
    {
        std::cerr << "[PostProcessor] Failed to load blit shader ("
                  << blitVert << ", " << blitFrag << ")\n";
        return false;
    }

    if (!crtShader_.load(crtVert, crtFrag))
    {
        std::cerr << "[PostProcessor] Failed to load CRT shader ("
                  << crtVert << ", " << crtFrag << ")\n";
        return false;
    }

    if (!fisheyeShader_.load(fishVert, fishFrag))
    {
        std::cerr << "[PostProcessor] Failed to load fisheye shader ("
                  << fishVert << ", " << fishFrag << ")\n";
        return false;
    }

    quad_.init();

    return true;
}

// ---------------------------------------------------------------------------
// resize()
// ---------------------------------------------------------------------------
void PostProcessor::resize(int width, int height)
{
    width_ = width;
    height_ = height;
}

// ---------------------------------------------------------------------------
// render()
// ---------------------------------------------------------------------------
void PostProcessor::render(GLuint sceneTex, const Config &cfg)
{
    // Target the default (screen) framebuffer.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width_, height_);
    glClear(GL_COLOR_BUFFER_BIT);

    switch (cfg.filter)
    {
    case FilterMode::CRT:
        crtShader_.use();
        crtShader_.setFloat("uBarrelStrength", cfg.barrelStrength);
        crtShader_.setFloat("uScanlineAlpha", cfg.scanlineAlpha);
        crtShader_.setFloat("uVignetteRadius", cfg.vignetteRadius);
        crtShader_.setVec2("uResolution",
                           static_cast<float>(width_),
                           static_cast<float>(height_));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneTex);
        crtShader_.setInt("uScene", 0);
        break;
    case FilterMode::Fisheye:
        fisheyeShader_.use();
        fisheyeShader_.setFloat("uStrength", cfg.fisheyeStrength);
        fisheyeShader_.setFloat("uVignetteRadius", cfg.vignetteRadius);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneTex);
        fisheyeShader_.setInt("uScene", 0);
        break;
    case FilterMode::None:
    default:
        blitShader_.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneTex);
        blitShader_.setInt("uScene", 0);
        break;
    }

    quad_.draw();
}
