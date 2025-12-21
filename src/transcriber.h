#pragma once

#include <string>
#include <vector>

namespace komodo {

enum class TranscriptionModels {
    small_en_q8_0,
};

class Transcriber {
public:
    explicit Transcriber(TranscriptionModels model);

    void transcribe(const std::vector<float>& samples) const;
    bool isTranscribable(const std::vector<float>& samples) const;

private:
    std::string modelName_;
};

}  // namespace komodo
