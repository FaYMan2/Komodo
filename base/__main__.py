import os
import random
import wave

import sounddevice as sd
import numpy as np
import keyboard
import base.constants as constants
from base.transcription import Transcriber, TranscriptionModels, TranscriptionWorker
from base.utils import get_logger
from base.buffer import RingAudioBuffer
from base.executor_agent import ExecutorAgent 

logger = get_logger()


def save_wav(audio: np.ndarray, sample_rate: int, path: str):
    """
    audio: float32 mono array in [-1, 1]
    """
    audio_int16 = (audio * 32767).astype(np.int16)

    with wave.open(path, "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)  # int16
        wf.setframerate(sample_rate)
        wf.writeframes(audio_int16.tobytes())


def select_microphone() -> int:
    """
    Display available input devices and let user select one.
    Returns the device index.
    """
    print("\n" + "="*50)
    print("Available Microphones:")
    print("="*50)
    
    devices = sd.query_devices()
    input_devices = []
    
    for i, device in enumerate(devices):
        if device['max_input_channels'] > 0:
            input_devices.append((i, device))
            marker = " (default)" if i == sd.default.device[0] else ""
            print(f"  [{len(input_devices)}] {device['name']}{marker}")
            print(f"      Channels: {device['max_input_channels']}, Sample Rate: {device['default_samplerate']}")
    
    print("="*50)
    
    if not input_devices:
        raise RuntimeError("No input devices found!")
    
    while True:
        try:
            choice = input(f"\nSelect microphone [1-{len(input_devices)}] (Enter for default): ").strip()
            
            if choice == "":
                # Use default device
                device_index = sd.default.device[0]
                device_name = sd.query_devices(device_index)['name']
                print(f"Using default: {device_name}")
                return device_index
            
            choice_num = int(choice)
            if 1 <= choice_num <= len(input_devices):
                device_index, device = input_devices[choice_num - 1]
                print(f"Selected: {device['name']}")
                return device_index
            else:
                print(f"Please enter a number between 1 and {len(input_devices)}")
        except ValueError:
            print("Invalid input. Please enter a number.")


def initalise_workers():
    transcriber = Transcriber(TranscriptionModels.tiny_en)

    ring = RingAudioBuffer(
        sample_rate=constants.SAMPLE_RATE,   
        chunk_duration=constants.CHUNK_SIZE,                  
        overlap_duration=constants.CHUNK_OVERLAP,                
        buffer_duration=constants.DEFAULT_BUFFER_SIZE                 
    )

    worker = TranscriptionWorker(
        ring_buffer=ring,
        transcriber=transcriber
    )
    
    return ring, worker

def main():
    # Select microphone before starting
    device_index = select_microphone()
    print("\n")
    
    ring, worker = initalise_workers()
    with sd.InputStream(
        device=device_index,
        samplerate=constants.SAMPLE_RATE,
        channels=constants.CHANNELS,
        blocksize=constants.BLOCKSIZE,
        dtype="float32",
    ) as stream:

        while True:
            try:
                logger.info(
                    "Press and hold %s to record audio...",
                    constants.ACTIVATION_HOTKEY
                )

                keyboard.wait(constants.ACTIVATION_HOTKEY)
                logger.info(
                    "Recording... Release %s to stop.",
                    constants.ACTIVATION_HOTKEY
                )
                
                ring.clear()
                worker.session_transcripts.clear()

                recorded_chunks = []   
                did_record = False

                while keyboard.is_pressed(constants.ACTIVATION_HOTKEY):
                    data, _ = stream.read(constants.BLOCKSIZE)
                    logger.debug(f"Read {len(data)} samples")
                    # force mono explicitly
                    if data.ndim == 2:
                        data = data[:, 0]
                    
                 #   audio = data.astype(np.float32) / 32768.0
                    audio = data.copy()
                    ring.add(audio)                  
                    recorded_chunks.append(audio)   
                    did_record = True

                logger.info("Recording stopped.")

                if did_record:
                    os.makedirs(".recordings", exist_ok=True)

                    filename = f"{random.randint(100000, 999999)}.wav"
                    path = os.path.join(".recordings", filename)

                    full_audio = np.concatenate(recorded_chunks)
                    save_wav(full_audio, constants.SAMPLE_RATE, path)

                    logger.info(f"Saved recording to {path}")

                    logger.info("Finalizing transcription...")
                    worker.flush_and_transcribe()
                    session_text = worker.get_session_transcript()
                    ExecutorAgent.type_text(session_text + " ")
                    
            except KeyboardInterrupt:
                logger.info("Stopping (KeyboardInterrupt)")
                ring.stop()
                worker.stop()
                return

            except Exception:
                logger.exception("Unexpected error in recording loop")


if __name__ == "__main__":
    main()
