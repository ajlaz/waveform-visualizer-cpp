#include "QuadVisualizer.h"
#include <cstring>

bool QuadVisualizer::init(const std::string& shaderDir,
                          std::unique_ptr<Visualizer> topLeft,
                          std::unique_ptr<Visualizer> topRight,
                          std::unique_ptr<Visualizer> bottomLeft,
                          std::unique_ptr<Visualizer> bottomRight)
{
    if (!blitShader_.load(shaderDir + "/blit.vert",
                          shaderDir + "/blit.frag"))
        return false;

    quad_.init();

    cells_[0].vis = std::move(topLeft);
    cells_[1].vis = std::move(topRight);
    cells_[2].vis = std::move(bottomLeft);
    cells_[3].vis = std::move(bottomRight);

    return true;
}

void QuadVisualizer::resizeCells(int fullW, int fullH) {
    qw_ = fullW  / 2;
    qh_ = fullH  / 2;

    for (int i = 0; i < 4; ++i) {
        auto& c = cells_[i];

        // Destroy old resources
        if (c.fbo) { glDeleteFramebuffers(1, &c.fbo);  c.fbo = 0; }
        if (c.tex) { glDeleteTextures(1, &c.tex);       c.tex = 0; }
        if (c.rbo) { glDeleteRenderbuffers(1, &c.rbo);  c.rbo = 0; }

        // Create FBO + colour texture + depth RBO
        glGenFramebuffers(1, &c.fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, c.fbo);

        glGenTextures(1, &c.tex);
        glBindTexture(GL_TEXTURE_2D, c.tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, qw_, qh_,
                     0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, c.tex, 0);

        glGenRenderbuffers(1, &c.rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, c.rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, qw_, qh_);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER, c.rbo);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        c.vis->onResize(qw_, qh_);
    }
}

void QuadVisualizer::onResize(int w, int h) {
    width_  = w;
    height_ = h;
    resizeCells(w, h);
}

void QuadVisualizer::update(const AnalysisFrame& frame) {
    for (auto& c : cells_)
        c.vis->update(frame);
}

void QuadVisualizer::render() {
    // Save the currently bound FBO (the scene FBO set by Renderer::beginFrame)
    GLint prevFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);

    // Render each sub-vis into its own FBO
    for (int i = 0; i < 4; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, cells_[i].fbo);
        glViewport(0, 0, qw_, qh_);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        cells_[i].vis->render();
    }

    // Restore the scene FBO so the blit quads go into it
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFBO));
    blitShader_.use();

    // Each quadrant: use glViewport to place the blit quad in the right screen region
    struct Quad { int x, y; };
    Quad positions[4] = {
        {0,    qh_},   // top-left
        {qw_,  qh_},   // top-right
        {0,    0},     // bottom-left
        {qw_,  0},     // bottom-right
    };

    for (int i = 0; i < 4; ++i) {
        glViewport(positions[i].x, positions[i].y, qw_, qh_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, cells_[i].tex);
        blitShader_.setInt("uScene", 0);
        quad_.draw();
    }

    // Restore full viewport
    glViewport(0, 0, width_, height_);
}

QuadVisualizer::~QuadVisualizer() {
    for (auto& c : cells_) {
        if (c.fbo) glDeleteFramebuffers(1, &c.fbo);
        if (c.tex) glDeleteTextures(1, &c.tex);
        if (c.rbo) glDeleteRenderbuffers(1, &c.rbo);
    }
}

void QuadVisualizer::setColorScheme(const VisualizerColorScheme& scheme)
{
    for (auto& c : cells_)
        c.vis->setColorScheme(scheme);
}
