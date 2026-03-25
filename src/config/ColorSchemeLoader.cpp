#include "ColorSchemeLoader.h"

#include <fstream>
#include <iostream>
#include <vector>

#include <nlohmann/json.hpp>

#include "ColorSchemeValidation.h"

using nlohmann::json;

static int hexPairToInt(const std::string &s, size_t pos)
{
    return std::stoi(s.substr(pos, 2), nullptr, 16);
}

static void loadColor3(const json &j, const char *key, Color3 &out)
{
    const std::string s = j.at(key).get<std::string>();
    const int r = hexPairToInt(s, 1);
    const int g = hexPairToInt(s, 3);
    const int b = hexPairToInt(s, 5);
    out.r = static_cast<float>(r) / 255.0f;
    out.g = static_cast<float>(g) / 255.0f;
    out.b = static_cast<float>(b) / 255.0f;
}

static void loadFloat(const json &j, const char *key, float &out)
{
    out = j.at(key).get<float>();
}

static void loadOscilloscope(const json &j, VisualizerColorScheme &scheme)
{
    loadColor3(j, "line", scheme.oscilloscope.line);
}

static void loadSpectrum(const json &j, VisualizerColorScheme &scheme)
{
    loadColor3(j, "low", scheme.spectrum.low);
    loadColor3(j, "mid", scheme.spectrum.mid);
    loadColor3(j, "high", scheme.spectrum.high);
    loadColor3(j, "background", scheme.spectrum.background);
    loadColor3(j, "tip", scheme.spectrum.tip);
}

static void loadSpectrogram(const json &j, VisualizerColorScheme &scheme)
{
    loadColor3(j, "c0", scheme.spectrogram.c0);
    loadColor3(j, "c1", scheme.spectrogram.c1);
    loadColor3(j, "c2", scheme.spectrogram.c2);
    loadColor3(j, "c3", scheme.spectrogram.c3);
    loadColor3(j, "c4", scheme.spectrogram.c4);
    loadFloat(j, "t1", scheme.spectrogram.t1);
    loadFloat(j, "t2", scheme.spectrogram.t2);
    loadFloat(j, "t3", scheme.spectrogram.t3);
    loadFloat(j, "t4", scheme.spectrogram.t4);
}

static void loadVuMeter(const json &j, VisualizerColorScheme &scheme)
{
    loadColor3(j, "background", scheme.vu.background);
    loadColor3(j, "unfilled", scheme.vu.unfilled);
    loadColor3(j, "green", scheme.vu.green);
    loadColor3(j, "yellow", scheme.vu.yellow);
    loadColor3(j, "red", scheme.vu.red);
    loadColor3(j, "peak", scheme.vu.peak);
    loadColor3(j, "border", scheme.vu.border);
    loadColor3(j, "ticks", scheme.vu.ticks);
}

static void loadWaveform(const json &j, VisualizerColorScheme &scheme)
{
    loadColor3(j, "low", scheme.waveform.low);
    loadColor3(j, "mid", scheme.waveform.mid);
    loadColor3(j, "high", scheme.waveform.high);
    loadFloat(j, "colorGain", scheme.waveform.colorGain);
    loadFloat(j, "mixGain", scheme.waveform.mixGain);
}

static void loadStereoImager(const json &j, VisualizerColorScheme &scheme)
{
    loadColor3(j, "background", scheme.stereoImager.background);
    loadColor3(j, "dot",        scheme.stereoImager.dot);
}

bool applyColorSchemeFromFile(VisualizerColorScheme &scheme,
                              const std::string &filePath)
{
    std::ifstream f(filePath);
    if (!f.is_open())
    {
        std::cerr << "[ColorSchemes] Could not read scheme file: " << filePath << "\n";
        return false;
    }

    json root;
    try
    {
        f >> root;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ColorSchemes] Invalid JSON in " << filePath << ": " << e.what() << "\n";
        return false;
    }

    std::vector<std::string> errors;
    if (!validateColorSchemeJson(root, errors))
    {
        std::cerr << "[ColorSchemes] Invalid scheme file: " << filePath << "\n";
        for (const auto &err : errors)
        {
            std::cerr << "  - " << err << "\n";
        }
        return false;
    }

    loadOscilloscope(root.at("oscilloscope"), scheme);
    loadSpectrum(root.at("spectrum"), scheme);
    loadSpectrogram(root.at("spectrogram"), scheme);
    loadVuMeter(root.at("vu_meter"), scheme);
    loadWaveform(root.at("waveform"), scheme);
    loadStereoImager(root.at("stereo_imager"), scheme);

    return true;
}

bool applyColorSchemeFromDir(VisualizerColorScheme &scheme,
                             const std::string &dir,
                             const std::string &schemeName)
{
    const std::string filePath = dir + "/" + schemeName + ".json";
    return applyColorSchemeFromFile(scheme, filePath);
}
