#pragma once
#include "RingBuffer.h"
#include <portaudio.h>
#include <string>
#include <vector>
#include <memory>

// ---------------------------------------------------------------------------
// AudioCapture
// Wraps a PortAudio input stream. Captured float32 mono samples are pushed
// into the ring buffer from within the real-time callback — no allocation,
// no blocking, no mutex.
// ---------------------------------------------------------------------------
class AudioCapture {
public:
    struct DeviceInfo {
        int         index;
        std::string name;
        int         maxInputChannels;
        double      defaultSampleRate;
    };

    AudioCapture();
    ~AudioCapture();

    // Enumerate all PortAudio input devices.
    static std::vector<DeviceInfo> listDevices();

    // Open the capture stream. Pass -1 to use the system default input.
    // Returns false on failure (error printed to stderr).
    bool open(int deviceIndex = -1);
    void close();

    bool isOpen() const { return stream_ != nullptr; }

    // The ring buffer written by the callback. Read from the DSP thread.
    RingBuffer<float>& ringBuffer() { return *ring_; }

private:
    static int paCallback(const void*                     inputBuffer,
                          void*                           outputBuffer,
                          unsigned long                   framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags           statusFlags,
                          void*                           userData);

    PaStream*                         stream_ = nullptr;
    std::unique_ptr<RingBuffer<float>> ring_;
};
