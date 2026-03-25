#include "WaveformVisualizer.h"
#include <cmath>
#include <algorithm>
#include <cstddef>

bool WaveformVisualizer::init(const std::string &shaderDir, const VisualizerColorScheme &scheme)
{
    if (!shader_.load(shaderDir + "/waveform.vert",
                      shaderDir + "/waveform.frag"))
        return false;
    quad_.init();
    colors_ = scheme.waveform;
    return true;
}

void WaveformVisualizer::onResize(int w, int h)
{
    width_ = w;
    height_ = h;
    writeCol_ = 0;
    topHeights_ = {0.0f, 0.0f, 0.0f};
    botHeights_ = {0.0f, 0.0f, 0.0f};
    bandGains_ = {0.0f, 0.0f, 0.0f};
    // Texture: width x height, RGB bytes
    waveformTex_.init(w, h, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
    // Clear to black
    std::vector<uint8_t> black(w * h * 3, 0);
    waveformTex_.uploadFull(black.data());

    lowQueue_.clear();
    midQueue_.clear();
    highQueue_.clear();
    queueRead_ = 0;
}

void WaveformVisualizer::update(const AnalysisFrame &frame)
{
    if (width_ == 0)
        return;

    enqueueSamples(frame);

    float minLow = 0.0f, maxLow = 0.0f;
    float minMid = 0.0f, maxMid = 0.0f;
    float minHigh = 0.0f, maxHigh = 0.0f;

    const float half = static_cast<float>(height_) * 0.5f - 1.0f;
    const float targetFill = 0.60f;
    std::vector<uint8_t> col(height_ * 3);

    while (popWindow(minLow, maxLow, minMid, maxMid, minHigh, maxHigh))
    {
        const float ampRaw[3] = {
            std::max(std::abs(minLow), std::abs(maxLow)),
            std::max(std::abs(minMid), std::abs(maxMid)),
            std::max(std::abs(minHigh), std::abs(maxHigh))};

        const float ampMax = std::max({ampRaw[0], ampRaw[1], ampRaw[2], 1e-4f});
        const float desiredGain = std::clamp(targetFill / ampMax, 0.5f, 3.0f);
        const float attack = 0.05f;
        const float release = 0.30f;
        const float alpha = (desiredGain > autoGain_) ? attack : release;
        autoGain_ = autoGain_ + (desiredGain - autoGain_) * alpha;

        const float amp[3] = {
            ampRaw[0] * autoGain_,
            ampRaw[1] * autoGain_,
            ampRaw[2] * autoGain_};

        for (int b = 0; b < 3; ++b)
        {
            const float h = std::round(std::clamp(amp[b], 0.0f, 1.0f) * half);
            topHeights_[b] = h;
            botHeights_[b] = h;
        }
        const float maxH = std::max({topHeights_[0], topHeights_[1], topHeights_[2]});
        if (maxH > 0.0f)
        {
            bandGains_[0] = topHeights_[0] / maxH;
            bandGains_[1] = topHeights_[1] / maxH;
            bandGains_[2] = topHeights_[2] / maxH;
        }
        else
        {
            bandGains_ = {0.0f, 0.0f, 0.0f};
        }

        buildColumn(col);
        waveformTex_.uploadColumn(writeCol_, col.data());
        writeCol_ = (writeCol_ + 1) % width_;
    }
}

void WaveformVisualizer::enqueueSamples(const AnalysisFrame &frame)
{
    if (frame.bandSamplesLow.empty() || frame.bandSamplesMid.empty() || frame.bandSamplesHigh.empty())
        return;

    lowQueue_.insert(lowQueue_.end(), frame.bandSamplesLow.begin(), frame.bandSamplesLow.end());
    midQueue_.insert(midQueue_.end(), frame.bandSamplesMid.begin(), frame.bandSamplesMid.end());
    highQueue_.insert(highQueue_.end(), frame.bandSamplesHigh.begin(), frame.bandSamplesHigh.end());
}

bool WaveformVisualizer::popWindow(float &outMinLow, float &outMaxLow,
                                   float &outMinMid, float &outMaxMid,
                                   float &outMinHigh, float &outMaxHigh)
{
    const size_t available = std::min(lowQueue_.size(),
                                      std::min(midQueue_.size(), highQueue_.size()));
    if (available < queueRead_ + static_cast<size_t>(samplesPerColumn_))
        return false;

    outMinLow = 1.0f;
    outMinMid = 1.0f;
    outMinHigh = 1.0f;
    outMaxLow = -1.0f;
    outMaxMid = -1.0f;
    outMaxHigh = -1.0f;

    const size_t start = queueRead_;
    const size_t stop = queueRead_ + static_cast<size_t>(samplesPerColumn_);
    for (size_t i = start; i < stop; ++i)
    {
        const float low = lowQueue_[i];
        const float mid = midQueue_[i];
        const float high = highQueue_[i];

        outMinLow = std::min(outMinLow, low);
        outMinMid = std::min(outMinMid, mid);
        outMinHigh = std::min(outMinHigh, high);
        outMaxLow = std::max(outMaxLow, low);
        outMaxMid = std::max(outMaxMid, mid);
        outMaxHigh = std::max(outMaxHigh, high);
    }

    queueRead_ = stop;
    if (queueRead_ > 4096)
    {
        const auto eraseCount = static_cast<std::ptrdiff_t>(queueRead_);
        lowQueue_.erase(lowQueue_.begin(), lowQueue_.begin() + eraseCount);
        midQueue_.erase(midQueue_.begin(), midQueue_.begin() + eraseCount);
        highQueue_.erase(highQueue_.begin(), highQueue_.begin() + eraseCount);
        queueRead_ = 0;
    }

    return true;
}

void WaveformVisualizer::buildColumn(std::vector<uint8_t> &col) const
{
    const float mid = static_cast<float>(height_) * 0.5f;

    for (int y = 0; y < height_; ++y)
    {
        const float offset = static_cast<float>(y) - mid;
        const float dist = std::abs(offset);
        float r = 0.0f, g = 0.0f, b = 0.0f;

        // Mixxx-style color blending: additive mix with normalization
        const Color3 colours[3] = {colors_.low, colors_.mid, colors_.high};
        for (int band = 0; band < 3; ++band)
        {
            const float bh = (offset <= 0.0f) ? topHeights_[band] : botHeights_[band];
            if (bh < 1.0f || dist > bh)
                continue;
            const float gain = bandGains_[band];
            r += colours[band].r * 255.0f * gain;
            g += colours[band].g * 255.0f * gain;
            b += colours[band].b * 255.0f * gain;
        }

        r *= colors_.colorGain;
        g *= colors_.colorGain;
        b *= colors_.colorGain;

        const float maxC = std::max({r, g, b});
        if (maxC > 255.0f)
        {
            const float scale = 255.0f / maxC;
            r *= scale;
            g *= scale;
            b *= scale;
        }

        r *= colors_.mixGain;
        g *= colors_.mixGain;
        b *= colors_.mixGain;

        col[y * 3 + 0] = static_cast<uint8_t>(r);
        col[y * 3 + 1] = static_cast<uint8_t>(g);
        col[y * 3 + 2] = static_cast<uint8_t>(b);
    }
}

void WaveformVisualizer::render()
{
    if (width_ == 0)
        return;
    shader_.use();
    waveformTex_.bind(0);
    shader_.setInt("uWaveform", 0);
    // Normalised offset for circular scroll: next write position
    const float writeNorm = static_cast<float>(writeCol_) / width_;
    shader_.setFloat("uWriteCol", writeNorm);
    quad_.draw();
}

WaveformVisualizer::~WaveformVisualizer() = default;