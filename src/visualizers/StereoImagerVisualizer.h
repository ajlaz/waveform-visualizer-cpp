#pragma once
#include "Visualizer.h"
#include "../render/ShaderProgram.h"
#include "../render/GPUBuffer.h"
#include "../ColorSchemes.h"
#include <vector>

// Goniometer / Lissajous stereo imager.
// X axis = (L-R)/2  (side signal)   — right = more L, left = more R
// Y axis = (L+R)/2  (mid signal)    — up = louder
// A CPU-side float accumulation buffer decays each frame and is uploaded
// to a GL_R32F texture; the fragment shader applies a colour gradient.
class StereoImagerVisualizer : public Visualizer
{
public:
    bool init(const std::string &shaderDir, const VisualizerColorScheme &scheme);

    std::string_view name() const override { return "StereoImager"; }
    void onResize(int w, int h) override;
    void update(const AnalysisFrame &frame) override;
    void render() override;

    ~StereoImagerVisualizer() override;

private:
    ShaderProgram shader_;
    FullscreenQuad quad_;
    GLuint         tex_  = 0;  // GL_R32F accumulation texture

    StereoImagerColors colors_;

    std::vector<float> accum_;  // TEX_SIZE × TEX_SIZE float buffer
    int width_  = 0;
    int height_ = 0;

    static constexpr int   TEX_SIZE = 512;
    static constexpr float DECAY    = 0.96f;   // per-frame fade
    static constexpr float ADD      = 0.08f;   // brightness per sample hit
    static constexpr float SCALE    = 0.85f;   // amplitude → normalised coords
};
