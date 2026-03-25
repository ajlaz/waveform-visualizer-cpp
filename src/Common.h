#pragma once
#include <array>
#include <vector>
#include <cstddef>

// ---------------------------------------------------------------------------
// Global constants — shared across audio, DSP, and rendering
// ---------------------------------------------------------------------------
static constexpr int SAMPLE_RATE = 44100;
static constexpr int FFT_SIZE = 8192;
static constexpr int READ_SIZE = 512;
static constexpr float FREQ_MIN = 20.0f;
static constexpr float FREQ_MAX = 16000.0f;
static constexpr float DB_MIN = -90.0f;
static constexpr float DB_MAX = 0.0f;

// IIR band crossover frequencies (Hz)
// Typical EDM ranges: lows 20-250 Hz, mids 250-6 kHz, highs 6-20 kHz.
static constexpr float BAND_LOW_CUTOFF = 250.0f;
static constexpr float BAND_HIGH_CUTOFF = 6000.0f;

// ---------------------------------------------------------------------------
// AnalysisFrame — the single data contract between DSP and rendering
// One instance is produced by AudioAnalyzer and consumed by all visualizers.
// ---------------------------------------------------------------------------
struct AnalysisFrame
{
    // Raw PCM window of FFT_SIZE samples (most recent)
    std::vector<float> samples;

    // Half-spectrum magnitudes in dB: FFT_SIZE/2+1 bins
    std::vector<float> spectrumDb;

    // Absolute peak amplitudes per band [low, mid, high], IIR smoothed
    std::array<float, 3> bandPeaks = {0.0f, 0.0f, 0.0f};

    // Per-chunk min/max samples per band (post-filter), used for solid envelopes
    std::array<float, 3> bandMins = {0.0f, 0.0f, 0.0f};
    std::array<float, 3> bandMaxs = {0.0f, 0.0f, 0.0f};

    // Per-chunk filtered samples for per-pixel windowing (waveform style)
    std::vector<float> bandSamplesLow;
    std::vector<float> bandSamplesMid;
    std::vector<float> bandSamplesHigh;

    // Smoothed RMS across all bands
    float vuLevel = 0.0f;

    // Per-channel raw PCM chunk (READ_SIZE samples each).
    // samplesR == samplesL when the capture device is mono.
    std::vector<float> samplesL;
    std::vector<float> samplesR;

    // Seconds since capture started
    double timestamp = 0.0;
};
