#include "StereoImagerVisualizer.h"
#include "../render/GLHeaders.h"
#include <cmath>
#include <algorithm>

bool StereoImagerVisualizer::init(const std::string &shaderDir,
                                  const VisualizerColorScheme &scheme,
                                  int texSize)
{
    if (!shader_.load(shaderDir + "/imager.vert",
                      shaderDir + "/imager.frag"))
        return false;

    quad_.init();
    colors_  = scheme.stereoImager;
    texSize_ = texSize;

    glGenTextures(1, &tex_);
    glBindTexture(GL_TEXTURE_2D, tex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F,
                 texSize_, texSize_, 0,
                 GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    accum_.assign(texSize_ * texSize_, 0.0f);
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

    for (float &v : accum_)
        v *= decay_;

    auto add = [&](int x, int y, float a) {
        if (x >= 0 && x < texSize_ && y >= 0 && y < texSize_)
            accum_[y * texSize_ + x] = std::min(1.0f, accum_[y * texSize_ + x] + a);
    };

    for (size_t i = 0; i < n; ++i) {
        const float L    = frame.samplesL[i];
        const float R    = frame.samplesR[i];
        float side = (L - R) * 0.5f;
        float mid  = (L + R) * 0.5f;

        float dotBrightness = brightness_;
        if (edgeSoften_ > 0.0f) {
            const float edge = std::max(std::abs(side), std::abs(mid)) / scale_;
            const float t = std::clamp((edge - 0.85f) / 0.30f, 0.0f, 1.0f);
            const float s = t * t * (3.0f - 2.0f * t);
            dotBrightness *= (1.0f - edgeSoften_ * s);
        }

        const float xf =  (side / scale_ * 0.5f + 0.5f) * (texSize_ - 1);
        const float yf = (-mid  / scale_ * 0.5f + 0.5f) * (texSize_ - 1);
        if (xf < 0.0f || xf >= texSize_ - 1 || yf < 0.0f || yf >= texSize_ - 1) continue;

        const int   x0 = static_cast<int>(xf);
        const int   y0 = static_cast<int>(yf);
        const float fx = xf - x0;
        const float fy = yf - y0;

        add(x0,     y0,     dotBrightness * (1.0f - fx) * (1.0f - fy));
        add(x0 + 1, y0,     dotBrightness * fx          * (1.0f - fy));
        add(x0,     y0 + 1, dotBrightness * (1.0f - fx) * fy);
        add(x0 + 1, y0 + 1, dotBrightness * fx          * fy);
    }

    glBindTexture(GL_TEXTURE_2D, tex_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0,
                    0, 0, texSize_, texSize_,
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

void StereoImagerVisualizer::setParam(std::string_view key, float value)
{
    if      (key == "decay")      decay_      = std::clamp(value, 0.80f, 0.99f);
    else if (key == "brightness") brightness_ = std::clamp(value, 0.01f, 0.50f);
    else if (key == "scale")      scale_      = std::clamp(value, 0.10f, 2.00f);
    else if (key == "edge_soften") edgeSoften_ = std::clamp(value, 0.0f, 1.0f);
}

nlohmann::json StereoImagerVisualizer::getParams() const
{
    return {
        {"decay", decay_},
        {"brightness", brightness_},
        {"scale", scale_},
        {"edge_soften", edgeSoften_}
    };
}

StereoImagerVisualizer::~StereoImagerVisualizer()
{
    if (tex_) { glDeleteTextures(1, &tex_); tex_ = 0; }
}
