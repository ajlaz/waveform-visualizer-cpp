#include "AudioAnalyzer.h"
#include "../Common.h"

#include <fftw3.h>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <array>

// ---------------------------------------------------------------------------
// Asymmetric smoothing constants
// ---------------------------------------------------------------------------
static constexpr float ATTACK = 0.8f;
static constexpr float DECAY = 0.12f;

// ---------------------------------------------------------------------------
// Butterworth filter design helpers (file-local)
// ---------------------------------------------------------------------------

// Returns 2 biquad sections for a 4th order Butterworth lowpass at fc Hz, fs Hz.
// Q values for N=4: Q1=1/2sin(π/8)≈1.3066, Q2=1/2sin(3π/8)≈0.5412
static std::array<BiquadSection, 2> butterLP4(float fc, float fs)
{
    const float wc = 2.0f * std::tan(static_cast<float>(M_PI) * fc / fs);
    const float wc2 = wc * wc;
    const float Qs[2] = {1.3066f, 0.5412f};
    std::array<BiquadSection, 2> out;
    for (int k = 0; k < 2; k++)
    {
        const float wc_Q = wc / Qs[k];
        const float a0 = 4.0f + 2.0f * wc_Q + wc2;
        out[k].b0 = wc2 / a0;
        out[k].b1 = 2.0f * wc2 / a0;
        out[k].b2 = wc2 / a0;
        out[k].a1 = (2.0f * wc2 - 8.0f) / a0;
        out[k].a2 = (4.0f - 2.0f * wc_Q + wc2) / a0;
    }
    return out;
}

// Returns 2 biquad sections for a 4th order Butterworth highpass at fc Hz, fs Hz.
static std::array<BiquadSection, 2> butterHP4(float fc, float fs)
{
    const float wc = 2.0f * std::tan(static_cast<float>(M_PI) * fc / fs);
    const float wc2 = wc * wc;
    const float Qs[2] = {1.3066f, 0.5412f};
    std::array<BiquadSection, 2> out;
    for (int k = 0; k < 2; k++)
    {
        const float wc_Q = wc / Qs[k];
        const float a0 = 4.0f + 2.0f * wc_Q + wc2;
        out[k].b0 = 4.0f / a0;
        out[k].b1 = -8.0f / a0;
        out[k].b2 = 4.0f / a0;
        out[k].a1 = (2.0f * wc2 - 8.0f) / a0;
        out[k].a2 = (4.0f - 2.0f * wc_Q + wc2) / a0;
    }
    return out;
}

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

AudioAnalyzer::AudioAnalyzer()
{
    // Allocate FFTW resources
    fftIn_ = fftwf_alloc_real(FFT_SIZE);
    fftOut_ = fftwf_alloc_complex(FFT_SIZE / 2 + 1);
    plan_ = fftwf_plan_dft_r2c_1d(FFT_SIZE, fftIn_, fftOut_, FFTW_MEASURE);

    // Build Hann window
    window_.resize(FFT_SIZE);
    for (int i = 0; i < FFT_SIZE; ++i)
    {
        window_[i] = 0.5f * (1.0f - std::cos(2.0f * static_cast<float>(M_PI) * i / static_cast<float>(FFT_SIZE - 1)));
    }

    // Initialise sliding PCM ring with silence
    pcmRing_.assign(FFT_SIZE, 0.0f);
    writePos_ = 0;
    samplesProcessed_ = 0;

    // Initialise latestFrame_ with correct sizes
    latestFrame_.samples.assign(FFT_SIZE, 0.0f);
    latestFrame_.spectrumDb.assign(FFT_SIZE / 2 + 1, DB_MIN);

    // Design IIR filters
    designFilters();

    // Record start time
    using Clock = std::chrono::steady_clock;
    startTime_ = std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
    hasStartTime_ = true;
}

AudioAnalyzer::~AudioAnalyzer()
{
    if (plan_ != nullptr)
    {
        fftwf_destroy_plan(plan_);
        plan_ = nullptr;
    }
    if (fftOut_ != nullptr)
    {
        fftwf_free(fftOut_);
        fftOut_ = nullptr;
    }
    if (fftIn_ != nullptr)
    {
        fftwf_free(fftIn_);
        fftIn_ = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Filter design
// ---------------------------------------------------------------------------

void AudioAnalyzer::designFilters()
{
    lowFilter_.sections = butterLP4(BAND_LOW_CUTOFF, static_cast<float>(SAMPLE_RATE));
    midLP_.sections = butterLP4(BAND_HIGH_CUTOFF, static_cast<float>(SAMPLE_RATE));
    midHP_.sections = butterHP4(BAND_LOW_CUTOFF, static_cast<float>(SAMPLE_RATE));
    highFilter_.sections = butterHP4(BAND_HIGH_CUTOFF, static_cast<float>(SAMPLE_RATE));

    lowFilter_.reset();
    midLP_.reset();
    midHP_.reset();
    highFilter_.reset();
}

// ---------------------------------------------------------------------------
// Main DSP entry point
// ---------------------------------------------------------------------------

bool AudioAnalyzer::process(RingBuffer<float> &ring)
{
    if (ring.available() < static_cast<size_t>(READ_SIZE))
        return false;

    // Drain exactly READ_SIZE samples
    float buf[READ_SIZE];
    size_t count = ring.read(buf, READ_SIZE);
    if (count == 0)
        return false;

    // Update band peaks / VU from this chunk
    computeBands(buf, count);

    // Write samples into the sliding PCM window (circular)
    for (size_t i = 0; i < count; ++i)
    {
        pcmRing_[writePos_] = buf[i];
        writePos_ = (writePos_ + 1) % static_cast<size_t>(FFT_SIZE);
    }

    samplesProcessed_ += count;

    // Recompute FFT once per chunk
    computeFFT();

    return true;
}

// ---------------------------------------------------------------------------
// Band peak / VU smoothing
// ---------------------------------------------------------------------------

void AudioAnalyzer::computeBands(const float *chunk, size_t n)
{
    float sumSq[3] = {0.0f, 0.0f, 0.0f};
    float bandMin[3] = {1.0f, 1.0f, 1.0f};
    float bandMax[3] = {-1.0f, -1.0f, -1.0f};
    std::vector<float> lowSamples(n);
    std::vector<float> midSamples(n);
    std::vector<float> highSamples(n);

    for (size_t i = 0; i < n; ++i)
    {
        const float s = chunk[i];

        const float lowOut = lowFilter_.process(s);
        const float midOut = midHP_.process(midLP_.process(s));
        const float highOut = highFilter_.process(s);

        lowSamples[i] = lowOut;
        midSamples[i] = midOut;
        highSamples[i] = highOut;

        sumSq[0] += lowOut * lowOut;
        sumSq[1] += midOut * midOut;
        sumSq[2] += highOut * highOut;

        bandMin[0] = std::min(bandMin[0], lowOut);
        bandMin[1] = std::min(bandMin[1], midOut);
        bandMin[2] = std::min(bandMin[2], highOut);
        bandMax[0] = std::max(bandMax[0], lowOut);
        bandMax[1] = std::max(bandMax[1], midOut);
        bandMax[2] = std::max(bandMax[2], highOut);
    }

    float rawPeak[3] = {
        std::sqrt(sumSq[0] / static_cast<float>(n)),
        std::sqrt(sumSq[1] / static_cast<float>(n)),
        std::sqrt(sumSq[2] / static_cast<float>(n))};

    // Asymmetric smoothing per band
    for (int b = 0; b < 3; ++b)
    {
        float &sm = bandSmoothed_[b];
        if (rawPeak[b] > sm)
            sm += ATTACK * (rawPeak[b] - sm);
        else
            sm += DECAY * (rawPeak[b] - sm);
    }

    // VU = average of the three smoothed peaks
    const float rawVu = (bandSmoothed_[0] + bandSmoothed_[1] + bandSmoothed_[2]) / 3.0f;
    if (rawVu > vuSmoothed_)
        vuSmoothed_ += ATTACK * (rawVu - vuSmoothed_);
    else
        vuSmoothed_ += DECAY * (rawVu - vuSmoothed_);

    // Publish to latestFrame_
    {
        std::lock_guard<std::mutex> lk(frameMutex_);
        latestFrame_.bandPeaks[0] = bandSmoothed_[0];
        latestFrame_.bandPeaks[1] = bandSmoothed_[1];
        latestFrame_.bandPeaks[2] = bandSmoothed_[2];
        latestFrame_.bandMins[0] = bandMin[0];
        latestFrame_.bandMins[1] = bandMin[1];
        latestFrame_.bandMins[2] = bandMin[2];
        latestFrame_.bandMaxs[0] = bandMax[0];
        latestFrame_.bandMaxs[1] = bandMax[1];
        latestFrame_.bandMaxs[2] = bandMax[2];
        latestFrame_.bandSamplesLow = std::move(lowSamples);
        latestFrame_.bandSamplesMid = std::move(midSamples);
        latestFrame_.bandSamplesHigh = std::move(highSamples);
        latestFrame_.vuLevel = vuSmoothed_;
    }
}

// ---------------------------------------------------------------------------
// FFT + spectrum conversion
// ---------------------------------------------------------------------------

void AudioAnalyzer::computeFFT()
{
    // Copy pcmRing_ into fftIn_ in chronological order starting from writePos_
    // (writePos_ is the next write slot, so it points to the oldest sample)
    for (int i = 0; i < FFT_SIZE; ++i)
    {
        const size_t src = (writePos_ + static_cast<size_t>(i)) % static_cast<size_t>(FFT_SIZE);
        fftIn_[i] = pcmRing_[src] * window_[i];
    }

    fftwf_execute(plan_);

    const int numBins = FFT_SIZE / 2 + 1;
    std::vector<float> spectrumDb(numBins);

    for (int i = 0; i < numBins; ++i)
    {
        const float re = fftOut_[i][0];
        const float im = fftOut_[i][1];
        const float mag = std::sqrt(re * re + im * im) / static_cast<float>(FFT_SIZE);
        float db = 20.0f * std::log10(std::max(mag, 1e-10f));
        db = std::max(db, DB_MIN);
        db = std::min(db, DB_MAX);
        spectrumDb[i] = db;
    }

    // Compute timestamp
    double ts = 0.0;
    if (hasStartTime_)
    {
        using Clock = std::chrono::steady_clock;
        double now = std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
        ts = now - startTime_;
    }

    // Copy the current PCM window out in chronological order
    std::vector<float> samples(FFT_SIZE);
    for (int i = 0; i < FFT_SIZE; ++i)
    {
        const size_t src = (writePos_ + static_cast<size_t>(i)) % static_cast<size_t>(FFT_SIZE);
        samples[i] = pcmRing_[src];
    }

    {
        std::lock_guard<std::mutex> lk(frameMutex_);
        latestFrame_.spectrumDb = std::move(spectrumDb);
        latestFrame_.samples = std::move(samples);
        latestFrame_.timestamp = ts;
    }
}

// ---------------------------------------------------------------------------
// Thread-safe frame read
// ---------------------------------------------------------------------------

AnalysisFrame AudioAnalyzer::getLatestFrame() const
{
    std::lock_guard<std::mutex> lk(frameMutex_);
    return latestFrame_;
}
