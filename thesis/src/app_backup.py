from flask import Flask, request, Response
import wave
import os
from datetime import datetime
import whisper

app = Flask(__name__)

# Whisperモデルのロード
print("Whisperモデルを読み込んでいます...")
whisper_model = whisper.load_model("base") 
print("Whisperの準備完了！")


# ================================================================
# ★追加：BLEタグ検知データを受け取る専用の窓口 (ルートパス '/')
# ================================================================
@app.route('/', methods=['POST'])
def handle_ble_data():
    # M5Stackから送られてきたJSONデータを取得
    ble_data = request.get_json(silent=True)
    
    if ble_data:
        print("\n==================================================")
        print(f"【BLEタグ検知】 機器: {ble_data.get('machine_type')} | タグ: {ble_data.get('name')} | 電波強度: {ble_data.get('rssi')} dBm")
        print("==================================================")
        return "BLE Data Received", 200
    else:
        # JSONではなく生のテキストなどが届いた場合のフォールバック
        raw_data = request.data.decode('utf-8', errors='ignore')
        print(f"\n[BLE宛先に不明なデータを受信]: {raw_data}")
        return "Raw Data Received", 200


# ================================================================
# 既存：音声データを受け取る専用の窓口 ('/upload')
# ================================================================
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

    print(f"\n[{wav_filename}] を保存しました！ データサイズ: {len(raw_audio_data)} bytes")

    print("文字起こしを開始します...")
    result = whisper_model.transcribe(wav_filename, language="ja")
    
    transcribed_text = result["text"].strip()
    print("--------------------------------------------------")
    print(f"【認識結果】: {transcribed_text}")
    print("--------------------------------------------------")
    
    return Response(transcribed_text, mimetype='text/plain')


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080, debug=True)