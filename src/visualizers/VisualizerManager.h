#pragma once
#include "Visualizer.h"
#include <vector>
#include <memory>
#include <string_view>

// ---------------------------------------------------------------------------
// VisualizerManager
// Owns all registered visualizers and routes resize/update/render calls to
// whichever one is currently active.
// Adding a new visualizer: create a class, implement three methods, call
// registerVisualizer() in main — nothing else changes.
// ---------------------------------------------------------------------------
class VisualizerManager {
public:
    // Register in desired cycle order. Takes ownership.
    void registerVisualizer(std::unique_ptr<Visualizer> vis);

    void cycleNext();
    void cyclePrev();

    void onResize(int width, int height);
    void update(const AnalysisFrame& frame);
    void render();

    std::string_view activeName() const;
    size_t           activeIndex() const { return activeIdx_; }
    size_t           count()       const { return visualizers_.size(); }

private:
    std::vector<std::unique_ptr<Visualizer>> visualizers_;
    size_t activeIdx_ = 0;
};
