#include "OscilloscopeVisualizer.h"
#include <cmath>
#include <algorithm>

bool OscilloscopeVisualizer::init(const std::string &shaderDir, const VisualizerColorScheme &scheme)
{
    if (!shader_.load(shaderDir + "/oscilloscope.vert",
                      shaderDir + "/oscilloscope.frag"))
        return false;

    colors_ = scheme.oscilloscope;

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    // location 0: vec2 position (NDC)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    return true;
}

void OscilloscopeVisualizer::onResize(int w, int h)
{
    width_ = w;
    height_ = h;
    vertices_.resize(w * 2); // x,y per pixel column
}

void OscilloscopeVisualizer::update(const AnalysisFrame &frame)
{
    if (width_ == 0 || frame.samples.empty())
        return;

    const auto &s = frame.samples;
    const int N = static_cast<int>(s.size());

    // Find a zero crossing for stable trigger
    int start = 0;
    int searchStart = N / 4;
    int searchEnd = N * 3 / 4;
    for (int i = searchStart; i < searchEnd - 1; ++i)
    {
        if (s[i] < 0.0f && s[i + 1] >= 0.0f)
        {
            start = i;
            break;
        }
    }

    const int requested = static_cast<int>(timeWindow_ * static_cast<float>(N - start));
    const int count = std::min(width_, std::max(2, std::min(requested, N - start)));
    vertices_.resize(count * 2);

    for (int i = 0; i < count; ++i)
    {
        float x = (static_cast<float>(i) / (count - 1)) * 2.0f - 1.0f;
        float y = std::clamp(s[start + i] * 0.85f, -1.0f, 1.0f);
        vertices_[i * 2 + 0] = x;
        vertices_[i * 2 + 1] = y;
    }
}

void OscilloscopeVisualizer::render()
{
    if (vertices_.empty())
        return;

    shader_.use();
    shader_.setVec3("uColor", colors_.line.r, colors_.line.g, colors_.line.b);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices_.size() * sizeof(float)),
                 vertices_.data(), GL_STREAM_DRAW);

    glLineWidth(1.5f);
    glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(vertices_.size() / 2));
    glBindVertexArray(0);
}

void OscilloscopeVisualizer::setParam(std::string_view key, float value)
{
    if (key == "time_window")
        timeWindow_ = std::clamp(value, 0.1f, 1.0f);
}

nlohmann::json OscilloscopeVisualizer::getParams() const
{
    return { {"time_window", timeWindow_} };
}

OscilloscopeVisualizer::~OscilloscopeVisualizer()
{
    if (vao_)
        glDeleteVertexArrays(1, &vao_);
    if (vbo_)
        glDeleteBuffers(1, &vbo_);
}
