#pragma once
// ---------------------------------------------------------------------------
// Single include point for OpenGL headers.
// macOS:        OpenGL 3.3 core via Apple's framework (no loader needed).
// Linux / RPi:  OpenGL 3.3 core via GLEW (must call glewInit() after context).
// ---------------------------------------------------------------------------
#ifdef __APPLE__
#  define GL_SILENCE_DEPRECATION
#  include <OpenGL/gl3.h>
#else
#  include <GL/glew.h>
#endif

#include <SDL2/SDL.h>
