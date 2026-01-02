import keyboard as kb
from base.utils import get_logger

logger = get_logger()


class ExecutorAgent:
    def __init__(self) -> None:
        pass
    
    @staticmethod
    def type_text(text: str) -> None:
        kb.write(text)
        logger.info("Typed text: %s", text)
    
    
    