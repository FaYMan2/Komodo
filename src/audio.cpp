#include "audio.h"

#include <algorithm>
#include <iostream>

namespace komodo {
namespace {

void logPortAudioError(PaError error, const char* context) {
    if (error == paNoError) {
        return;
    }
    std::cerr << "[AudioEngine] " << context << " failed: " << Pa_GetErrorText(error) << '\n';
}

}  // namespace

AudioEngine::AudioEngine(AudioConfig config) : config_(config) {}

AudioEngine::~AudioEngine() {
    stop();
    if (initialized_) {
        Pa_Terminate();
        initialized_ = false;
    }
}

bool AudioEngine::initialize() {
    if (initialized_) {
        return true;
    }

    const PaError error = Pa_Initialize();
    if (error != paNoError) {
        logPortAudioError(error, "Pa_Initialize");
        return false;
    }

    initialized_ = true;
    return true;
}

void AudioEngine::setCallback(SampleCallback callback) {
    callback_ = std::move(callback);
}

bool AudioEngine::start() {
    if (!initialized_ && !initialize()) {
        return false;
    }

    if (stream_ != nullptr) {
        return true;
    }

    PaStreamParameters inputParameters{};
    inputParameters.device = Pa_GetDefaultInputDevice();
    if (inputParameters.device == paNoDevice) {
        std::cerr << "[AudioEngine] No default input device available." << '\n';
        return false;
    }

    inputParameters.channelCount = config_.channels;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;

    const PaError openError = Pa_OpenStream(
        &stream_,
        &inputParameters,
        nullptr,
        config_.sampleRate,
        config_.framesPerBuffer,
        paNoFlag,
        &AudioEngine::paCallback,
        this);

    if (openError != paNoError) {
        logPortAudioError(openError, "Pa_OpenStream");
        stream_ = nullptr;
        return false;
    }

    const PaError startError = Pa_StartStream(stream_);
    if (startError != paNoError) {
        logPortAudioError(startError, "Pa_StartStream");
        Pa_CloseStream(stream_);
        stream_ = nullptr;
        return false;
    }

    std::cout << "[AudioEngine] Started with sample rate " << config_.sampleRate
              << " Hz and buffer size " << config_.framesPerBuffer << '\n';
    return true;
}

void AudioEngine::stop() {
    if (stream_ == nullptr) {
        return;
    }

    const PaError stopError = Pa_StopStream(stream_);
    if (stopError != paNoError) {
        logPortAudioError(stopError, "Pa_StopStream");
    }

    Pa_CloseStream(stream_);
    stream_ = nullptr;
}

int AudioEngine::paCallback(const void* input,
                            void* /*output*/,
                            unsigned long frameCount,
                            const PaStreamCallbackTimeInfo* /*timeInfo*/,
                            PaStreamCallbackFlags /*statusFlags*/,
                            void* userData) {
    auto* engine = static_cast<AudioEngine*>(userData);
    if (engine == nullptr || input == nullptr) {
        return paContinue;
    }

    const auto* floatInput = static_cast<const float*>(input);
    engine->handleSamples(floatInput, frameCount);
    return paContinue;
}

void AudioEngine::handleSamples(const float* input, unsigned long frameCount) {
    if (!callback_) {
        return;
    }

    const std::size_t sampleCount = static_cast<std::size_t>(frameCount) * static_cast<std::size_t>(config_.channels);
    std::vector<float> buffer(sampleCount);
    std::copy(input, input + sampleCount, buffer.begin());
    callback_(buffer);
}

}  // namespace komodo
