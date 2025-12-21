#include "audio.h"
#include "transcriber.h"

#include <iostream>

int main() {
    using namespace komodo;

    AudioConfig config;
    config.sampleRate = 16000.0;
    config.channels = 1;
    config.framesPerBuffer = 512;

    AudioEngine engine(config);
    Transcriber transcriber(TranscriptionModels::small_en_q8_0);

    if (!engine.initialize()) {
        std::cerr << "Failed to initialize audio engine." << std::endl;
        return 1;
    }

    engine.setCallback([&](const std::vector<float>& samples) {
        if (transcriber.isTranscribable(samples)) {
            transcriber.transcribe(samples);
        }
    });

    if (!engine.start()) {
        std::cerr << "Failed to start audio stream." << std::endl;
        return 1;
    }

    std::cout << "komodo is capturing audio. Press Enter to stop..." << std::endl;
    std::cin.get();

    engine.stop();
    return 0;
}
