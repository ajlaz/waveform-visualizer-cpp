#pragma once
#include "GLHeaders.h"
#include <string>

// ---------------------------------------------------------------------------
// Renderer
// Owns the SDL2 window, OpenGL context, and the off-screen scene FBO.
// All visualizers render into the scene FBO; the PostProcessor then
// reads it and blits to the screen (optionally applying CRT effects).
// ---------------------------------------------------------------------------
class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    // Create window + GL context + scene FBO. Returns false on failure.
    bool init(int width, int height, const std::string& title);
    void shutdown();

    // Called each frame before dispatching to the active visualizer.
    // Binds the scene FBO and clears it.
    void beginFrame();

    // Called after the PostProcessor has blitted to screen.
    // Swaps the SDL window buffers.
    void endFrame();

    // Bind the off-screen scene FBO for visualizer rendering.
    void bindSceneFBO() const;

    // Bind the default (screen) framebuffer.
    void bindDefaultFBO() const;

    // The colour attachment of the scene FBO — read by PostProcessor.
    GLuint sceneTexture() const { return sceneTex_; }
    GLuint sceneFBO()     const { return sceneFBO_; }

    // Handle window resize: recreates the scene FBO at the new size.
    void resize(int width, int height);

    int  width()  const { return width_; }
    int  height() const { return height_; }

    SDL_Window* window() const { return window_; }

private:
    void createSceneFBO(int width, int height);
    void destroySceneFBO();

    SDL_Window*   window_  = nullptr;
    SDL_GLContext context_ = nullptr;

    GLuint sceneFBO_     = 0;
    GLuint sceneTex_     = 0;
    GLuint sceneRBO_     = 0;   // depth/stencil renderbuffer

    int    width_  = 0;
    int    height_ = 0;
};
