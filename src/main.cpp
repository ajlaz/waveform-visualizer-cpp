#include "Common.h"
#include "audio/AudioCapture.h"
#include "audio/AudioAnalyzer.h"
#include "render/Renderer.h"
#include "render/PostProcessor.h"
#include "visualizers/VisualizerManager.h"
#include "visualizers/OscilloscopeVisualizer.h"
#include "visualizers/SpectrumVisualizer.h"
#include "visualizers/SpectrogramVisualizer.h"
#include "visualizers/VUMeterVisualizer.h"
#include "visualizers/WaveformVisualizer.h"
#include "visualizers/StereoImagerVisualizer.h"
#include "visualizers/QuadVisualizer.h"
#include "render/GLHeaders.h"
#include "ColorSchemes.h"
#include "config/ColorSchemeLoader.h"

#include <SDL2/SDL.h>
#include <cstdio>
#include <cstring>
#include <memory>
#include <thread>
#include <atomic>
#include <string>

#ifndef SHADER_DIR
#define SHADER_DIR "shaders"
#endif

static constexpr int INITIAL_WIDTH = 1200;
static constexpr int INITIAL_HEIGHT = 500;

// ---------------------------------------------------------------------------
// DSP thread: drains the ring buffer and publishes AnalysisFrames
// ---------------------------------------------------------------------------
static void dspThread(AudioCapture *capture,
                      AudioAnalyzer *analyzer,
                      std::atomic<bool> &running)
{
    while (running.load(std::memory_order_relaxed))
    {
        if (!analyzer->process(capture->ringBuffer(), capture->ringBufferR()))
        {
            // Ring buffer not yet full — yield to avoid busy-spin
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

// ---------------------------------------------------------------------------
// Helper: make a fully-initialised visualizer of type T
// ---------------------------------------------------------------------------
template <typename T>
static std::unique_ptr<T> makeVis(const std::string &shaderDir, int w, int h,
                                  const VisualizerColorScheme &scheme)
{
    auto v = std::make_unique<T>();
    if (!v->init(shaderDir, scheme))
    {
        std::fprintf(stderr, "Failed to init visualizer '%s'\n",
                     std::string(v->name()).c_str());
        return nullptr;
    }
    v->onResize(w, h);
    return v;
}

int main(int argc, char *argv[])
{
    // --- Argument parsing ---------------------------------------------------
    int deviceIndex = -1;
    bool listOnly = false;
    std::string schemeName = "default";
    std::string schemeDir = "color_schemes";
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--list") == 0)
        {
            listOnly = true;
        }
        else if (std::strcmp(argv[i], "--device") == 0 && i + 1 < argc)
        {
            deviceIndex = std::atoi(argv[++i]);
        }
        else if (std::strcmp(argv[i], "--scheme") == 0 && i + 1 < argc)
        {
            schemeName = argv[++i];
        }
        else if (std::strcmp(argv[i], "--scheme-dir") == 0 && i + 1 < argc)
        {
            schemeDir = argv[++i];
        }
        else
        {
            deviceIndex = std::atoi(argv[i]);
        }
    }

    // --- Audio capture (constructor calls Pa_Initialize) --------------------
    AudioCapture capture;

    // --- List devices and exit if requested ---------------------------------
    auto devices = AudioCapture::listDevices();
    std::printf("\nAvailable input devices:\n");
    for (const auto &d : devices)
        std::printf("  [%d] %s\n", d.index, d.name.c_str());
    std::printf("\n");
    if (listOnly)
        return 0;
    if (!capture.open(deviceIndex))
    {
        std::fprintf(stderr, "Failed to open audio device.\n");
        return 1;
    }

    AudioAnalyzer analyzer;

    // --- Renderer ------------------------------------------------------------
    Renderer renderer;
    if (!renderer.init(INITIAL_WIDTH, INITIAL_HEIGHT, "Waveform Visualizer"))
    {
        std::fprintf(stderr, "Failed to init renderer.\n");
        return 1;
    }

    const std::string shaderDir = SHADER_DIR;
    const int W = renderer.width();
    const int H = renderer.height();

    // --- Post-processor -------------------------------------------------------
    PostProcessor postProc;
    if (!postProc.init(shaderDir, W, H))
    {
        std::fprintf(stderr, "Failed to init post-processor.\n");
        return 1;
    }
    PostProcessor::Config ppConfig; // CRT off by default

    // --- Color scheme -------------------------------------------------------
    VisualizerColorScheme colors{};
    colors.name = schemeName;
    if (!applyColorSchemeFromDir(colors, schemeDir, schemeName))
    {
        std::fprintf(stderr, "Failed to load color scheme '%s' from %s\n",
                     schemeName.c_str(), schemeDir.c_str());
        return 1;
    }

    // --- Build visualizers ---------------------------------------------------
    // Individual visualizers for single-mode display
    auto osc = makeVis<OscilloscopeVisualizer>(shaderDir, W, H, colors);
    auto spec = makeVis<SpectrumVisualizer>(shaderDir, W, H, colors);
    auto spect = makeVis<SpectrogramVisualizer>(shaderDir, W, H, colors);
    auto vu = makeVis<VUMeterVisualizer>(shaderDir, W, H, colors);
    auto wf      = makeVis<WaveformVisualizer>    (shaderDir, W, H, colors);
    auto imager  = makeVis<StereoImagerVisualizer>(shaderDir, W, H, colors);

    if (!osc || !spec || !spect || !vu || !wf || !imager)
        return 1;

    // Quad view: four independent instances
    auto qOsc  = makeVis<OscilloscopeVisualizer>(shaderDir, W / 2, H / 2, colors);
    auto qSpec = makeVis<SpectrumVisualizer>    (shaderDir, W / 2, H / 2, colors);
    auto qWf   = makeVis<WaveformVisualizer>    (shaderDir, W / 2, H / 2, colors);
    if (!qOsc || !qSpec || !qWf)
        return 1;

    // Bottom-left: stereo imager when stereo device, VU meter otherwise
    std::unique_ptr<Visualizer> qBL;
    if (capture.isStereo()) {
        qBL = makeVis<StereoImagerVisualizer>(shaderDir, W / 2, H / 2, colors);
    } else {
        qBL = makeVis<VUMeterVisualizer>(shaderDir, W / 2, H / 2, colors);
    }
    if (!qBL)
        return 1;

    auto quad = std::make_unique<QuadVisualizer>();
    if (!quad->init(shaderDir,
                    std::move(qSpec),
                    std::move(qOsc),
                    std::move(qBL),
                    std::move(qWf)))
    {
        std::fprintf(stderr, "Failed to init quad visualizer.\n");
        return 1;
    }
    quad->onResize(W, H);

    // --- Register with manager -----------------------------------------------
    VisualizerManager manager;
    manager.registerVisualizer(std::move(spec));
    manager.registerVisualizer(std::move(spect));
    manager.registerVisualizer(std::move(osc));
    manager.registerVisualizer(std::move(vu));
    manager.registerVisualizer(std::move(wf));
    manager.registerVisualizer(std::move(imager));
    manager.registerVisualizer(std::move(quad));

    // --- Start DSP thread ----------------------------------------------------
    std::atomic<bool> running{true};
    std::thread dsp(dspThread, &capture, &analyzer, std::ref(running));

    // --- Main loop -----------------------------------------------------------
    std::printf("Controls: M = next mode  |  N = prev mode  |  F = filter  |  Esc/Q = quit\n");
    std::printf("Active: %s\n", std::string(manager.activeName()).c_str());

    bool quit = false;
    while (!quit)
    {
        // Event handling
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                quit = true;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                case SDLK_ESCAPE:
                case SDLK_q:
                    quit = true;
                    break;
                case SDLK_m:
                    manager.cycleNext();
                    std::printf("Mode: %s\n", std::string(manager.activeName()).c_str());
                    break;
                case SDLK_n:
                    manager.cyclePrev();
                    std::printf("Mode: %s\n", std::string(manager.activeName()).c_str());
                    break;
                case SDLK_f:
                    if (ppConfig.filter == PostProcessor::FilterMode::None)
                        ppConfig.filter = PostProcessor::FilterMode::CRT;
                    else if (ppConfig.filter == PostProcessor::FilterMode::CRT)
                        ppConfig.filter = PostProcessor::FilterMode::Fisheye;
                    else
                        ppConfig.filter = PostProcessor::FilterMode::None;
                    std::printf("Filter: %s\n",
                                ppConfig.filter == PostProcessor::FilterMode::CRT ? "CRT" : ppConfig.filter == PostProcessor::FilterMode::Fisheye ? "Fisheye"
                                                                                                                                                  : "None");
                    break;
                default:
                    break;
                }
                break;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                {
                    int nw = event.window.data1;
                    int nh = event.window.data2;
                    renderer.resize(nw, nh);
                    postProc.resize(renderer.width(), renderer.height());
                    manager.onResize(renderer.width(), renderer.height());
                }
                break;

            default:
                break;
            }
        }

        // Get latest DSP frame
        AnalysisFrame frame = analyzer.getLatestFrame();

        // Render into scene FBO
        renderer.beginFrame(); // binds scene FBO, clears
        manager.update(frame);
        manager.render();

        // Post-process and blit to screen
        postProc.render(renderer.sceneTexture(), ppConfig);

        renderer.endFrame(); // SDL_GL_SwapWindow
    }

    // --- Cleanup -------------------------------------------------------------
    running = false;
    dsp.join();
    capture.close();
    renderer.shutdown();

    return 0;
}
