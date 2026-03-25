#pragma once
#include "GLHeaders.h"
#include <string>

// ---------------------------------------------------------------------------
// ShaderProgram
// Compiles and links a GLSL vertex + fragment shader pair loaded from disk.
// Uniform setters cache locations for repeated use.
// ---------------------------------------------------------------------------
class ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram();

    // Load, compile and link shaders from file paths.
    // Returns false and prints errors to stderr on failure.
    bool load(const std::string& vertPath, const std::string& fragPath);

    void use() const;
    bool isValid() const { return program_ != 0; }

    // Uniform setters — silently no-op if the name doesn't exist.
    void setInt  (const char* name, int v)                            const;
    void setFloat(const char* name, float v)                          const;
    void setVec2 (const char* name, float x, float y)                const;
    void setVec3 (const char* name, float x, float y, float z)       const;
    void setVec4 (const char* name, float x, float y, float z, float w) const;

    GLuint id() const { return program_; }

private:
    GLuint compileShader(GLenum type, const std::string& src) const;

    GLuint program_ = 0;
};
