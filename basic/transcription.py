
from re import S
import basic.constants as constants
import numpy as np
from pywhispercpp.model import Model
from basic.utils import get_logger
from enum import Enum

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
            models_dir=constants.MODELS_DIR,
        )
        self.sample_rate = constants.SAMPLE_RATE
        
    def transcribe(self, audio: np.ndarray) -> None:
        try:
            result = self.model.transcribe(
                media=audio,
                n_processors=1
            )
            logger.info("Transcription: %s", result)
        except Exception:
            logger.exception("Transcription failed")


    def is_transcribable(self, audio: np.ndarray, threshold: float = 300.0, min_seconds: float = 1.0) -> bool:
        if audio.dtype != np.int16:
            threshold = threshold /  32768

        checks = [
            np.abs(audio).mean() > threshold,
            audio.shape[0] > int(self.sample_rate * min_seconds) # at least 1 second
        ]

        return all(checks)