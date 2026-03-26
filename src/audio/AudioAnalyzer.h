#pragma once
#include "../Common.h"
#include "RingBuffer.h"
#include <fftw3.h>
#include <array>
#include <atomic>
#include <mutex>
#include <vector>

// ---------------------------------------------------------------------------
// BiquadSection
// Direct Form II Transposed second-order IIR section.
// y[n] = b0*x[n] + w0
// w0   = b1*x[n] - a1*y[n] + w1
// w1   = b2*x[n] - a2*y[n]
// ---------------------------------------------------------------------------
struct BiquadSection {
    float b0 = 1, b1 = 0, b2 = 0;
    float a1 = 0, a2 = 0;
    float w0 = 0, w1 = 0;

    float process(float x) noexcept {
        const float y = b0 * x + w0;
        w0 = b1 * x - a1 * y + w1;
        w1 = b2 * x - a2 * y;
        return y;
    }

    void reset() noexcept { w0 = w1 = 0.0f; }
};

// A cascade of N second-order sections
template<int N>
struct BiquadChain {
    std::array<BiquadSection, N> sections;

    float process(float x) noexcept {
        for (auto& s : sections) x = s.process(x);
        return x;
    }

    void reset() noexcept { for (auto& s : sections) s.reset(); }
};

// ---------------------------------------------------------------------------
// AudioAnalyzer
// Consumes samples from the ring buffer, runs:
//   - FFT (Hann window, FFT_SIZE) for spectrumDb
//   - Two-section 4th-order Butterworth IIR per band for bandPeaks
//   - Smoothed RMS for vuLevel
// Publishes a completed AnalysisFrame via a mutex-protected swap so the
// render thread can always grab the latest without stalling the DSP thread.
// ---------------------------------------------------------------------------
class AudioAnalyzer {
public:
    AudioAnalyzer();
    ~AudioAnalyzer();

    // Call from the DSP thread. Drains both ring buffers, runs analysis.
    // ringR is the right channel (or the same mono buffer as ringL).
    // Returns true if a new AnalysisFrame was produced.
    bool process(RingBuffer<float>& ringL, RingBuffer<float>& ringR);

    // Thread-safe read of the latest frame (render thread calls this).
    AnalysisFrame getLatestFrame() const;

    void  setGain(float g) { gain_.store(std::clamp(g, 0.0f, 4.0f), std::memory_order_relaxed); }
    float getGain()  const { return gain_.load(std::memory_order_relaxed); }

private:
    void designFilters();
    void computeFFT();
    void computeBands(const float* chunk, size_t n);

    // FFT resources
    float*         fftIn_   = nullptr;
    fftwf_complex* fftOut_  = nullptr;
    fftwf_plan     plan_    = nullptr;
    std::vector<float> window_;         // Hann window, FFT_SIZE
    std::vector<float> pcmRing_;        // sliding PCM window, FFT_SIZE
    size_t         writePos_ = 0;
    size_t         samplesProcessed_ = 0;

    // IIR filters: 4th order Butterworth (2 biquad sections each)
    // Low:  LP @ BAND_LOW_CUTOFF
    // Mid:  LP @ BAND_HIGH_CUTOFF then HP @ BAND_LOW_CUTOFF (cascaded in process)
    // High: HP @ BAND_HIGH_CUTOFF
    BiquadChain<2> lowFilter_;
    BiquadChain<2> midLP_;          // LP at 4kHz  \  cascaded for
    BiquadChain<2> midHP_;          // HP at 600Hz /  mid band
    BiquadChain<2> highFilter_;

    // Asymmetric smoothing state
    std::array<float, 3> bandSmoothed_ = {0.0f, 0.0f, 0.0f};
    float vuSmoothed_ = 0.0f;

    // Output double-buffer
    mutable std::mutex frameMutex_;
    AnalysisFrame      latestFrame_;

    double startTime_    = 0.0;
    bool   hasStartTime_ = false;

    std::atomic<float> gain_{1.0f};
};
