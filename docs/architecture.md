# Komodo Desktop architecture

This document explains the new push-to-talk flow so you can evolve the native implementation without cross-referencing the Python prototype.

## Runtime flow

1. The process boots PortAudio once (`AudioEngine::initialize`).
2. The main loop waits for the recording hotkey (`Ctrl + Q`).
3. While the user keeps the hotkey pressed, `AudioEngine::recordWhile` performs blocking reads and accumulates raw samples.
4. When the user releases the hotkey, the accumulated samples are returned as **one utterance**.
5. `Transcriber::transcribe` receives that single utterance and can run Whisper, a cloud API, etc.
6. Control returns to step 2 and the assistant sits idle again.

There is no background audio stream and no partial, in-flight callbacks anymore. Everything is explicit and user-driven.

## Audio engine responsibilities

`AudioEngine` now exposes a single high-level method:

```cpp
std::vector<float> AudioEngine::recordWhile(const std::function<bool()>& keepRecording);
```

- `recordWhile` opens its own PortAudio stream, performs blocking `Pa_ReadStream` calls, and appends each block to an output buffer.
- The `keepRecording` callback (a cheap lambda) tells the engine whether the hotkey is still held down.
- As soon as `keepRecording` returns `false`, the engine stops the stream, closes it, and returns the captured buffer.
- If PortAudio fails at any stage, the method logs the error and returns whatever audio was gathered up to that point (possibly empty).

Because the engine no longer owns long-lived streams, there is no `start/stop` bookkeeping to worry about and no thread-hopping callbacks to reason over.

### Tips for extending `AudioEngine`

- **Silence trimming:** Add a post-processing step to `recordWhile` before returning the buffer.
- **Input selection:** Replace `Pa_GetDefaultInputDevice` with UI-configured device indices.
- **Multi-platform hotkeys:** The engine is agnostic; only the `keepRecording` predicate cares about the platform.

## Hotkey loop (Windows)

`main.cpp` owns the hotkey UX. Helper functions in the translation unit wrap `GetAsyncKeyState` so we can:

- Wait idly until `Ctrl + Q` is pressed (`waitForHotkeyPress`).
- Keep sampling audio while the key is held.
- Wait for a clean release before offering another prompt.

If you want to change the key or add the ability to exit the loop, update the helpers; the rest of the application is unaffected.

### Porting guidance

- **macOS:** Replace `GetAsyncKeyState` with `CGEventSourceKeyState` or a small Carbon hotkey listener.
- **Linux:** Use X11, evdev, or a cross-platform hotkey library; just ensure you still provide a `keepRecording` lambda.

## Transcriber placeholder

`Transcriber` still prints a pretend message. Swap the body of `transcribe` with a real Whisper call or IPC to your Python worker. The key invariant is that the method now receives one contiguous utterance, so you can reuse the exact pipeline from the original prototype.

## Growing the app

1. **Add VAD or silence detection** before handing audio to Whisper if you want to automatically discard empty recordings.
2. **Stream status UI** by emitting events ("recording", "transcribing", "ready") from the main loop.
3. **Configurable hotkeys** by reading from a config file and rebuilding the `isHotkeyPressed` predicate at runtime.

Because the architecture matches the Python version, every Whisper-side improvement you previously built ports directly onto the C++ layer now.
