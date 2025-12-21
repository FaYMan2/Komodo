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
    explicit AudioEngine(AudioConfig config = {});
    ~AudioEngine();

    bool initialize();
    std::vector<float> recordWhile(const std::function<bool()>& keepRecording);

private:
    AudioConfig config_{};
    bool initialized_ = false;
};

}  // namespace komodo
