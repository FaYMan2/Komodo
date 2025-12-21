#include "audio.h"

#include <iostream>
#include <vector>

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

std::vector<float> AudioEngine::recordWhile(const std::function<bool()>& keepRecording) {
    std::vector<float> recorded;
    if (!keepRecording) {
        return recorded;
    }

    if (!initialize()) {
        return recorded;
    }

    PaStreamParameters inputParameters{};
    inputParameters.device = Pa_GetDefaultInputDevice();
    if (inputParameters.device == paNoDevice) {
        std::cerr << "[AudioEngine] No default input device available." << '\n';
        return recorded;
    }

    inputParameters.channelCount = config_.channels;
    inputParameters.sampleFormat = paFloat32;

    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(inputParameters.device);
    if (deviceInfo == nullptr) {
        std::cerr << "[AudioEngine] Failed to query the default input device." << '\n';
        return recorded;
    }
    inputParameters.suggestedLatency = deviceInfo->defaultLowInputLatency;

    PaStream* stream = nullptr;
    const PaError openError = Pa_OpenStream(
        &stream,
        &inputParameters,
        nullptr,
        config_.sampleRate,
        config_.framesPerBuffer,
        paNoFlag,
        nullptr,
        nullptr);

    if (openError != paNoError) {
        logPortAudioError(openError, "Pa_OpenStream");
        return recorded;
    }

    const PaError startError = Pa_StartStream(stream);
    if (startError != paNoError) {
        logPortAudioError(startError, "Pa_StartStream");
        Pa_CloseStream(stream);
        return recorded;
    }

    const std::size_t samplesPerBlock = static_cast<std::size_t>(config_.framesPerBuffer) * static_cast<std::size_t>(config_.channels);
    std::vector<float> block(samplesPerBlock, 0.0f);

    // Keep pulling data from PortAudio until the caller indicates we're done (hotkey released).
    while (keepRecording()) {
        const PaError readError = Pa_ReadStream(stream, block.data(), config_.framesPerBuffer);
        if (readError != paNoError) {
            logPortAudioError(readError, "Pa_ReadStream");
            break;
        }
        recorded.insert(recorded.end(), block.begin(), block.end());
    }

    const PaError stopError = Pa_StopStream(stream);
    if (stopError != paNoError) {
        logPortAudioError(stopError, "Pa_StopStream");
    }

    Pa_CloseStream(stream);
    return recorded;
}

}  // namespace komodo
