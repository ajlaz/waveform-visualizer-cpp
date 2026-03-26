#pragma once
#include "../Common.h"
#include "../ColorSchemes.h"
#include <nlohmann/json.hpp>
#include <string_view>

// ---------------------------------------------------------------------------
// Visualizer — abstract base class for all display modes.
//
// Contract:
//   onResize  — called once at startup and again on every window resize.
//               Rebuild ALL size-dependent GL resources here.
//   update    — called every frame with the latest AnalysisFrame.
//               Write to internal state. Do NOT issue GL calls here.
//   render    — called every frame after update(). Issue all GL draw calls.
//               The scene FBO is already bound; just draw.
//   setParam  — called from main loop when a remote control command arrives.
//               Default no-op; override to handle named float params.
//   getParams — returns current param values as a JSON object.
//   setColorScheme — called when user switches color scheme at runtime.
// ---------------------------------------------------------------------------
class Visualizer {
public:
    virtual ~Visualizer() = default;

    virtual std::string_view name() const = 0;

    virtual void onResize(int width, int height) = 0;
    virtual void update(const AnalysisFrame& frame) = 0;
    virtual void render() = 0;

    virtual void setParam(std::string_view /*key*/, float /*value*/) {}
    virtual nlohmann::json getParams() const { return nlohmann::json::object(); }
    virtual void setColorScheme(const VisualizerColorScheme& /*scheme*/) {}
};
