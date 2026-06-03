from flask import Flask, request
import wave
import os
from datetime import datetime
import whisper  # ★追加：Whisperライブラリ
from flask import Response


app = Flask(__name__)

# ★追加：Whisperモデルのロード（起動時に1回だけ読み込むことで処理を高速化）
print("Whisperモデルを読み込んでいます...（初回はダウンロードが入ります）")
# "base"は軽くて速いモデル。精度を上げたい場合は後で "small" や "turbo" に変更可能です。
whisper_model = whisper.load_model("base") 
print("Whisperの準備完了！")

@app.route('/upload', methods=['POST'])
def upload_audio():
    raw_audio_data = request.data
    
    if not raw_audio_data:
        return "No audio data received", 400

    now = datetime.now()
    timestamp = now.strftime("%Y%m%d_%H%M%S")
    wav_filename = f"recorded_audio_{timestamp}.wav"
    
    with wave.open(wav_filename, 'wb') as wav_file:
        wav_file.setnchannels(1)       
        wav_file.setsampwidth(2)       
        wav_file.setframerate(16000)   
        wav_file.writeframes(raw_audio_data)

    print(f"[{wav_filename}] を保存しました！ データサイズ: {len(raw_audio_data)} bytes")

    # ========= 追加した処理 =========
    print("文字起こしを開始します...")
    # language="ja" で日本語に固定すると、より速く正確に認識します
    result = whisper_model.transcribe(wav_filename, language="ja")
    
    transcribed_text = result["text"].strip()
    print("--------------------------------------------------")
    print(f"【認識結果】: {transcribed_text}")
    print("--------------------------------------------------")
    # ================================================
    return Response(transcribed_text, mimetype='text/plain')

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080, debug=True)