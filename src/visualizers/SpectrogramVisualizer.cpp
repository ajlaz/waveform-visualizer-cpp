#include "SpectrogramVisualizer.h"
#include <cmath>
#include <algorithm>

static Color3 lerpColor(const Color3 &a, const Color3 &b, float t)
{
    return {
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t};
}

// dB normalised [0,1] → heat-map RGB
static void dbToRGB(float t, const SpectrogramColors &colors,
                    uint8_t &r, uint8_t &g, uint8_t &b)
{
    t = std::clamp(t, 0.0f, 1.0f);
    Color3 out;
    if (t < colors.t1)
    {
        float s = t / colors.t1;
        out = lerpColor(colors.c0, colors.c1, s);
    }
    else if (t < colors.t2)
    {
        float s = (t - colors.t1) / (colors.t2 - colors.t1);
        out = lerpColor(colors.c1, colors.c2, s);
    }
    else if (t < colors.t3)
    {
        float s = (t - colors.t2) / (colors.t3 - colors.t2);
        out = lerpColor(colors.c2, colors.c3, s);
    }
    else if (t < colors.t4)
    {
        float s = (t - colors.t3) / (colors.t4 - colors.t3);
        out = lerpColor(colors.c3, colors.c4, s);
    }
    else
    {
        out = colors.c4;
    }

    r = static_cast<uint8_t>(std::clamp(out.r, 0.0f, 1.0f) * 255.0f);
    g = static_cast<uint8_t>(std::clamp(out.g, 0.0f, 1.0f) * 255.0f);
    b = static_cast<uint8_t>(std::clamp(out.b, 0.0f, 1.0f) * 255.0f);
}

bool SpectrogramVisualizer::init(const std::string &shaderDir, const VisualizerColorScheme &scheme)
{
    if (!shader_.load(shaderDir + "/spectrogram.vert",
                      shaderDir + "/spectrogram.frag"))
        return false;
    quad_.init();
    if (!textRenderer_.init(shaderDir)) return false;
    colors_ = scheme.spectrogram;
    return true;
}

void SpectrogramVisualizer::onResize(int w, int h)
{
    width_ = w;
    height_ = h;
    textRenderer_.resize(w, h);
    writeCol_ = 0;
    colBuf_.assign(h * 3, 0);
    scrollTex_.init(w, h, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
}

void SpectrogramVisualizer::update(const AnalysisFrame &frame)
{
    if (width_ == 0 || frame.spectrumDb.empty())
        return;

    const int numBins = static_cast<int>(frame.spectrumDb.size());

    // Map spectrum bins to pixel rows (log scale), compute heat-map column
    std::vector<float> pixelDb(height_, DB_MIN);
    for (int b = 0; b < numBins; ++b)
    {
        const float freq = static_cast<float>(b) * SAMPLE_RATE / FFT_SIZE;
        if (freq < FREQ_MIN || freq > FREQ_MAX)
            continue;
        const float logMin = std::log10(FREQ_MIN);
        const float logMax = std::log10(FREQ_MAX);
        const float t = (std::log10(freq) - logMin) / (logMax - logMin);
        int py = static_cast<int>(std::clamp(t * (height_ - 1), 0.0f, float(height_ - 1)));
        pixelDb[py] = std::max(pixelDb[py], frame.spectrumDb[b]);
    }

    for (int y = 0; y < height_; ++y)
    {
        float norm = std::clamp((pixelDb[y] - DB_MIN) / (DB_MAX - DB_MIN), 0.0f, 1.0f);
        uint8_t r, g, b;
        dbToRGB(norm, colors_, r, g, b);
        colBuf_[y * 3 + 0] = r;
        colBuf_[y * 3 + 1] = g;
        colBuf_[y * 3 + 2] = b;
    }

    // Upload column and advance write head (circular)
    // For spectrogram we scroll horizontally: write into column writeCol_
    glBindTexture(GL_TEXTURE_2D, 0);
    scrollTex_.bind(0);
    glTexSubImage2D(GL_TEXTURE_2D, 0,
                    writeCol_, 0,
                    1, height_,
                    GL_RGB, GL_UNSIGNED_BYTE,
                    colBuf_.data());

    writeCol_ = (writeCol_ + 1) % width_;
}

void SpectrogramVisualizer::render()
{
    if (width_ == 0)
        return;
    shader_.use();
    scrollTex_.bind(0);
    shader_.setInt("uScroll", 0);
    // Pass normalised write position so shader can offset UVs
    shader_.setFloat("uWriteCol", static_cast<float>(writeCol_) / width_);
    quad_.draw();

    // Draw frequency labels on the left Y-axis
    // Frequency maps to pixel row py = t*(height_-1) where t = log-scale [0,1]
    // In screen top-left coords: screen_y = height_ - 1 - py
    struct FreqLabel { float freq; const char* label; };
    static const FreqLabel kLabels[] = {
        {50.0f,"50"},{100.0f,"100"},{200.0f,"200"},
        {500.0f,"500"},{1000.0f,"1k"},{2000.0f,"2k"},{5000.0f,"5k"},
        {10000.0f,"10k"},{15000.0f,"15k"}
    };
    const float logMin = std::log10(FREQ_MIN);
    const float logMax = std::log10(FREQ_MAX);
    const float scale  = (height_ >= 400) ? 1.0f : 0.75f;
    const float lr = 0.7f, lg = 0.7f, lb = 0.7f; // light gray
    for (const auto& lbl : kLabels) {
        const float t  = (std::log10(lbl.freq) - logMin) / (logMax - logMin);
        const int   py = static_cast<int>(t * (height_ - 1));
        const int   sy = height_ - 1 - py - static_cast<int>(4.0f * scale);
        if (sy < 0 || sy >= height_) continue;
        textRenderer_.draw(lbl.label, 3, sy, lr, lg, lb, scale);
    }
}

SpectrogramVisualizer::~SpectrogramVisualizer() = default;
