#include "VisualizerManager.h"
#include <stdexcept>

void VisualizerManager::registerVisualizer(std::unique_ptr<Visualizer> vis) {
    visualizers_.push_back(std::move(vis));
}

void VisualizerManager::cycleNext() {
    if (visualizers_.empty()) return;
    activeIdx_ = (activeIdx_ + 1) % visualizers_.size();
}

void VisualizerManager::cyclePrev() {
    if (visualizers_.empty()) return;
    activeIdx_ = (activeIdx_ + visualizers_.size() - 1) % visualizers_.size();
}

void VisualizerManager::onResize(int width, int height) {
    for (auto& v : visualizers_)
        v->onResize(width, height);
}

void VisualizerManager::update(const AnalysisFrame& frame) {
    if (visualizers_.empty()) return;
    visualizers_[activeIdx_]->update(frame);
}

void VisualizerManager::render() {
    if (visualizers_.empty()) return;
    visualizers_[activeIdx_]->render();
}

std::string_view VisualizerManager::activeName() const {
    if (visualizers_.empty()) return "none";
    return visualizers_[activeIdx_]->name();
}
