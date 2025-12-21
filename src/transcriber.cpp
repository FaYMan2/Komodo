#include "transcriber.h"

#include <chrono>
#include <iostream>

namespace komodo {
namespace {

std::string toModelName(TranscriptionModels model) {
    switch (model) {
        case TranscriptionModels::small_en_q8_0:
            return "small.en.q8_0";
        default:
            return "unknown";
    }
}

}  // namespace

Transcriber::Transcriber(TranscriptionModels model)
    : modelName_(toModelName(model)) {}

void Transcriber::transcribe(const std::vector<float>& samples) const {
    const auto durationMs = static_cast<double>(samples.size()) / 16000.0 * 1000.0;
    std::cout << "[Transcriber] Pretending to transcribe " << durationMs << " ms of audio using model "
              << modelName_ << '\n';
}

bool Transcriber::isTranscribable(const std::vector<float>& samples) const {
    constexpr std::size_t kMinSamples = 16000;  // 1 second at 16 kHz
    return samples.size() >= kMinSamples;
}

}  // namespace komodo
