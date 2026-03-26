#pragma once
#include "Visualizer.h"
#include "../render/ShaderProgram.h"
#include "../render/GPUBuffer.h"
#include "../ColorSchemes.h"
#include <vector>
#include <algorithm>

// Goniometer / Lissajous stereo imager.
// X axis = (L-R)/2  (side signal)   — right = more L, left = more R
// Y axis = (L+R)/2  (mid signal)    — up = louder
class StereoImagerVisualizer : public Visualizer
{
public:
    // texSize: accumulation buffer side length (256 for Pi, 512 for desktop)
    bool init(const std::string &shaderDir, const VisualizerColorScheme &scheme,
              int texSize = 512);

    std::string_view name() const override { return "Stereo Imager"; }
    void onResize(int w, int h) override;
    void update(const AnalysisFrame &frame) override;
    void render() override;

    void setParam(std::string_view key, float value) override;
    nlohmann::json getParams() const override;
    void setColorScheme(const VisualizerColorScheme &scheme) override { colors_ = scheme.stereoImager; }

    ~StereoImagerVisualizer() override;

private:
    ShaderProgram shader_;
    FullscreenQuad quad_;
    GLuint tex_ = 0;

    StereoImagerColors colors_;
    std::vector<float> accum_;

    int width_ = 0;
    int height_ = 0;
    int texSize_ = 512;

    float decay_ = 0.96f;
    float brightness_ = 0.08f;
    float scale_ = 0.85f;
    float edgeSoften_ = 0.0f;
};
