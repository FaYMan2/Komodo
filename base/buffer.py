from operator import le
from turtle import clear
import numpy as np
import threading
import queue
import time
import base.constants as constants
from base.utils import get_logger
from typing import List
logger = get_logger()

class RingAudioBuffer:
    """
    Realtime-safe ring buffer for streaming audio with overlapping chunk extraction.
    """

    def __init__(
        self,
        sample_rate: int = constants.SAMPLE_RATE,
        chunk_duration: float = constants.CHUNK_SIZE,
        overlap_duration: float = constants.CHUNK_OVERLAP,
        buffer_duration: float = constants.DEFAULT_BUFFER_SIZE,
        dtype=np.float32,
    ):
        """
        :param sample_rate: audio sample rate (e.g. 16000)
        :param chunk_duration: seconds per transcription chunk (e.g. 2.0)
        :param overlap_duration: seconds of overlap (e.g. 0.3)
        :param buffer_duration: total ring buffer size in seconds
        """

        assert chunk_duration > overlap_duration

        self.sample_rate = sample_rate
        self.chunk_size = int(chunk_duration * sample_rate)
        self.overlap = int(overlap_duration * sample_rate)
        self.step = self.chunk_size - self.overlap

        self.capacity = int(buffer_duration * sample_rate)

        self.buffer = np.zeros(self.capacity, dtype=dtype)

        self.write_idx = 0
        self.read_idx = 0
        self.available = 0

        self.lock = threading.Lock()
        self.queue = queue.Queue()
        self.running = True

        self.worker = threading.Thread(
            target=self._chunking_worker, daemon=True
        )
        self.worker.start()


    def add(self, audio: np.ndarray) -> None:
        n = len(audio)
        if n == 0:
            return

        with self.lock:
            end = self.write_idx + n
            logger.debug(f"RingBuffer add: {n} samples. Write idx: {self.write_idx}. Available: {self.available}")

            if end <= self.capacity:
                self.buffer[self.write_idx:end] = audio
            else:
                first = self.capacity - self.write_idx
                self.buffer[self.write_idx:] = audio[:first]
                self.buffer[:end % self.capacity] = audio[first:]

            self.write_idx = end % self.capacity
            self.available = min(self.available + n, self.capacity)

    def _chunking_worker(self):
        while self.running:
            try:
                with self.lock:
                    # logger.debug(f"Chunking worker: Available {self.available} / {self.chunk_size}")
                    if self.available >= self.chunk_size:
                        chunk = np.empty(self.chunk_size, dtype=self.buffer.dtype)

                        start = self.read_idx
                        end = start + self.chunk_size

                        if end <= self.capacity:
                            chunk[:] = self.buffer[start:end]
                        else:
                            first = self.capacity - start
                            chunk[:first] = self.buffer[start:]
                            chunk[first:] = self.buffer[:end % self.capacity]

                        self.read_idx = (self.read_idx + self.step) % self.capacity
                        logger.debug(f"Chunk created. Size: {len(chunk)}")
                        self.available -= self.step
                    else:
                        chunk = None

                if chunk is not None:
                    self.queue.put(chunk)
                else:
                    time.sleep(0.005)
            except Exception:
                logger.exception("Error in chunking worker")
                time.sleep(0.1)

    def get_chunk(self) -> np.ndarray:
        """
        Blocks until a chunk is available.
        """
        return self.queue.get()

    def clear(self):
        """
        Reset buffer state.
        """
        with self.lock:
            self.write_idx = 0
            self.read_idx = 0
            self.available = 0
            while True:
                try:
                    self.queue.get_nowait()
                except queue.Empty:
                    break
    
    def flush(self) -> List[np.ndarray]: 
        """
        Flush remaining audio into a final chunk
        
        :param self
        """
        left_chunks = []
        while not self.queue.empty():
            try:
                left_chunks.append(self.queue.get())
            except queue.Empty:
                break

        with self.lock:
            if self.available > 0:
                size = min(self.available, self.chunk_size)
                chunk = np.empty(size, dtype=self.buffer.dtype)
                start = self.read_idx 
                end = start + size
                if end <= self.capacity:
                    chunk[:] = self.buffer[start:end]
                else:
                    first = self.capacity - start
                    chunk[:first] = self.buffer[start:]
                    chunk[first:] = self.buffer[:end % self.capacity]
                self.read_idx = (self.read_idx + size) % self.capacity
                self.available -= size
                left_chunks.append(chunk)
        self.clear()                
        return left_chunks
    def stop(self):
        """
        Stop background worker.
        """
        self.running = False
