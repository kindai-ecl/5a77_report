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
# BLEタグ検知データを受け取る専用の窓口 (ルートパス '/')
# ================================================================
@app.route('/', methods=['POST'])
def handle_ble_data():
    ble_data = request.get_json(silent=True)
    if ble_data:
        print("\n==================================================")
        print(f"【BLEタグ検知】 機器: {ble_data.get('machine_type')} | タグ: {ble_data.get('name')} | 電波強度: {ble_data.get('rssi')} dBm")
        print("==================================================")
        return "BLE Data Received", 200
    else:
        raw_data = request.data.decode('utf-8', errors='ignore')
        print(f"\n[BLE宛先に不明なデータを受信]: {raw_data}")
        return "Raw Data Received", 200


# ================================================================
# 音声データを受け取る専用の窓口 ('/upload') [精度大幅改善版]
# ================================================================
@app.route('/upload', methods=['POST'])
def upload_audio():
    raw_audio_data = request.data
    
    if not raw_audio_data:
        return "No audio data received", 400

    now = datetime.now()
    timestamp = now.strftime("%Y%m%d_%H%M%S")
    wav_filename = f"recorded_audio_{timestamp}.wav"
    
    # --- M5Stack側から送られたデータ構造を判定 ---
    # 先頭4バイトが 'RIFF'（WAVヘッダー付き）であれば直接ファイル書き込み
    if raw_audio_data.startswith(b'RIFF'):
        with open(wav_filename, 'wb') as f:
            f.write(raw_audio_data)
    else:
        # RAW PCMデータの場合は wave モジュールでWAV化
        with wave.open(wav_filename, 'wb') as wav_file:
            wav_file.setnchannels(1)       
            wav_file.setsampwidth(2)       
            wav_file.setframerate(16000)   
            wav_file.writeframes(raw_audio_data)

    print(f"\n[{wav_filename}] を保存しました！ データサイズ: {len(raw_audio_data)} bytes")

    print("文字起こしを開始します...")
    
    # ★重要：initial_prompt を追加して定型フレーズに限定誘導する
    prompt_text = "定型フレーズ：加工開始、加工終了、段取開始、段取終了。"
    
    result = whisper_model.transcribe(
        wav_filename, 
        language="ja",
        initial_prompt=prompt_text  # これにより特定の4フレーズへの認識率が急上昇します
    )
    
    transcribed_text = result["text"].strip()
    print("--------------------------------------------------")
    print(f"【認識結果】: {transcribed_text}")
    print("--------------------------------------------------")
    
    return Response(transcribed_text, mimetype='text/plain')


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080, debug=True)