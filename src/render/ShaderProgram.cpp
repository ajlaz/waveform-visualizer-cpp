#include "ShaderProgram.h"

#include <fstream>
#include <sstream>
#include <iostream>

// ---------------------------------------------------------------------------
// Internal helper: read a whole file into a std::string.
// Returns an empty string on failure (caller checks the result).
// ---------------------------------------------------------------------------
static std::string readFile(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "[ShaderProgram] Cannot open file: " << path << "\n";
        return {};
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// ---------------------------------------------------------------------------
// Destructor
// ---------------------------------------------------------------------------
ShaderProgram::~ShaderProgram()
{
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
}

// ---------------------------------------------------------------------------
// load()
// ---------------------------------------------------------------------------
bool ShaderProgram::load(const std::string& vertPath, const std::string& fragPath)
{
    // Delete any existing program so load() can be called more than once.
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }

    const std::string vertSrc = readFile(vertPath);
    if (vertSrc.empty()) {
        std::cerr << "[ShaderProgram] Failed to read vertex shader: " << vertPath << "\n";
        return false;
    }

    const std::string fragSrc = readFile(fragPath);
    if (fragSrc.empty()) {
        std::cerr << "[ShaderProgram] Failed to read fragment shader: " << fragPath << "\n";
        return false;
    }

    GLuint vert = compileShader(GL_VERTEX_SHADER, vertSrc);
    if (vert == 0) {
        std::cerr << "[ShaderProgram] Vertex shader compilation failed: " << vertPath << "\n";
        return false;
    }

    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    if (frag == 0) {
        std::cerr << "[ShaderProgram] Fragment shader compilation failed: " << fragPath << "\n";
        glDeleteShader(vert);
        return false;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    // Shaders are linked into the program; objects are no longer needed.
    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint linkStatus = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE) {
        GLint logLen = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(static_cast<size_t>(logLen), '\0');
        glGetProgramInfoLog(prog, logLen, nullptr, log.data());
        std::cerr << "[ShaderProgram] Link error (" << vertPath << " / " << fragPath << "):\n"
                  << log << "\n";
        glDeleteProgram(prog);
        return false;
    }

    program_ = prog;
    return true;
}

// ---------------------------------------------------------------------------
// compileShader()
// ---------------------------------------------------------------------------
GLuint ShaderProgram::compileShader(GLenum type, const std::string& src) const
{
    GLuint shader = glCreateShader(type);
    const char* srcPtr = src.c_str();
    glShaderSource(shader, 1, &srcPtr, nullptr);
    glCompileShader(shader);

    GLint status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(static_cast<size_t>(logLen), '\0');
        glGetShaderInfoLog(shader, logLen, nullptr, log.data());
        std::cerr << "[ShaderProgram] Shader compile error ("
                  << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << "):\n"
                  << log << "\n";
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

// ---------------------------------------------------------------------------
// use()
// ---------------------------------------------------------------------------
void ShaderProgram::use() const
{
    glUseProgram(program_);
}

// ---------------------------------------------------------------------------
// Uniform setters
// ---------------------------------------------------------------------------
void ShaderProgram::setInt(const char* name, int v) const
{
    GLint loc = glGetUniformLocation(program_, name);
    if (loc != -1) glUniform1i(loc, v);
}

void ShaderProgram::setFloat(const char* name, float v) const
{
    GLint loc = glGetUniformLocation(program_, name);
    if (loc != -1) glUniform1f(loc, v);
}

void ShaderProgram::setVec2(const char* name, float x, float y) const
{
    GLint loc = glGetUniformLocation(program_, name);
    if (loc != -1) glUniform2f(loc, x, y);
}

void ShaderProgram::setVec3(const char* name, float x, float y, float z) const
{
    GLint loc = glGetUniformLocation(program_, name);
    if (loc != -1) glUniform3f(loc, x, y, z);
}

void ShaderProgram::setVec4(const char* name, float x, float y, float z, float w) const
{
    GLint loc = glGetUniformLocation(program_, name);
    if (loc != -1) glUniform4f(loc, x, y, z, w);
}
