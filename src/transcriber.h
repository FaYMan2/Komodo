#pragma once

#include <filesystem>
#include <string>
#include <vector>

// whisper.cpp forward declaration (keeps compile times low)
struct whisper_context;

namespace komodo {

enum class TranscriptionModels {
    small_en_q8_0,
};

class Transcriber {
public:
    explicit Transcriber(TranscriptionModels model, std::filesystem::path modelDir = {});
    // Load from an explicit model file path (e.g. ggml-base.en.bin).
    explicit Transcriber(std::filesystem::path modelPath);
    ~Transcriber();

    void transcribe(const std::vector<float>& samples);

    bool isTranscribable(const std::vector<float>& samples) const;

private:
    std::string modelName_;
    std::filesystem::path modelPath_;

    // Loaded once, reused across calls
    whisper_context* ctx_ = nullptr;
};

}  // namespace komodo
