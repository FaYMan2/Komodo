
from re import S
from basic.buffer import RingAudioBuffer
import basic.constants as constants
import numpy as np
from pywhispercpp.model import Model
from basic.utils import get_logger
from enum import Enum
import threading
import time
from basic.buffer import RingAudioBuffer

logger = get_logger()

class TranscriptionModels(Enum):
    tiny_en_q5_1 = 'tiny.en.q5_1'
    base_en  = 'base.en'
    small_en = 'small.en'
    small_en_q8_0 = 'small.en-q8_0'
    small_q8_0 = 'small-q8_0'
    small_q5_1 = 'small-q5_1'


class Transcriber():
    def __init__(self,model: TranscriptionModels = TranscriptionModels.base_en):
        print("loading model: ", model.value, "...",constants.MODELS_DIR)
        self.model = Model(
            model=model.value,
            models_dir=constants.MODELS_DIR
        )
        self.sample_rate = constants.SAMPLE_RATE
        
    def transcribe(self, audio: np.ndarray) -> None:
        try:
            result = self.model.transcribe(
                media=audio,
                n_processors=1,
                new_segment_callback=lambda segment: logger.info("Segment: %s", segment.text),
                extract_probability=False,
            )
            logger.info("Transcription: %s", result)
        except Exception:
            logger.exception("Transcription failed")


    def is_transcribable(self, audio: np.ndarray, threshold: float = 300.0, min_seconds: float = 1.0) -> bool:
        if audio.dtype != np.int16:
            threshold = threshold /  32768

        mean_val = np.abs(audio).mean()
        logger.debug(f"is_transcribable check: mean={mean_val:.6f}, threshold={threshold}, shape={audio.shape}")

        checks = [
            mean_val > threshold,
            audio.shape[0] > int(self.sample_rate * min_seconds) # at least 1 second
        ]

        return all(checks)
    
class TranscriptionWorker:
    """
    Background worker that consumes audio chunks from a RingAudioBuffer
    and feeds them into an existing Transcriber.
    """

    def __init__(self, ring_buffer : RingAudioBuffer, transcriber : Transcriber):
        """
        :param ring_buffer: RingAudioBuffer instance
        :param transcriber: Transcriber instance
        """
        self.ring = ring_buffer
        self.transcriber = transcriber

        self.running = True
        self.thread = threading.Thread(
            target=self._loop,
            daemon=True
        )
        self.thread.start()

    def _loop(self):
        while self.running:
            try:
                logger.debug("Worker waiting for chunk...")
                chunk = self.ring.get_chunk()
                logger.debug(f"Worker got chunk. Shape: {chunk.shape}")
                if not self.transcriber.is_transcribable(chunk):
                    logger.debug("Chunk ignored (silence/short)")
                    continue
                logger.debug("Transcribing chunk...")
                self.transcriber.transcribe(chunk)

            except Exception:
                logger.exception("Transcription worker error")
                time.sleep(0.05)
                
    def stop(self):
        self.running = False
