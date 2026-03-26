#pragma once
#include "Visualizer.h"
#include "../render/ShaderProgram.h"
#include "../render/GPUBuffer.h"
#include "../render/TextRenderer.h"
#include "../ColorSchemes.h"
#include <vector>

// Spectrogram visualizer: scrolling 2D heat-map.
// Each frame writes one new row (or column) of frequency data into a
// circular 2D texture; the shader offsets UVs so the display stays continuous.
class SpectrogramVisualizer : public Visualizer
{
public:
    bool init(const std::string &shaderDir, const VisualizerColorScheme &scheme);

    std::string_view name() const override { return "Spectrogram"; }
    void onResize(int w, int h) override;
    void update(const AnalysisFrame &frame) override;
    void render() override;

    void setParam(std::string_view key, float value) override;
    nlohmann::json getParams() const override;
    void setColorScheme(const VisualizerColorScheme& scheme) override { colors_ = scheme.spectrogram; }

    ~SpectrogramVisualizer() override;

private:
    ShaderProgram shader_;
    FullscreenQuad quad_;
    StreamTexture2D scrollTex_;
    TextRenderer textRenderer_;

    SpectrogramColors colors_;

    std::vector<uint8_t> colBuf_; // one column: height_ RGB bytes
    int writeCol_ = 0;            // circular write position
    int width_ = 0;
    int height_ = 0;
    float scrollSpeed_ = 1.0f;  // columns written per frame
    float scrollAccum_ = 0.0f;  // fractional accumulator
};
