#include "StereoImagerVisualizer.h"
#include "../render/GLHeaders.h"
#include <cmath>
#include <algorithm>

bool StereoImagerVisualizer::init(const std::string &shaderDir,
                                  const VisualizerColorScheme &scheme)
{
    if (!shader_.load(shaderDir + "/imager.vert",
                      shaderDir + "/imager.frag"))
        return false;

    quad_.init();
    colors_ = scheme.stereoImager;

    // Allocate GL_R32F accumulation texture
    glGenTextures(1, &tex_);
    glBindTexture(GL_TEXTURE_2D, tex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F,
                 TEX_SIZE, TEX_SIZE, 0,
                 GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    accum_.assign(TEX_SIZE * TEX_SIZE, 0.0f);
    return true;
}

void StereoImagerVisualizer::onResize(int w, int h)
{
    width_  = w;
    height_ = h;
}

void StereoImagerVisualizer::update(const AnalysisFrame &frame)
{
    const size_t n = std::min(frame.samplesL.size(), frame.samplesR.size());
    if (n == 0) return;

    // Decay the whole accumulation buffer
    for (float &v : accum_)
        v *= DECAY;

    // Helper: paint a pixel and its 4 neighbours (cross splat)
    auto paint = [&](int px, int py, float amount) {
        auto add = [&](int x, int y, float a) {
            if (x >= 0 && x < TEX_SIZE && y >= 0 && y < TEX_SIZE)
                accum_[y * TEX_SIZE + x] = std::min(1.0f, accum_[y * TEX_SIZE + x] + a);
        };
        add(px,   py,   amount);
        add(px-1, py,   amount * 0.5f);
        add(px+1, py,   amount * 0.5f);
        add(px,   py-1, amount * 0.5f);
        add(px,   py+1, amount * 0.5f);
    };

    for (size_t i = 0; i < n; ++i) {
        const float L = frame.samplesL[i];
        const float R = frame.samplesR[i];

        // M/S decomposition, both in [-1, 1]
        const float side = (L - R) * 0.5f;
        const float mid  = (L + R) * 0.5f;

        // Map to [0,1] texture coords: side→X (right=more L), mid→Y (up=louder)
        const float xn =  side / SCALE * 0.5f + 0.5f;
        const float yn = -mid  / SCALE * 0.5f + 0.5f;
        if (xn < 0.0f || xn > 1.0f || yn < 0.0f || yn > 1.0f) continue;

        const int px = static_cast<int>(xn * (TEX_SIZE - 1));
        const int py = static_cast<int>(yn * (TEX_SIZE - 1));
        paint(px, py, ADD);
    }

    // Upload updated accumulation to GPU
    glBindTexture(GL_TEXTURE_2D, tex_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0,
                    0, 0, TEX_SIZE, TEX_SIZE,
                    GL_RED, GL_FLOAT, accum_.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void StereoImagerVisualizer::render()
{
    if (width_ == 0) return;

    shader_.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_);
    shader_.setInt("uAccum", 0);
    shader_.setVec3("uBackground",
                    colors_.background.r,
                    colors_.background.g,
                    colors_.background.b);
    shader_.setVec3("uDot",
                    colors_.dot.r,
                    colors_.dot.g,
                    colors_.dot.b);
    shader_.setVec2("uResolution",
                    static_cast<float>(width_),
                    static_cast<float>(height_));
    quad_.draw();
}

StereoImagerVisualizer::~StereoImagerVisualizer()
{
    if (tex_) { glDeleteTextures(1, &tex_); tex_ = 0; }
}
