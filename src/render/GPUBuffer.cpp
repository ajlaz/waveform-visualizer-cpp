#include "GPUBuffer.h"

#include <cstddef>   // offsetof

// ===========================================================================
// FullscreenQuad
// ===========================================================================

void FullscreenQuad::init()
{
    // Interleaved vec2 position + vec2 texCoord, triangle-strip order.
    static const float verts[] = {
        // pos          // uv
        -1.0f, -1.0f,   0.0f, 0.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
        -1.0f,  1.0f,   0.0f, 1.0f,
         1.0f,  1.0f,   1.0f, 1.0f,
    };

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    // location 0: position (vec2)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          /*stride=*/4 * sizeof(float),
                          /*offset=*/reinterpret_cast<void*>(0));

    // location 1: texCoord (vec2)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                          /*stride=*/4 * sizeof(float),
                          /*offset=*/reinterpret_cast<void*>(2 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void FullscreenQuad::draw() const
{
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

FullscreenQuad::~FullscreenQuad()
{
    if (vbo_ != 0) { glDeleteBuffers(1, &vbo_);       vbo_ = 0; }
    if (vao_ != 0) { glDeleteVertexArrays(1, &vao_);  vao_ = 0; }
}

// ===========================================================================
// StreamTexture1D
// ===========================================================================

void StreamTexture1D::init(int width)
{
    width_ = width;

    glGenTextures(1, &tex_);
    glBindTexture(GL_TEXTURE_1D, tex_);

    // Allocate storage; data is uploaded every frame via upload().
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RED, width, 0,
                 GL_RED, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_1D, 0);
}

void StreamTexture1D::upload(const float* data, int width)
{
    glBindTexture(GL_TEXTURE_1D, tex_);

    if (width == width_) {
        // Fast path: dimensions unchanged, update in place.
        glTexSubImage1D(GL_TEXTURE_1D, 0, 0, width,
                        GL_RED, GL_FLOAT, data);
    } else {
        // Reallocate when width changes (e.g. FFT size change).
        width_ = width;
        glTexImage1D(GL_TEXTURE_1D, 0, GL_RED, width, 0,
                     GL_RED, GL_FLOAT, data);
    }

    glBindTexture(GL_TEXTURE_1D, 0);
}

void StreamTexture1D::bind(int unit) const
{
    glActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + unit));
    glBindTexture(GL_TEXTURE_1D, tex_);
}

StreamTexture1D::~StreamTexture1D()
{
    if (tex_ != 0) { glDeleteTextures(1, &tex_); tex_ = 0; }
}

// ===========================================================================
// StreamTexture2D
// ===========================================================================

void StreamTexture2D::init(int width, int height,
                           GLenum internalFormat,
                           GLenum format,
                           GLenum type)
{
    width_   = width;
    height_  = height;
    format_  = format;
    type_    = type;

    glGenTextures(1, &tex_);
    glBindTexture(GL_TEXTURE_2D, tex_);

    // Allocate storage; pixels will be streamed via uploadColumn / uploadFull.
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internalFormat),
                 width, height, 0,
                 format, type, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void StreamTexture2D::uploadColumn(int col, const void* data) const
{
    glBindTexture(GL_TEXTURE_2D, tex_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // RGB rows are not always 4-byte aligned
    glTexSubImage2D(GL_TEXTURE_2D, 0,
                    /*xoffset=*/col, /*yoffset=*/0,
                    /*width=*/1,     /*height=*/height_,
                    format_, type_, data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void StreamTexture2D::uploadFull(const void* data) const
{
    glBindTexture(GL_TEXTURE_2D, tex_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // RGB rows are not always 4-byte aligned
    glTexSubImage2D(GL_TEXTURE_2D, 0,
                    0, 0, width_, height_,
                    format_, type_, data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void StreamTexture2D::bind(int unit) const
{
    glActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + unit));
    glBindTexture(GL_TEXTURE_2D, tex_);
}

StreamTexture2D::~StreamTexture2D()
{
    if (tex_ != 0) { glDeleteTextures(1, &tex_); tex_ = 0; }
}
