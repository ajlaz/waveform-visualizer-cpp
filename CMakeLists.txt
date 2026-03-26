cmake_minimum_required(VERSION 3.20)
project(waveform_visualizer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Build type default
# ---------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG   "-g -O0")

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------
find_package(PkgConfig REQUIRED)

# nlohmann/json (for scheme parsing)
include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# cpp-httplib (embedded HTTP server, header-only, no OpenSSL)
set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.15.3
)
FetchContent_MakeAvailable(httplib)

# SDL2
find_package(SDL2 REQUIRED)

# PortAudio
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

# FFTW3 (single precision)
pkg_check_modules(FFTW3F REQUIRED fftw3f)
list(REMOVE_DUPLICATES FFTW3F_LIBRARIES)

# OpenGL + loader
find_package(OpenGL REQUIRED)
if(NOT APPLE)
    find_package(GLEW REQUIRED)
endif()

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(SOURCES
    src/main.cpp
    src/config/ColorSchemeLoader.cpp
    src/config/ColorSchemeValidation.cpp
    src/audio/AudioCapture.cpp
    src/audio/AudioAnalyzer.cpp
    src/render/ShaderProgram.cpp
    src/render/GPUBuffer.cpp
    src/render/TextRenderer.cpp
    src/render/Renderer.cpp
    src/render/PostProcessor.cpp
    src/visualizers/VisualizerManager.cpp
    src/visualizers/OscilloscopeVisualizer.cpp
    src/visualizers/SpectrumVisualizer.cpp
    src/visualizers/SpectrogramVisualizer.cpp
    src/visualizers/VUMeterVisualizer.cpp
    src/visualizers/WaveformVisualizer.cpp
    src/visualizers/StereoImagerVisualizer.cpp
    src/visualizers/QuadVisualizer.cpp
)

add_executable(visualizer ${SOURCES})

# ---------------------------------------------------------------------------
# Include paths
# ---------------------------------------------------------------------------
target_include_directories(visualizer PRIVATE
    src
    ${SDL2_INCLUDE_DIRS}
    ${PORTAUDIO_INCLUDE_DIRS}
    ${FFTW3F_INCLUDE_DIRS}
)

target_link_directories(visualizer PRIVATE
    ${PORTAUDIO_LIBRARY_DIRS}
    ${FFTW3F_LIBRARY_DIRS}
)

# ---------------------------------------------------------------------------
# Link libraries
# ---------------------------------------------------------------------------
target_link_libraries(visualizer PRIVATE
    OpenGL::GL
    ${SDL2_LIBRARIES}
    ${PORTAUDIO_LIBRARIES}
    ${FFTW3F_LIBRARIES}
    nlohmann_json::nlohmann_json
)

if(APPLE)
    target_link_libraries(visualizer PRIVATE
        "-framework OpenGL"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework AudioUnit"
        "-framework CoreFoundation"
        "-framework CoreServices"
    )
else()
    target_link_libraries(visualizer PRIVATE GLEW::GLEW)
endif()

# ---------------------------------------------------------------------------
# Shader path: pass it as a compile definition so main.cpp can find shaders
# ---------------------------------------------------------------------------
target_compile_definitions(visualizer PRIVATE
    SHADER_DIR="${CMAKE_SOURCE_DIR}/shaders"
)

# ---------------------------------------------------------------------------
# Compiler warnings
# ---------------------------------------------------------------------------
if(MSVC)
    target_compile_options(visualizer PRIVATE /W4)
else()
    target_compile_options(visualizer PRIVATE -Wall -Wextra -Wpedantic)
endif()
