#include "AudioCapture.h"
#include "../Common.h"

#include <portaudio.h>
#include <iostream>
#include <cstdio>

static constexpr size_t RING_CAPACITY = 65536; // power-of-two, ~1.5 s at 44100 Hz

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

AudioCapture::AudioCapture()
    : stream_(nullptr)
    , ring_(std::make_unique<RingBuffer<float>>(RING_CAPACITY))
{
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "[AudioCapture] Pa_Initialize failed: "
                  << Pa_GetErrorText(err) << "\n";
    }
}

AudioCapture::~AudioCapture()
{
    close();
    Pa_Terminate();
}

// ---------------------------------------------------------------------------
// Device enumeration
// ---------------------------------------------------------------------------

std::vector<AudioCapture::DeviceInfo> AudioCapture::listDevices()
{
    std::vector<DeviceInfo> devices;

    int count = Pa_GetDeviceCount();
    if (count < 0) {
        std::cerr << "[AudioCapture] Pa_GetDeviceCount error: "
                  << Pa_GetErrorText(static_cast<PaError>(count)) << "\n";
        return devices;
    }

    for (int i = 0; i < count; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info == nullptr)
            continue;
        if (info->maxInputChannels > 0) {
            DeviceInfo d;
            d.index             = i;
            d.name              = info->name ? info->name : "";
            d.maxInputChannels  = info->maxInputChannels;
            d.defaultSampleRate = info->defaultSampleRate;
            devices.push_back(d);
        }
    }
    return devices;
}

// ---------------------------------------------------------------------------
// Stream open / close
// ---------------------------------------------------------------------------

bool AudioCapture::open(int deviceIndex)
{
    if (stream_ != nullptr) {
        std::cerr << "[AudioCapture] Stream already open; call close() first.\n";
        return false;
    }

    // Resolve device
    PaDeviceIndex paDevice = (deviceIndex == -1)
                             ? Pa_GetDefaultInputDevice()
                             : static_cast<PaDeviceIndex>(deviceIndex);

    if (paDevice == paNoDevice) {
        std::cerr << "[AudioCapture] No input device available.\n";
        return false;
    }

    const PaDeviceInfo* devInfo = Pa_GetDeviceInfo(paDevice);
    if (devInfo == nullptr) {
        std::cerr << "[AudioCapture] Invalid device index " << paDevice << ".\n";
        return false;
    }

    std::cout << "[AudioCapture] Opening device: " << devInfo->name << "\n";

    PaStreamParameters inputParams;
    inputParams.device                    = paDevice;
    inputParams.channelCount              = 1;               // mono
    inputParams.sampleFormat              = paFloat32;
    inputParams.suggestedLatency          = devInfo->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = nullptr;

    PaError err = Pa_OpenStream(
        &stream_,
        &inputParams,
        nullptr,                 // no output
        static_cast<double>(SAMPLE_RATE),
        static_cast<unsigned long>(READ_SIZE),
        paClipOff,
        &AudioCapture::paCallback,
        this
    );

    if (err != paNoError) {
        std::cerr << "[AudioCapture] Pa_OpenStream failed: "
                  << Pa_GetErrorText(err) << "\n";
        stream_ = nullptr;
        return false;
    }

    err = Pa_StartStream(stream_);
    if (err != paNoError) {
        std::cerr << "[AudioCapture] Pa_StartStream failed: "
                  << Pa_GetErrorText(err) << "\n";
        Pa_CloseStream(stream_);
        stream_ = nullptr;
        return false;
    }

    return true;
}

void AudioCapture::close()
{
    if (stream_ == nullptr)
        return;

    Pa_StopStream(stream_);
    Pa_CloseStream(stream_);
    stream_ = nullptr;
}

// ---------------------------------------------------------------------------
// PortAudio real-time callback
// ---------------------------------------------------------------------------

int AudioCapture::paCallback(const void*                     inputBuffer,
                             void*                           /*outputBuffer*/,
                             unsigned long                   framesPerBuffer,
                             const PaStreamCallbackTimeInfo* /*timeInfo*/,
                             PaStreamCallbackFlags           /*statusFlags*/,
                             void*                           userData)
{
    AudioCapture* self = static_cast<AudioCapture*>(userData);
    if (inputBuffer != nullptr) {
        const float* in = static_cast<const float*>(inputBuffer);
        self->ring_->write(in, framesPerBuffer);
    }
    return paContinue;
}
