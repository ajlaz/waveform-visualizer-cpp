#pragma once
#include "Visualizer.h"
#include "../render/ShaderProgram.h"
#include "../render/GPUBuffer.h"
#include "../render/TextRenderer.h"
#include "../ColorSchemes.h"

class VUMeterVisualizer : public Visualizer
{
public:
    bool init(const std::string &shaderDir, const VisualizerColorScheme &scheme);

    std::string_view name() const override { return "VU Meter"; }
    void onResize(int w, int h) override;
    void update(const AnalysisFrame &frame) override;
    void render() override;

    ~VUMeterVisualizer() override;

private:
    ShaderProgram shader_;
    FullscreenQuad quad_;
    TextRenderer textRenderer_;

    VUMeterColors colors_;

    float level_ = 0.0f;    // normalised 0–1 (maps DB_FLOOR..DB_CEIL)
    float peakDb_ = -90.0f; // peak hold
    float peakDecay_ = 0.0f;
    int width_ = 0;
    int height_ = 0;
};
