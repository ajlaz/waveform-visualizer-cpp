#pragma once
#include <string>

struct Color3
{
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
};

struct OscilloscopeColors
{
    Color3 line;
};

struct SpectrumColors
{
    Color3 low;
    Color3 mid;
    Color3 high;
    Color3 background;
    Color3 tip;
};

struct SpectrogramColors
{
    Color3 c0;
    Color3 c1;
    Color3 c2;
    Color3 c3;
    Color3 c4;
    float t1 = 0.0f;
    float t2 = 0.0f;
    float t3 = 0.0f;
    float t4 = 0.0f;
};

struct VUMeterColors
{
    Color3 background;
    Color3 unfilled;
    Color3 green;
    Color3 yellow;
    Color3 red;
    Color3 peak;
    Color3 border;
    Color3 ticks;
};

struct WaveformColors
{
    Color3 low;
    Color3 mid;
    Color3 high;
    float colorGain = 0.0f;
    float mixGain = 0.0f;
};

struct StereoImagerColors
{
    Color3 background;
    Color3 dot;
};

struct VisualizerColorScheme
{
    std::string name;
    OscilloscopeColors oscilloscope;
    SpectrumColors spectrum;
    SpectrogramColors spectrogram;
    VUMeterColors vu;
    WaveformColors waveform;
    StereoImagerColors stereoImager;
};
