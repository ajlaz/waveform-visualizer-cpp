#pragma once
#include "GLHeaders.h"
#include <cstddef>

// ---------------------------------------------------------------------------
// FullscreenQuad
// A unit quad covering NDC [-1,+1]² with UV [0,1]².
// Used by spectrum, waveform, post-processor, and any visualizer that
// renders via a fullscreen fragment shader.
// Vertex layout: location 0 = vec2 position, location 1 = vec2 texCoord.
// ---------------------------------------------------------------------------
class FullscreenQuad
{
public:
    void init();
    void draw() const;
    ~FullscreenQuad();

private:
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
};

// ---------------------------------------------------------------------------
// StreamTexture1D
// A 1D GL_RED texture updated each frame from a CPU float array.
// Used by the spectrum visualizer to upload spectrumDb per-column.
// ---------------------------------------------------------------------------
class StreamTexture1D
{
public:
    void init(int width);
    void upload(const float *data, int width);
    void bind(int unit = 0) const;
    ~StreamTexture1D();

private:
    GLuint tex_ = 0;
    int width_ = 0;
};

// ---------------------------------------------------------------------------
// StreamTexture2D
// A 2D RGBA/RGB texture updated via glTexSubImage2D.
// Used by the waveform scrolling waveform.
// Supports circular-buffer rendering: one column written per frame,
// a shader offset uniform handles the display wrap-around.
// ---------------------------------------------------------------------------
class StreamTexture2D
{
public:
    // format: GL_RGB or GL_RGBA, type: GL_UNSIGNED_BYTE or GL_FLOAT
    void init(int width, int height, GLenum internalFormat = GL_RGB,
              GLenum format = GL_RGB, GLenum type = GL_UNSIGNED_BYTE);

    // Upload a single column of `height` pixels at x position `col`.
    void uploadColumn(int col, const void *data) const;

    // Upload the entire texture at once.
    void uploadFull(const void *data) const;

    void bind(int unit = 0) const;
    int width() const { return width_; }
    int height() const { return height_; }
    ~StreamTexture2D();

private:
    GLuint tex_ = 0;
    int width_ = 0;
    int height_ = 0;
    GLenum format_ = GL_RGB;
    GLenum type_ = GL_UNSIGNED_BYTE;
};
