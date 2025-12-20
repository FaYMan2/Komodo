from random import sample
from pywhispercpp.model import Model
import os
import sounddevice as sd
import numpy as np
import keyboard
import constants
from transcription import Transcriber
from utils import get_logger


logger = get_logger()


transcriber = Transcriber()

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

                if not transcriber.is_transcribable(audio):
                    logger.warning("Audio too silent/short, please try again.")
                    continue

                audio = audio.astype(np.float32) / 32768.0
                logger.debug("Captured %d samples", int(audio.shape[0]))
                transcriber.transcribe(audio)
            except KeyboardInterrupt:
                logger.info("Stopping (KeyboardInterrupt)")
                return
            except Exception:
                logger.exception("Unexpected error in recording loop")
            
 
if __name__ == "__main__":
    main()