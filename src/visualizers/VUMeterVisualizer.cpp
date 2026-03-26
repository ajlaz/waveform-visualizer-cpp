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
    if (!textRenderer_.init(shaderDir)) return false;
    colors_ = scheme.vu;
    return true;
}

void VUMeterVisualizer::onResize(int w, int h)
{
    width_ = w;
    height_ = h;
    textRenderer_.resize(w, h);
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
        peakDecay_ += 1.0f / 60.0f;  // accumulates time in seconds
        if (peakDecay_ > peakHold_) {
            peakDecay_ = peakHold_;   // reset so decay rate stays constant (1/60 * 0.4 dB/frame)
            peakDb_ -= (1.0f / 60.0f) * 0.4f;
            peakDb_  = std::max(peakDb_, DB_FLOOR);
        }
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

    // Draw dB labels below the tick marks
    // Bar occupies x: [BAR_X0=0.07, BAR_X1=0.93], ticks at y just below BAR_Y0=0.38
    // In top-left pixel coords: tick bottom is at (1.0-0.33)*height_ = 0.67*height_
    static constexpr float BAR_X0 = 0.07f;
    static constexpr float BAR_W  = 0.86f;   // 0.93 - 0.07
    static constexpr float DB_FLOOR_VU = -40.0f;
    static constexpr float DB_RANGE    =  43.0f; // DB_CEIL(3) - DB_FLOOR(-40)
    struct DbLabel { float db; const char* label; };
    static const DbLabel kDbLabels[] = {
        {-40.0f,"-40"},{-20.0f,"-20"},{-10.0f,"-10"},{-3.0f,"-3"},{0.0f,"0"},{3.0f,"+3"}
    };
    const float scale  = (height_ >= 200) ? 1.0f : 0.75f;
    const int   labelY = static_cast<int>(0.67f * height_) + 2;
    const float lr = colors_.ticks.r;
    const float lg = colors_.ticks.g;
    const float lb = colors_.ticks.b;
    for (const auto& lbl : kDbLabels) {
        float barX = (lbl.db - DB_FLOOR_VU) / DB_RANGE;
        int px = static_cast<int>((BAR_X0 + barX * BAR_W) * width_);
        if (lbl.db <= -40.0f) {
            textRenderer_.draw(lbl.label, px, labelY, lr, lg, lb, scale);
        } else if (lbl.db >= 3.0f) {
            int textW = static_cast<int>(2 * 8 * scale); // "+3" = 2 chars
            textRenderer_.draw(lbl.label, px - textW, labelY, lr, lg, lb, scale);
        } else {
            textRenderer_.drawCentered(lbl.label, px, labelY, lr, lg, lb, scale);
        }
    }
}

void VUMeterVisualizer::setParam(std::string_view key, float value)
{
    if (key == "peak_hold")
        peakHold_ = std::clamp(value, 0.0f, 5.0f);
}

nlohmann::json VUMeterVisualizer::getParams() const
{
    return { {"peak_hold", peakHold_} };
}

VUMeterVisualizer::~VUMeterVisualizer() = default;
