#pragma once
#include "../Common.h"
#include <string_view>

// ---------------------------------------------------------------------------
// Visualizer — abstract base class for all display modes.
//
// Contract:
//   onResize  — called once at startup and again on every window resize.
//               Rebuild ALL size-dependent GL resources here (textures,
//               VBOs, projection matrices). Never allocate GL objects in
//               render().
//   update    — called every frame with the latest AnalysisFrame.
//               Write to internal state. Do NOT issue GL calls here.
//   render    — called every frame after update(). Issue all GL draw calls.
//               The scene FBO is already bound; just draw.
// ---------------------------------------------------------------------------
class Visualizer {
public:
    virtual ~Visualizer() = default;

    virtual std::string_view name() const = 0;

    virtual void onResize(int width, int height) = 0;
    virtual void update(const AnalysisFrame& frame) = 0;
    virtual void render() = 0;
};
