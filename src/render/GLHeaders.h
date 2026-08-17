#pragma once
// ---------------------------------------------------------------------------
// Single include point for OpenGL headers.
// macOS:        OpenGL 3.3 core via Apple's framework (no loader needed).
// Linux / RPi:  OpenGL 3.3 core via glad (must call gladLoadGL() after context).
//               glad is used instead of GLEW because GLEW's extension
//               detection assumes a GLX context and fails with
//               "Unknown error" under EGL-only contexts (e.g. SDL's
//               kmsdrm backend on a headless Pi with no X server).
// ---------------------------------------------------------------------------
#ifdef __APPLE__
#  define GL_SILENCE_DEPRECATION
#  include <OpenGL/gl3.h>
#else
#  include <glad/gl.h>
#endif

#include <SDL2/SDL.h>
