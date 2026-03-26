#include "VisualizerManager.h"
#include <cctype>

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

void VisualizerManager::setParam(std::string_view visName,
                                  std::string_view key, float value)
{
    for (auto& v : visualizers_)
        if (v->name() == visName) { v->setParam(key, value); return; }
}

void VisualizerManager::setColorScheme(const VisualizerColorScheme& scheme)
{
    for (auto& v : visualizers_)
        v->setColorScheme(scheme);
}

nlohmann::json VisualizerManager::getAllParams() const
{
    nlohmann::json result = nlohmann::json::object();
    for (const auto& v : visualizers_) {
        auto params = v->getParams();
        if (!params.empty()) {
            // Build JSON key: visualizer name lowercased with spaces -> underscores
            std::string key(v->name());
            for (char& c : key)
                c = (c == ' ') ? '_' : static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            result[key] = params;
        }
    }
    return result;
}
