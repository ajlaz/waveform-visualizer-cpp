#pragma once
#include "GLHeaders.h"
#include "ShaderProgram.h"
#include <string>

// Renders ASCII text as pixel-art quads using a built-in 8×8 bitmap font.
// Coordinates use top-left origin (x right, y down), matching screen pixels.
class TextRenderer {
public:
    bool init(const std::string& shaderDir);
    void resize(int vpW, int vpH);

    // Draw text at pixel position (x, y from top-left). scale=1 → 8×8px per glyph.
    void draw(const std::string& text, int x, int y,
              float r, float g, float b, float scale = 1.0f) const;
    // Horizontally center the text on pixel column cx.
    void drawCentered(const std::string& text, int cx, int y,
                      float r, float g, float b, float scale = 1.0f) const;

    ~TextRenderer();

private:
    void buildAtlas();

    ShaderProgram shader_;
    GLuint fontTex_ = 0;
    GLuint vao_     = 0;
    GLuint vbo_     = 0;
    int vpW_ = 1, vpH_ = 1;

    static constexpr int kCharW    = 8;
    static constexpr int kCharH    = 8;
    static constexpr int kNumChars = 95; // printable ASCII 32–126
};
