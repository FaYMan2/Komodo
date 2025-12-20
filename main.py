from random import sample
from pywhispercpp.model import Model
import os
import sounddevice as sd
import numpy as np
import keyboard
import constants
from utils import get_logger


logger = get_logger()


model = Model(
    model='base.en',
    models_dir=constants.MODELS_DIR,
)


def transcribe(audio: np.ndarray) -> None:
    try:
        result = model.transcribe(
            media=audio,
            n_processors=1
        )
        logger.info("Transcription: %s", result)
    except Exception:
        logger.exception("Transcription failed")


def is_transcribable(audio: np.ndarray, threshold: float = 300.0, min_seconds: float = 1.0) -> bool:
    if audio.dtype != np.int16:
        threshold = threshold /  32768
        
    checks = [
        np.abs(audio).mean() > threshold,
        audio.shape[0] > int(constants.SAMPLE_RATE * min_seconds) # at least 1 second
    ]
    
    return all(checks)


def main():    
    audio_buffer = []
    with sd.InputStream(
        samplerate=constants.SAMPLE_RATE,
        channels=constants.CHANNELS,
        blocksize=constants.BLOCKSIZE,
        dtype='int16'
    ) as stream:
        while True:
            try:
                logger.info("Press and hold %s to record audio...", constants.ACTIVATION_HOTKEY)
                keyboard.wait(constants.ACTIVATION_HOTKEY)
                logger.info("Recording... Release %s to stop.", constants.ACTIVATION_HOTKEY)
                audio_buffer.clear()

                while keyboard.is_pressed(constants.ACTIVATION_HOTKEY):
                    data, _ = stream.read(constants.BLOCKSIZE)
                    audio_buffer.append(data.copy())
                audio = np.concatenate(audio_buffer, axis=0).squeeze()

                if not is_transcribable(audio):
                    logger.warning("Audio too silent/short, please try again.")
                    continue

                audio = audio.astype(np.float32) / 32768.0
                logger.debug("Captured %d samples", int(audio.shape[0]))
                transcribe(audio)
            except KeyboardInterrupt:
                logger.info("Stopping (KeyboardInterrupt)")
                return
            except Exception:
                logger.exception("Unexpected error in recording loop")
            
 
if __name__ == "__main__":
    main()