"""
Bloop: Local AI Robot Companion
This script orchestrates the full pipeline for a voice-activated assistant.
It uses openWakeWord for local trigger detection, Groq's Whisper and LLaMA 3.1 
APIs for transcription and reasoning, and Piper TTS for local voice synthesis.
"""

import openwakeword
import os
import subprocess
import urllib.request
import time
from dotenv import load_dotenv
import numpy as np
import sounddevice as sd
import soundfile as sf
from groq import Groq
from openwakeword.model import Model

# ==========================================
# CONFIGURATION & CONSTANTS
# ==========================================
MODEL_NAME = "en_GB-alba-medium.onnx"
CONFIG_NAME = "en_GB-alba-medium.onnx.json"
MODEL_URL = f"https://huggingface.co/rhasspy/piper-voices/resolve/v1.0.0/en/en_GB/alba/medium/{MODEL_NAME}"
CONFIG_URL = f"https://huggingface.co/rhasspy/piper-voices/resolve/v1.0.0/en/en_GB/alba/medium/{CONFIG_NAME}"

# Audio Settings
SAMPLE_RATE = 16000
RECORDING_DURATION = 4  
TEMP_COMMAND_WAV = "command_temp.wav"
TEMP_OUTPUT_WAV = "output_temp.wav"

# Awake State Settings
AWAKE_TIME = 180  # Cooldown timer in seconds (3 minutes)
SILENCE_THRESHOLD = 2700  # Peak amplitude required to trigger the API (0-32767)

# ==========================================
# HELPER FUNCTIONS
# ==========================================
def ensure_voice_model() -> None:
    """
    Verifies the existence of the Piper TTS voice model locally.
    Downloads the required files from HuggingFace if they are missing.
    """
    if not os.path.exists(MODEL_NAME):
        print(f"Downloading Piper voice model ({MODEL_NAME})...")
        urllib.request.urlretrieve(MODEL_URL, MODEL_NAME)
        urllib.request.urlretrieve(CONFIG_URL, CONFIG_NAME)


def record_audio(filepath: str, duration: int, samplerate: int) -> int:
    """
    Records audio from the microphone and calculates peak amplitude.

    Args:
        filepath (str): Destination path for the recorded WAV file.
        duration (int): How long to record in seconds.
        samplerate (int): The sample rate in Hz.

    Returns:
        int: The maximum absolute amplitude of the recorded audio chunk.
    """
    audio_data = sd.rec(
        int(duration * samplerate), samplerate=samplerate, channels=1, dtype="int16"
    )
    sd.wait()
    sf.write(filepath, audio_data, samplerate)
    
    # Calculate and return the peak volume of the recording for calibration
    safe_audio = audio_data[int(samplerate * 0.2):]
    peak_volume = np.max(np.abs(safe_audio))
    print(f" [Debug] Peak Room Volume: {peak_volume}")
    return peak_volume


def play_audio(file_path: str) -> None:
    """
    Reads and plays a WAV file through the default system audio output.
    """
    data, fs = sf.read(file_path, dtype="float32")
    sd.play(data, fs)
    sd.wait()

# ==========================================
# MAIN EXECUTION PIPELINE
# ==========================================
def main() -> None:
    # Initialization
    load_dotenv()
    client = Groq()
    ensure_voice_model()

    print("Loading openWakeWord engine...")
    openwakeword.utils.download_models()
    oww_model = Model(wakeword_models=["hey_bloop.onnx"])

    print("\n=======================================")
    print("🤖 Bloop is Online and Listening!")
    print("Say 'Hey Bloop' to wake her up.")
    print("=======================================")

    mic_stream = sd.InputStream(samplerate=SAMPLE_RATE, channels=1, dtype='int16')
    mic_stream.start()

    while True:
        try:
            # Wake word listening loop
            audio_chunk, overflow = mic_stream.read(1280)
            audio_array = audio_chunk.flatten()
            
            oww_model.predict(audio_array)
            
            triggered = False
            for mdl in oww_model.prediction_buffer.keys():
                if oww_model.prediction_buffer[mdl][-1] > 0.0015:
                    triggered = True
                    break
            
            if triggered:
                print("\n🔔 [WAKE WORD DETECTED] Bloop is awake.")
                mic_stream.stop()
                
                # Enter active conversation mode
                last_active = time.time()
                
                while time.time() - last_active < AWAKE_TIME:
                    print("\r🎤 Listening... (Awake for 3 minutes)", end="", flush=True)
                    
                    # Capture command and check for silence
                    peak_volume = record_audio(TEMP_COMMAND_WAV, RECORDING_DURATION, SAMPLE_RATE)
                    
                    if peak_volume < SILENCE_THRESHOLD:
                        continue  
                        
                    print("\n🛑 Voice detected. Thinking...")
                    
                    # Transcribe audio via Groq Whisper API
                    with open(TEMP_COMMAND_WAV, "rb") as audio_file:
                        transcription = client.audio.transcriptions.create(
                            file=(TEMP_COMMAND_WAV, audio_file.read()),
                            model="whisper-large-v3-turbo",
                        )
                    user_text = transcription.text.strip()
                    
                    # Filter non-alphanumeric noise and known Whisper hallucinations
                    has_words = any(char.isalnum() for char in user_text)
                    ignore_phrases = ["thank you.", "thank you", "thanks for watching.", "subscribe."]
                    
                    if not has_words or user_text.lower() in ignore_phrases:
                        continue

                    print(f"You said: {user_text}")

                    # Generate AI response via Groq LLaMA 3.1
                    system_prompt = (
                        "You are Bloop, an energetic, cute, and slightly "
                        "sarcastic desktop robot. Keep answers extremely brief."
                    )

                    response = client.chat.completions.create(
                        model="llama-3.1-8b-instant",
                        messages=[
                            {"role": "system", "content": system_prompt},
                            {"role": "user", "content": user_text},
                        ],
                        max_tokens=80,
                        temperature=0.7,
                    )

                    ai_text = response.choices[0].message.content
                    print(f"Bloop: {ai_text}")

                    # Synthesize and play voice via local Piper TTS
                    piper_cmd = (
                        f'echo "{ai_text}" | piper --model {MODEL_NAME} '
                        f"--length_scale 0.8 --output_file {TEMP_OUTPUT_WAV}"
                    )
                    subprocess.run(
                        piper_cmd, shell=True, 
                        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
                    )

                    if os.path.exists(TEMP_OUTPUT_WAV):
                        play_audio(TEMP_OUTPUT_WAV)
                    else:
                        print("Error: Piper failed to generate audio.")

                    # Reset the cooldown timer after a successful conversation turn
                    last_active = time.time()

                # Cleanup temporary files and enter sleep mode
                if os.path.exists(TEMP_COMMAND_WAV): os.remove(TEMP_COMMAND_WAV)
                if os.path.exists(TEMP_OUTPUT_WAV): os.remove(TEMP_OUTPUT_WAV)
                
                oww_model.reset()
                print("\n💤 Bloop went back to sleep. Listening for wake word...")
                mic_stream.start()
                
        except KeyboardInterrupt:
            print("\nShutting down Bloop. Goodbye!")
            break
        except Exception as e:
            print(f"\nError encountered: {e}")
            break

if __name__ == "__main__":
    main()