    #include "transcriber.h"

    #include <chrono>
    #include <filesystem>
    #include <iostream>
    #include <optional>
    #include <stdexcept>
    #include <whisper.h>
    #include <thread>

    namespace komodo {
    namespace {

    std::string toModelFile(TranscriptionModels model) {
        switch (model) {
            case TranscriptionModels::small_en_q8_0:
                return "ggml-small.en-q8_0.bin";
            default:
                throw std::runtime_error("Unknown transcription model");
        }
    }

    } // namespace

    Transcriber::Transcriber(TranscriptionModels model, std::filesystem::path modelDir)
        : modelName_(toModelFile(model)),
        modelPath_((modelDir.empty() ? std::filesystem::current_path() : std::move(modelDir)) / modelName_) {

        if (!std::filesystem::exists(modelPath_)) {
            // Friendly fallback: if the expected model name isn't present but the directory contains
            // a usable .bin (common when user downloaded ggml-base.en.bin), auto-select it.
            const auto dir = modelPath_.parent_path();
            if (std::filesystem::exists(dir) && std::filesystem::is_directory(dir)) {
                std::vector<std::filesystem::path> bins;
                for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                    if (!entry.is_regular_file()) {
                        continue;
                    }
                    const auto p = entry.path();
                    if (p.extension() == ".bin") {
                        bins.push_back(p);
                    }
                }

                auto pick = [&]() -> std::optional<std::filesystem::path> {
                    if (bins.empty()) {
                        return std::nullopt;
                    }
                    // Prefer common whisper.cpp download names if present.
                    const auto preferred = dir / "ggml-base.en.bin";
                    if (std::filesystem::exists(preferred)) {
                        return preferred;
                    }
                    // If there's only one candidate, use it.
                    if (bins.size() == 1) {
                        return bins[0];
                    }
                    return std::nullopt;
                };

                if (auto chosen = pick()) {
                    std::cerr
                        << "[Transcriber] Warning: expected model not found (" << modelPath_.string() << ").\n"
                        << "[Transcriber] Using detected model: " << chosen->string() << "\n";
                    modelPath_ = *chosen;
                    modelName_ = modelPath_.filename().string();
                } else if (bins.size() > 1) {
                    std::string list;
                    for (const auto& p : bins) {
                        list += "  - " + p.filename().string() + "\n";
                    }
                    throw std::runtime_error(
                        "Whisper model file not found: " + modelPath_.string() + "\n"
                        "Found multiple .bin files in: " + dir.string() + "\n" +
                        list +
                        "Fix:\n"
                        "  - Specify the model explicitly:\n"
                        "      komodo.exe --model <path-to-model.bin>\n"
                    );
                }
            }
        }

        if (!std::filesystem::exists(modelPath_)) {
            throw std::runtime_error(
                "Whisper model file not found: " + modelPath_.string() + "\n"
                "Fix:\n"
                "  - Download a ggml model with whisper.cpp, e.g.:\n"
                "      .\\external\\whisper.cpp\\models\\download-ggml-model.cmd small.en\n"
                "  - Copy/rename the downloaded model to:\n"
                "      " + modelPath_.string() + "\n"
                "  - Or run komodo with a custom model directory:\n"
                "      komodo.exe --model-dir <path>\n"
            );
        }

        whisper_context_params cparams = whisper_context_default_params();
        cparams.use_gpu = false;      // CPU first (stable)
        cparams.flash_attn = false;

        ctx_ = whisper_init_from_file_with_params(modelPath_.string().c_str(), cparams);
        if (!ctx_) {
            throw std::runtime_error("Failed to load whisper model: " + modelPath_.string());
        }

        std::cout << "[Transcriber] Loaded model: " << modelPath_.string() << "\n";
    }

    Transcriber::Transcriber(std::filesystem::path modelPath)
        : modelName_(modelPath.filename().string()),
          modelPath_(std::move(modelPath)) {

        if (!std::filesystem::exists(modelPath_)) {
            throw std::runtime_error(
                "Whisper model file not found: " + modelPath_.string() + "\n"
                "Fix:\n"
                "  - Download a ggml model with whisper.cpp, e.g.:\n"
                "      .\\external\\whisper.cpp\\models\\download-ggml-model.cmd base.en\n"
                "  - Or pass a different model file:\n"
                "      komodo.exe --model <path-to-model.bin>\n"
            );
        }

        whisper_context_params cparams = whisper_context_default_params();
        cparams.use_gpu = false;      // CPU first (stable)
        cparams.flash_attn = false;

        ctx_ = whisper_init_from_file_with_params(modelPath_.string().c_str(), cparams);
        if (!ctx_) {
            throw std::runtime_error("Failed to load whisper model: " + modelPath_.string());
        }

        std::cout << "[Transcriber] Loaded model: " << modelPath_.string() << "\n";
    }

    Transcriber::~Transcriber() {
        if (ctx_) {
            whisper_free(ctx_);
            ctx_ = nullptr;
        }
    }

    void Transcriber::transcribe(const std::vector<float>& samples) {
        if (!isTranscribable(samples)) {
            std::cerr << "[Transcriber] Audio too short\n";
            return;
        }

        whisper_full_params params =
            whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

        params.n_threads = std::thread::hardware_concurrency();
        params.language = "en";
        params.translate = false;
        params.print_progress = false;
        params.print_realtime = false;
        params.print_timestamps = true;
        params.single_segment = false;
        params.no_context = true;

        auto start = std::chrono::high_resolution_clock::now();

        int ret = whisper_full(
            ctx_,
            params,
            samples.data(),
            static_cast<int>(samples.size())
        );

        if (ret != 0) {
            std::cerr << "[Transcriber] whisper_full failed\n";
            return;
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;

        int n_segments = whisper_full_n_segments(ctx_);
        std::cout << "[Transcriber] Transcription done in "
                << elapsed.count() << " s\n";

        for (int i = 0; i < n_segments; ++i) {
            const char* text = whisper_full_get_segment_text(ctx_, i);
            int64_t t0 = whisper_full_get_segment_t0(ctx_, i);
            int64_t t1 = whisper_full_get_segment_t1(ctx_, i);

            std::cout << "[" << t0 / 100.0 << " --> "
                    << t1 / 100.0 << "] "
                    << text << "\n";
        }
    }

    bool Transcriber::isTranscribable(const std::vector<float>& samples) const {
        constexpr std::size_t kMinSamples = 16000; // 1 second @ 16kHz
        return samples.size() >= kMinSamples;
    }

    } // namespace komodo
