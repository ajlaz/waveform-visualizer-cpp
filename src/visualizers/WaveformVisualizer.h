#pragma once
#include "Visualizer.h"
#include "../render/ShaderProgram.h"
#include "../render/GPUBuffer.h"
#include "../ColorSchemes.h"
#include <array>
#include <vector>

// Mixxx-style RGB waveform.
// Three symmetric bars (low=red, mid=green, high=blue) scroll left.
// One column of RGB pixels is computed per frame on the CPU and uploaded
// to a circular GPU texture via glTexSubImage2D. The fragment shader
// offsets UVs by the write-head position so no texture copy is needed.
class WaveformVisualizer : public Visualizer
{
public:
    bool init(const std::string &shaderDir, const VisualizerColorScheme &scheme);

    std::string_view name() const override { return "Waveform"; }
    void onResize(int w, int h) override;
    void update(const AnalysisFrame &frame) override;
    void render() override;

    void setParam(std::string_view key, float value) override;
    nlohmann::json getParams() const override;
    void setColorScheme(const VisualizerColorScheme& scheme) override { colors_ = scheme.waveform; }

    ~WaveformVisualizer() override;

private:
    // Compute one RGB column from smoothed band heights
    void buildColumn(std::vector<uint8_t> &col) const;

    void enqueueSamples(const AnalysisFrame &frame);
    bool popWindow(float &outMinLow, float &outMaxLow,
                   float &outMinMid, float &outMaxMid,
                   float &outMinHigh, float &outMaxHigh);

    ShaderProgram shader_;
    FullscreenQuad quad_;
    StreamTexture2D waveformTex_;

    WaveformColors colors_;

    // Smoothed top/bottom heights in pixels [low, mid, high]
    std::array<float, 3> topHeights_ = {0.0f, 0.0f, 0.0f};
    std::array<float, 3> botHeights_ = {0.0f, 0.0f, 0.0f};
    std::array<float, 3> bandGains_ = {0.0f, 0.0f, 0.0f};
    float autoGain_ = 1.0f;

    std::vector<float> lowQueue_;
    std::vector<float> midQueue_;
    std::vector<float> highQueue_;
    size_t queueRead_ = 0;
    int samplesPerColumn_ = 256;

    int writeCol_ = 0; // circular write position [0, width_)
    int width_ = 0;
    int height_ = 0;
    float lineWidth_ = 1.0f;  // bar height scale multiplier (0.5–4.0)
};