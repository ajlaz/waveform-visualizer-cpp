#include "SpectrumVisualizer.h"
#include <cmath>
#include <algorithm>

// Log-frequency mapping: bin index → pixel column
static int binToPixel(int bin, int /*numBins*/, int width)
{
    const float freq = static_cast<float>(bin) * SAMPLE_RATE / FFT_SIZE;
    if (freq < FREQ_MIN || freq > FREQ_MAX)
        return -1;
    const float logMin = std::log10(FREQ_MIN);
    const float logMax = std::log10(FREQ_MAX);
    const float t = (std::log10(freq) - logMin) / (logMax - logMin);
    return static_cast<int>(std::clamp(t * (width - 1), 0.0f, float(width - 1)));
}

bool SpectrumVisualizer::init(const std::string &shaderDir, const VisualizerColorScheme &scheme)
{
    if (!shader_.load(shaderDir + "/spectrum.vert",
                      shaderDir + "/spectrum.frag"))
        return false;
    quad_.init();
    if (!textRenderer_.init(shaderDir)) return false;
    colors_ = scheme.spectrum;
    return true;
}

void SpectrumVisualizer::onResize(int w, int h)
{
    width_ = w;
    height_ = h;
    textRenderer_.resize(w, h);
    smoothed_.assign(w, DB_MIN);
    specTex_.init(w);
}

void SpectrumVisualizer::update(const AnalysisFrame &frame)
{
    if (width_ == 0 || frame.spectrumDb.empty())
        return;

    const int numBins = static_cast<int>(frame.spectrumDb.size());

    // Map spectrum bins onto pixel columns (log scale)
    // For each pixel, find the max bin that maps to it
    std::vector<float> pixelDb(width_, DB_MIN);
    std::vector<int> pixelCount(width_, 0);

    for (int b = 0; b < numBins; ++b)
    {
        int px = binToPixel(b, numBins, width_);
        if (px < 0)
            continue;
        pixelDb[px] = std::max(pixelDb[px], frame.spectrumDb[b]);
        pixelCount[px]++;
    }

    // Interpolate empty pixels (gaps in low-freq region)
    int lastValid = -1;
    for (int x = 0; x < width_; ++x)
    {
        if (pixelCount[x] > 0)
        {
            if (lastValid >= 0 && x - lastValid > 1)
            {
                // Linear interpolation between lastValid and x
                float vA = pixelDb[lastValid];
                float vB = pixelDb[x];
                for (int i = lastValid + 1; i < x; ++i)
                {
                    float t = float(i - lastValid) / float(x - lastValid);
                    pixelDb[i] = vA + t * (vB - vA);
                }
            }
            lastValid = x;
        }
    }

    // Asymmetric smoothing (tighter decay to reduce lingering)
    static constexpr float ATTACK = 0.65f;
    static constexpr float DECAY = 0.28f;
    for (int x = 0; x < width_; ++x)
    {
        const float raw = pixelDb[x];
        if (raw > smoothed_[x])
            smoothed_[x] += ATTACK * (raw - smoothed_[x]);
        else
            smoothed_[x] += DECAY * (raw - smoothed_[x]);
    }

    // Spatial smoothing for a more EQ-like curve (wider, two-pass)
    std::vector<float> pass1(width_);
    std::vector<float> pass2(width_);
    for (int x = 0; x < width_; ++x)
    {
        float acc = 0.0f;
        float wsum = 0.0f;
        for (int k = -4; k <= 4; ++k)
        {
            const int ix = std::clamp(x + k, 0, width_ - 1);
            float w = 1.0f;
            if (k == 0)
                w = 6.0f;
            else if (std::abs(k) == 1)
                w = 4.0f;
            else if (std::abs(k) == 2)
                w = 3.0f;
            else if (std::abs(k) == 3)
                w = 2.0f;
            acc += smoothed_[ix] * w;
            wsum += w;
        }
        pass1[x] = acc / wsum;
    }

    for (int x = 0; x < width_; ++x)
    {
        float acc = 0.0f;
        float wsum = 0.0f;
        for (int k = -3; k <= 3; ++k)
        {
            const int ix = std::clamp(x + k, 0, width_ - 1);
            float w = (k == 0) ? 4.0f : (std::abs(k) == 1 ? 3.0f : (std::abs(k) == 2 ? 2.0f : 1.0f));
            acc += pass1[ix] * w;
            wsum += w;
        }
        pass2[x] = acc / wsum;
    }

    // Normalise smoothed dB to [0,1], shape, and upload
    std::vector<float> norm(width_);
    for (int x = 0; x < width_; ++x)
    {
        float n = std::clamp((pass2[x] - DB_MIN) / (DB_MAX - DB_MIN), 0.0f, 1.0f);
        // Light curve shaping to smooth small spikes
        n = std::pow(n, 0.85f);
        norm[x] = n;
    }

    specTex_.upload(norm.data(), width_);
}

void SpectrumVisualizer::render()
{
    if (width_ == 0)
        return;
    shader_.use();
    specTex_.bind(0);
    shader_.setInt("uSpectrum", 0);
    shader_.setFloat("uDbMin", DB_MIN);
    shader_.setFloat("uDbMax", DB_MAX);
    shader_.setVec3("uColorLow", colors_.low.r, colors_.low.g, colors_.low.b);
    shader_.setVec3("uColorMid", colors_.mid.r, colors_.mid.g, colors_.mid.b);
    shader_.setVec3("uColorHigh", colors_.high.r, colors_.high.g, colors_.high.b);
    shader_.setVec3("uBackground", colors_.background.r, colors_.background.g, colors_.background.b);
    shader_.setVec3("uTipColor", colors_.tip.r, colors_.tip.g, colors_.tip.b);
    shader_.setFloat("uTipWidth", 1.5f / std::max(1, height_));
    quad_.draw();

    // Draw frequency labels along the bottom
    struct FreqLabel { float freq; const char* label; };
    static const FreqLabel kLabels[] = {
        {50.0f,"50"},{100.0f,"100"},{200.0f,"200"},
        {500.0f,"500"},{1000.0f,"1k"},{2000.0f,"2k"},{5000.0f,"5k"},
        {10000.0f,"10k"}
    };
    const float logMin = std::log10(FREQ_MIN);
    const float logMax = std::log10(FREQ_MAX);
    const float labelR = colors_.tip.r * 0.7f;
    const float labelG = colors_.tip.g * 0.7f;
    const float labelB = colors_.tip.b * 0.7f;
    const float scale  = (height_ >= 400) ? 1.5f : 1.0f;
    const int   labelY = height_ - static_cast<int>(10.0f * scale) - 2;
    for (const auto& lbl : kLabels) {
        const float t  = (std::log10(lbl.freq) - logMin) / (logMax - logMin);
        const int   px = static_cast<int>(t * (width_ - 1));
        textRenderer_.drawCentered(lbl.label, px, labelY,
                                   labelR, labelG, labelB, scale);
    }
}

SpectrumVisualizer::~SpectrumVisualizer() = default;
