import sounddevice as sd
import numpy as np
import keyboard
import basic.constants as constants
from basic.transcription import Transcriber, TranscriptionModels, TranscriptionWorker
from basic.utils import get_logger
from basic.buffer import RingAudioBuffer

logger = get_logger()


def main():
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

    with sd.InputStream(
        samplerate=constants.SAMPLE_RATE,
        channels=constants.CHANNELS,
        blocksize=constants.BLOCKSIZE,
        dtype="int16",
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

                while keyboard.is_pressed(constants.ACTIVATION_HOTKEY):
                    data, _ = stream.read(constants.BLOCKSIZE)
                    logger.debug(f"Read {len(data)} samples")

                    audio = data.astype(np.float32) / 32768.0
                    audio = audio.squeeze()

                    ring.add(audio)

                logger.info("Recording stopped.")

            except KeyboardInterrupt:
                logger.info("Stopping (KeyboardInterrupt)")
                ring.stop()
                worker.stop()
                return

            except Exception:
                logger.exception("Unexpected error in recording loop")


if __name__ == "__main__":
    main()
