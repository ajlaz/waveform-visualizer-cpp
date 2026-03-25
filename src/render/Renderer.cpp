#include "Renderer.h"

#include <iostream>

// ---------------------------------------------------------------------------
// Destructor — shutdown() is idempotent so calling it twice is safe.
// ---------------------------------------------------------------------------
Renderer::~Renderer()
{
    shutdown();
}

// ---------------------------------------------------------------------------
// init()
// ---------------------------------------------------------------------------
bool Renderer::init(int width, int height, const std::string &title)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cerr << "[Renderer] SDL_Init failed: " << SDL_GetError() << "\n";
        return false;
    }

    // Request OpenGL 3.3 core profile.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    window_ = SDL_CreateWindow(title.c_str(),
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               width, height,
                               SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window_)
    {
        std::cerr << "[Renderer] SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return false;
    }

    context_ = SDL_GL_CreateContext(window_);
    if (!context_)
    {
        std::cerr << "[Renderer] SDL_GL_CreateContext failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window_);
        window_ = nullptr;
        SDL_Quit();
        return false;
    }

#ifndef __APPLE__
    // GLEW must be initialised after a valid context exists.
    glewExperimental = GL_TRUE;
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK)
    {
        std::cerr << "[Renderer] glewInit failed: "
                  << reinterpret_cast<const char *>(glewGetErrorString(glewErr)) << "\n";
        SDL_GL_DeleteContext(context_);
        SDL_DestroyWindow(window_);
        context_ = nullptr;
        window_ = nullptr;
        SDL_Quit();
        return false;
    }
    // GLEW sometimes generates a benign GL_INVALID_ENUM on init; clear it.
    glGetError();
#endif

    // Alpha blending for all visualizer passes.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Vsync.
    SDL_GL_SetSwapInterval(1);

    SDL_GL_GetDrawableSize(window_, &width_, &height_);

    createSceneFBO(width_, height_);
    return true;
}

// ---------------------------------------------------------------------------
// shutdown()
// ---------------------------------------------------------------------------
void Renderer::shutdown()
{
    destroySceneFBO();

    if (context_)
    {
        SDL_GL_DeleteContext(context_);
        context_ = nullptr;
    }
    if (window_)
    {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    SDL_Quit();
}

// ---------------------------------------------------------------------------
// Frame control
// ---------------------------------------------------------------------------
void Renderer::beginFrame()
{
    bindSceneFBO();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::endFrame()
{
    bindDefaultFBO();
    SDL_GL_SwapWindow(window_);
}

// ---------------------------------------------------------------------------
// FBO binding helpers
// ---------------------------------------------------------------------------
void Renderer::bindSceneFBO() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO_);
    glViewport(0, 0, width_, height_);
}

void Renderer::bindDefaultFBO() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width_, height_);
}

// ---------------------------------------------------------------------------
// resize()
// ---------------------------------------------------------------------------
void Renderer::resize(int width, int height)
{
    (void)width;
    (void)height;
    SDL_GL_GetDrawableSize(window_, &width_, &height_);
    destroySceneFBO();
    createSceneFBO(width_, height_);
    glViewport(0, 0, width_, height_);
}

// ---------------------------------------------------------------------------
// createSceneFBO()
// ---------------------------------------------------------------------------
void Renderer::createSceneFBO(int width, int height)
{
    // --- Colour attachment (RGB texture) ---
    glGenTextures(1, &sceneTex_);
    glBindTexture(GL_TEXTURE_2D, sceneTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                 width, height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // --- Depth renderbuffer ---
    glGenRenderbuffers(1, &sceneRBO_);
    glBindRenderbuffer(GL_RENDERBUFFER, sceneRBO_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // --- FBO ---
    glGenFramebuffers(1, &sceneFBO_);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO_);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, sceneTex_, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, sceneRBO_);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "[Renderer] Scene FBO is not complete: 0x"
                  << std::hex << status << std::dec << "\n";
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ---------------------------------------------------------------------------
// destroySceneFBO()
// ---------------------------------------------------------------------------
void Renderer::destroySceneFBO()
{
    if (sceneFBO_)
    {
        glDeleteFramebuffers(1, &sceneFBO_);
        sceneFBO_ = 0;
    }
    if (sceneTex_)
    {
        glDeleteTextures(1, &sceneTex_);
        sceneTex_ = 0;
    }
    if (sceneRBO_)
    {
        glDeleteRenderbuffers(1, &sceneRBO_);
        sceneRBO_ = 0;
    }
}
