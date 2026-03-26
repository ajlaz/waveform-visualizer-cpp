#pragma once
#include "Visualizer.h"
#include "../render/ShaderProgram.h"
#include "../ColorSchemes.h"
#include <vector>

class OscilloscopeVisualizer : public Visualizer
{
public:
    bool init(const std::string &shaderDir, const VisualizerColorScheme &scheme);

    std::string_view name() const override { return "Oscilloscope"; }
    void onResize(int w, int h) override;
    void update(const AnalysisFrame &frame) override;
    void render() override;

    void setParam(std::string_view key, float value) override;
    nlohmann::json getParams() const override;
    void setColorScheme(const VisualizerColorScheme& scheme) override { colors_ = scheme.oscilloscope; }

    ~OscilloscopeVisualizer() override;

private:
    ShaderProgram shader_;

    OscilloscopeColors colors_;

    GLuint vao_ = 0;
    GLuint vbo_ = 0;

    std::vector<float> vertices_; // interleaved x,y pairs
    int width_ = 0;
    int height_ = 0;
    float timeWindow_ = 1.0f;  // fraction of available samples to display
};
