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

    // Left (or mono) ring buffer written by the callback.
    RingBuffer<float>& ringBuffer()  { return *ring_;  }
    // Right channel ring buffer (mirrors ringBuffer() when device is mono).
    RingBuffer<float>& ringBufferR() { return *ringR_; }

    bool isStereo() const { return isStereo_; }

private:
    static int paCallback(const void*                     inputBuffer,
                          void*                           outputBuffer,
                          unsigned long                   framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags           statusFlags,
                          void*                           userData);

    PaStream*                          stream_   = nullptr;
    std::unique_ptr<RingBuffer<float>> ring_;
    std::unique_ptr<RingBuffer<float>> ringR_;
    bool                               isStereo_ = false;
};
