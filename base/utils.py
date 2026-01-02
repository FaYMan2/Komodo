import logging


class KomodoLogger(logging.Logger):
    
    _FMT = '%(asctime)s | %(levelname)s | %(name)s | %(message)s'
    _DATEFMT = '%Y-%m-%d %H:%M:%S'

    def __init__(self, name: str, level: int = logging.DEBUG) -> None:
        super().__init__(name, level)
        self._ensure_stream_handler(level)

    def _ensure_stream_handler(self, handler_level: int) -> None:
        if any(isinstance(h, logging.StreamHandler) for h in self.handlers):
            return
        handler = logging.StreamHandler()
        handler.setLevel(handler_level)
        handler.setFormatter(logging.Formatter(fmt=self._FMT, datefmt=self._DATEFMT))
        self.addHandler(handler)


def get_logger(name: str = 'komodo', level: int = logging.DEBUG) -> logging.Logger:
    logging.setLoggerClass(KomodoLogger)
    logger = logging.getLogger(name)
    logger.setLevel(level)

    if isinstance(logger, KomodoLogger):
        logger._ensure_stream_handler(level)
    else:
        if not any(isinstance(h, logging.StreamHandler) for h in logger.handlers):
            handler = logging.StreamHandler()
            handler.setLevel(level)
            handler.setFormatter(logging.Formatter(fmt=KomodoLogger._FMT, datefmt=KomodoLogger._DATEFMT))
            logger.addHandler(handler)

    logger.propagate = False
    return logger