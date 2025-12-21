#include "audio.h"
#include "transcriber.h"

#include <Windows.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

namespace {
    constexpr SHORT kKeyPressedMask = static_cast<SHORT>(0x8000);

    bool isHotkeyPressed() {
        return (GetAsyncKeyState(VK_CONTROL) & kKeyPressedMask) && (GetAsyncKeyState('Q') & kKeyPressedMask);
    }

    void waitForHotkeyPress() {
        while (!isHotkeyPressed()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void waitForHotkeyRelease() {
        while (isHotkeyPressed()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    std::filesystem::path getExeDir() {
        wchar_t path[MAX_PATH]{};
        const DWORD len = GetModuleFileNameW(nullptr, path, static_cast<DWORD>(std::size(path)));
        if (len == 0 || len >= std::size(path)) {
            return std::filesystem::current_path();
        }
        return std::filesystem::path(path).parent_path();
    }

    std::filesystem::path resolveModelDir(int argc, char** argv) {
        // Default: directory containing komodo.exe (more reliable than CWD).
        std::filesystem::path modelDir = getExeDir();

        if (const char* env = std::getenv("KOMODO_MODEL_DIR"); env && *env) {
            modelDir = std::filesystem::path(env);
        }

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--model-dir" && i + 1 < argc) {
                modelDir = std::filesystem::path(argv[i + 1]);
                ++i;
            } else if (arg == "--help" || arg == "-h") {
                std::cout
                    << "komodo\n\n"
                    << "Usage:\n"
                    << "  komodo.exe [--model <path-to-model.bin>] [--model-dir <path>]\n\n"
                    << "Options:\n"
                    << "  --model <path>       Full path to a ggml model (.bin). (or set KOMODO_MODEL)\n"
                    << "  --model-dir <path>   Directory containing ggml-small.en-q8_0.bin (or set KOMODO_MODEL_DIR)\n";
                std::exit(0);
            }
        }

        return modelDir;
    }

    std::optional<std::filesystem::path> resolveModelPath(int argc, char** argv) {
        if (const char* env = std::getenv("KOMODO_MODEL"); env && *env) {
            return std::filesystem::path(env);
        }

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--model" && i + 1 < argc) {
                return std::filesystem::path(argv[i + 1]);
            }
        }

        return std::nullopt;
    }
}  // namespace

int main(int argc, char** argv) {
    using namespace komodo;

    try {
        std::cout << "[Main] Starting komodo...\n";

        AudioEngine engine({16000.0, 1, 512});
        if (!engine.initialize()) {
            std::cerr << "[Main] Failed to initialize audio engine.\n";
            return 1;
        }

        const auto modelPath = resolveModelPath(argc, argv);
        if (modelPath) {
            std::cout << "[Main] Model file: " << modelPath->string() << "\n";
        } else {
            const auto modelDir = resolveModelDir(argc, argv);
            std::cout << "[Main] Model dir: " << modelDir.string() << "\n";
        }

        Transcriber transcriber =
            modelPath
                ? Transcriber(*modelPath)
                : Transcriber(TranscriptionModels::small_en_q8_0, resolveModelDir(argc, argv));

        std::cout << "komodo ready. Hold Ctrl+Q to speak. Release to transcribe.\n";

        while (true) {
            waitForHotkeyPress();
            auto audio = engine.recordWhile([] { return isHotkeyPressed(); });
            waitForHotkeyRelease();

            if (!transcriber.isTranscribable(audio)) {
                std::cout << "[Main] Captured audio was too short. Try again.\n";
                continue;
            }

            transcriber.transcribe(audio);
            std::cout << "[Main] Utterance processed. Hold Ctrl+Q for the next one.\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "[Main] Fatal error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "[Main] Fatal error: unknown exception\n";
        return 1;
    }
}
