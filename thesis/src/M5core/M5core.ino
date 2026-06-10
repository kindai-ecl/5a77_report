#include <M5Unified.h>
#include <WiFi.h>
#include <WiFiClient.h>

// --- 設定エリア ---
const char* ssid = "ecl";
const char* password = "5avscfip2p3kt";
const char* serverIp = "192.168.2.1"; // MacのIPアドレス

// --- 録音設定 ---
static constexpr uint32_t SAMPLE_RATE = 16000;
static constexpr size_t MAX_RECORD_SECONDS = 10; 
int16_t* rec_data;
size_t rec_sample_index = 0;
bool isRecording = false;

// 関数プロトタイプ宣言
void sendAudioToServer();
void showStandbyScreen();
void saveLogLocal(String text);

void setup() {
    auto cfg = M5.config();
    cfg.internal_mic = true; // 内蔵マイクを有効化
    M5.begin(cfg);
    Serial.begin(115200);

    // ★日本語フォントを設定（16ピクセルサイズ）
    M5.Display.setFont(&fonts::lgfxJapanGothic_16); 
    M5.Display.setTextSize(1); // 日本語フォントは1倍で十分読みやすいサイズです
    
    // PSRAMに録音領域を確保
    rec_data = (int16_t*)heap_caps_malloc(SAMPLE_RATE * MAX_RECORD_SECONDS * sizeof(int16_t), MALLOC_CAP_SPIRAM);

    // 内蔵マイクの設定
    auto mic_cfg = M5.Mic.config();
    mic_cfg.sample_rate = SAMPLE_RATE;
    mic_cfg.stereo = false;
    M5.Mic.config(mic_cfg);
    M5.Mic.begin();

    // Wi-Fi接続画面の日本語化
    M5.Display.print("Wi-Fiに接続中...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        M5.Display.print(".");
    }
    
    showStandbyScreen();
}

void loop() {
    M5.update();
    
    // 画面タッチで録音開始/停止
    if (M5.Touch.getCount() > 0) {
        auto t = M5.Touch.getDetail();
        if (t.wasPressed()) {
            if (!isRecording) {
                // 録音開始
                isRecording = true;
                rec_sample_index = 0;
                M5.Display.clear();
                M5.Display.setCursor(0, 0);
                M5.Display.setTextColor(TFT_RED);
                M5.Display.println("● 録音中...");
                M5.Display.setTextColor(TFT_WHITE);
                M5.Display.println("画面をタップすると停止します");
            } else {
                // 録音停止
                isRecording = false;
                sendAudioToServer();
            }
        }
    }

    // 録音実行中
    if (isRecording && rec_sample_index + 1024 <= (SAMPLE_RATE * MAX_RECORD_SECONDS)) {
        if (M5.Mic.record(&rec_data[rec_sample_index], 1024, SAMPLE_RATE)) {
            rec_sample_index += 1024;
        }
    }
}

void sendAudioToServer() {
    Serial.println("\n[DEBUG] === sendAudioToServer 開始 ===");
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setCursor(0, 0);
    M5.Display.setTextColor(TFT_YELLOW);
    M5.Display.println("音声データを解析中...");

    WiFiClient client;
    Serial.print("[DEBUG] サーバーに接続中... IP: ");
    Serial.print(serverIp);
    Serial.println(" ポート: 8080");

    if (client.connect(serverIp, 8080)) {
        Serial.println("[DEBUG] サーバー接続成功！データ送信を開始します...");
        
        // 1. 音声データの送信
        size_t send_bytes = rec_sample_index * sizeof(int16_t);
        Serial.printf("[DEBUG] 送信データサイズ: %d bytes\n", send_bytes);

        client.printf("POST /upload HTTP/1.1\r\n");
        client.printf("Host: %s\r\n", serverIp);
        client.println("Content-Type: application/octet-stream");
        client.printf("Content-Length: %d\r\n", send_bytes);
        client.println();

        uint8_t* p = (uint8_t*)rec_data;
        size_t remaining = send_bytes;
        while (remaining > 0) {
            size_t chunk = (remaining > 4096) ? 4096 : remaining;
            client.write(p, chunk);
            p += chunk;
            remaining -= chunk;
            delay(1);
        }
        Serial.println("[DEBUG] データ送信完了。サーバーからの応答を待っています...");

        // 2. 受信処理（★日本語が壊れないバッファ読み込み方式に修正）
        unsigned long timeout = millis();
        String allResponse = "";
        char rxBuffer[512]; // まとめて受け取るためのバッファ
        
        Serial.println("[DEBUG] 受信ループに入ります（タイムアウト15秒）...");
        while (client.connected() && millis() - timeout < 15000) {
            int availableBytes = client.available();
            if (availableBytes > 0) {
                // バッファサイズ（511バイト）を超えないように制限して一括読み込み
                int bytesToRead = (availableBytes > 511) ? 511 : availableBytes;
                int readBytes = client.readBytes(rxBuffer, bytesToRead);
                rxBuffer[readBytes] = '\0'; // 終端文字を付与
                
                allResponse += rxBuffer; // まとめて文字列に追加（文字化けを防ぐ）
                timeout = millis();      // タイムアウトをリセット
            }
            delay(10); // 短いウェイトを入れてCPU負荷を下げる
        }
        client.stop();
        Serial.println("[DEBUG] 受信ループを抜けました。接続を閉じました。");
        Serial.printf("[DEBUG] 受信した生データ全体の長さ: %d 文字\n", allResponse.length());

        // 生データをそのままシリアルに出力して確認
        Serial.println("----- サーバーからの生の応答（ここから） -----");
        Serial.println(allResponse);
        Serial.println("----- サーバーからの生の応答（ここまで） -----");

        // HTTPヘッダーと本文（Body）を分離
        String resultText = "";
        int bodyIndex = allResponse.indexOf("\r\n\r\n");
        if (bodyIndex != -1) {
            resultText = allResponse.substring(bodyIndex + 4);
        } else {
            bodyIndex = allResponse.indexOf("\n\n");
            if (bodyIndex != -1) {
                resultText = allResponse.substring(bodyIndex + 2);
            } else {
                resultText = allResponse;
            }
        }
        resultText.trim(); 

        Serial.print("[DEBUG] 解析されたテキスト(Body): ");
        Serial.println(resultText);

        // 3. エラーハンドリング
        if (resultText == "" || resultText == "(No text detected)" || resultText.startsWith("HTTP/1.1 400") || resultText.startsWith("HTTP/1.1 500")) {
            Serial.println("[DEBUG] 認識エラー、またはサーバーエラーを検出しました。");
            M5.Display.fillScreen(TFT_BLACK);
            M5.Display.setCursor(0, 0);
            M5.Display.setTextColor(TFT_RED);
            M5.Display.println("【エラー】");
            M5.Display.println("音声が聞き取れませんでした。");
            delay(4000); 
            showStandbyScreen();
            return; 
        }

        // 4. 正しく聞き取れた場合の日本語確認画面
        Serial.println("[DEBUG] 正常にテキストを認識。画面に表示します。");
        M5.Display.fillScreen(TFT_BLACK);
        M5.Display.setCursor(0, 0);
        M5.Display.setTextColor(TFT_CYAN);
        M5.Display.println("【認識結果】");
        M5.Display.setTextColor(TFT_WHITE);
        M5.Display.println(resultText); // これで「こんにちは」が表示されるはずです
        
        M5.Display.println("--------------------");
        M5.Display.setTextColor(TFT_YELLOW);
        M5.Display.println("この内容で記録しますか？");
        M5.Display.println("左:やり直す | 右:保存する");

        // 5. ユーザー判定ループ
        while (true) {
            M5.update();
            if (M5.Touch.getCount() > 0) {
                auto t = M5.Touch.getDetail();
                if (t.wasPressed()) {
                    if (t.x > 160) {
                        saveLogLocal(resultText);
                        M5.Display.fillScreen(TFT_BLACK);
                        M5.Display.setTextColor(TFT_GREEN);
                        M5.Display.println("保存しました！");
                        delay(1200);
                        break;
                    } else {
                        M5.Display.fillScreen(TFT_BLACK);
                        M5.Display.setTextColor(TFT_RED);
                        M5.Display.println("破棄しました。");
                        delay(1000);
                        break;
                    }
                }
            }
            delay(10);
        }
    } else {
        Serial.println("[ERROR] サーバーへの接続に失敗しました！");
        M5.Display.fillScreen(TFT_BLACK);
        M5.Display.setTextColor(TFT_RED);
        M5.Display.println("サーバー接続失敗");
        delay(2000);
    }
    Serial.println("[DEBUG] === sendAudioToServer 終了 ===\n");
    showStandbyScreen();
}

void saveLogLocal(String text) {
    Serial.println("\n--- 音声ログ記録 ---");
    Serial.print("認識テキスト: ");
    Serial.println(text);
    Serial.print("起動後時間(ms): ");
    Serial.println(millis());
    Serial.println("--------------------\n");
}

void showStandbyScreen() {
    M5.Display.clear();
    M5.Display.setCursor(0, 0);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.println("【音声入力システム】");
    M5.Display.println("画面をタップして録音開始");
}