#include "VUMeterVisualizer.h"
#include <cmath>
#include <algorithm>

static constexpr float DB_FLOOR = -40.0f;
static constexpr float DB_CEIL = 3.0f;

bool VUMeterVisualizer::init(const std::string &shaderDir, const VisualizerColorScheme &scheme)
{
    if (!shader_.load(shaderDir + "/vu.vert", shaderDir + "/vu.frag"))
        return false;
    quad_.init();
    colors_ = scheme.vu;
    return true;
}

void VUMeterVisualizer::onResize(int w, int h)
{
    width_ = w;
    height_ = h;
}

void VUMeterVisualizer::update(const AnalysisFrame &frame)
{
    // Use pre-smoothed vuLevel from the analyzer
    const float rms = frame.vuLevel;
    const float db = 20.0f * std::log10(std::max(rms, 1e-10f));
    const float norm = std::clamp((db - DB_FLOOR) / (DB_CEIL - DB_FLOOR), 0.0f, 1.0f);
    level_ = norm;

    // Peak hold: rises instantly, decays slowly
    const float dbClamped = std::clamp(db, DB_FLOOR, DB_CEIL);
    if (dbClamped > peakDb_)
    {
        peakDb_ = dbClamped;
        peakDecay_ = 0.0f;
    }
    else
    {
        peakDecay_ += 1.0f / 60.0f; // assume ~60fps
        peakDb_ -= peakDecay_ * 0.4f;
        peakDb_ = std::max(peakDb_, DB_FLOOR);
    }
}

void VUMeterVisualizer::render()
{
    shader_.use();
    const float peakNorm = std::clamp(
        (peakDb_ - DB_FLOOR) / (DB_CEIL - DB_FLOOR), 0.0f, 1.0f);
    shader_.setFloat("uLevel", level_);
    shader_.setFloat("uPeakLevel", peakNorm);
    shader_.setVec2("uResolution",
                    static_cast<float>(width_),
                    static_cast<float>(height_));
    shader_.setVec3("uBackground", colors_.background.r, colors_.background.g, colors_.background.b);
    shader_.setVec3("uUnfilled", colors_.unfilled.r, colors_.unfilled.g, colors_.unfilled.b);
    shader_.setVec3("uGreen", colors_.green.r, colors_.green.g, colors_.green.b);
    shader_.setVec3("uYellow", colors_.yellow.r, colors_.yellow.g, colors_.yellow.b);
    shader_.setVec3("uRed", colors_.red.r, colors_.red.g, colors_.red.b);
    shader_.setVec3("uPeak", colors_.peak.r, colors_.peak.g, colors_.peak.b);
    shader_.setVec3("uBorder", colors_.border.r, colors_.border.g, colors_.border.b);
    shader_.setVec3("uTicks", colors_.ticks.r, colors_.ticks.g, colors_.ticks.b);
    quad_.draw();
}

VUMeterVisualizer::~VUMeterVisualizer() = default;
