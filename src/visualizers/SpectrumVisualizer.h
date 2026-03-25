#pragma once
#include "Visualizer.h"
#include "../render/ShaderProgram.h"
#include "../render/GPUBuffer.h"
#include "../ColorSchemes.h"
#include <vector>

// Spectrum visualizer: uploads spectrumDb as a 1D texture each frame.
// The fragment shader maps horizontal position → frequency bin (log scale)
// and vertical position → amplitude, filling bars with a heat-map gradient.
class SpectrumVisualizer : public Visualizer
{
public:
    bool init(const std::string &shaderDir, const VisualizerColorScheme &scheme);

    std::string_view name() const override { return "Spectrum"; }
    void onResize(int w, int h) override;
    void update(const AnalysisFrame &frame) override;
    void render() override;

    ~SpectrumVisualizer() override;

private:
    ShaderProgram shader_;
    FullscreenQuad quad_;
    StreamTexture1D specTex_;

    SpectrumColors colors_;

    std::vector<float> smoothed_; // per-pixel smoothed dB (width_ entries)
    int width_ = 0;
    int height_ = 0;
};
