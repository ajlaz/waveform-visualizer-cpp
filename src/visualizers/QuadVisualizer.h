#pragma once
#include "Visualizer.h"
#include "../render/ShaderProgram.h"
#include "../render/GPUBuffer.h"
#include <array>
#include <memory>

// QuadVisualizer: 2×2 composite of four sub-visualizers.
// Each sub-vis renders into its own FBO (QW×QH); the quad compositor
// blits all four onto the screen via a simple blit shader.
// The sub-visualizers are owned here and are independent instances —
// they share no state with the single-mode visualizers.
class QuadVisualizer : public Visualizer {
public:
    // Pass pointers to fully-init'd sub-visualizers.
    // QuadVisualizer takes ownership.
    bool init(const std::string& shaderDir,
              std::unique_ptr<Visualizer> topLeft,
              std::unique_ptr<Visualizer> topRight,
              std::unique_ptr<Visualizer> bottomLeft,
              std::unique_ptr<Visualizer> bottomRight);

    std::string_view name()     const override { return "Quad"; }
    void onResize(int w, int h) override;
    void update(const AnalysisFrame& frame) override;
    void render()               override;

    ~QuadVisualizer() override;

private:
    void resizeCells(int fullW, int fullH);

    struct Cell {
        std::unique_ptr<Visualizer> vis;
        GLuint fbo = 0;
        GLuint tex = 0;
        GLuint rbo = 0;
    };

    std::array<Cell, 4> cells_;

    ShaderProgram  blitShader_;
    FullscreenQuad quad_;

    int width_  = 0;
    int height_ = 0;
    int qw_     = 0;
    int qh_     = 0;
};
