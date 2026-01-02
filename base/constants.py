import os 

MODELS_DIR = os.path.join('.models')
SAMPLE_RATE = 16000
CHANNELS = 1
BLOCKSIZE = 1024 # 64 MS 
ACTIVATION_HOTKEY = 'ctrl+q'
CHUNK_SIZE =  2 # 2 seconds
CHUNK_OVERLAP = 0.25 # 0.25 seconds
DEFAULT_BUFFER_SIZE = 20 # 20 seconds   