#pragma once

#include <portaudio.h>

#include <functional>
#include <vector>

namespace komodo {

struct AudioConfig {
    double sampleRate = 16000.0;
    int channels = 1;
    unsigned long framesPerBuffer = 512;
};

class AudioEngine {
public:
    using SampleCallback = std::function<void(const std::vector<float>&)>;

    explicit AudioEngine(AudioConfig config = {});
    ~AudioEngine();

    bool initialize();
    bool start();
    void stop();

    void setCallback(SampleCallback callback);

private:
    static int paCallback(const void* input, void* output,
                          unsigned long frameCount,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void* userData);

    void handleSamples(const float* input, unsigned long frameCount);

    AudioConfig config_{};
    SampleCallback callback_{};
    PaStream* stream_ = nullptr;
    bool initialized_ = false;
};

}  // namespace komodo
