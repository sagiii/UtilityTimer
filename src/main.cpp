#include <M5StickCPlus.h>
// 背景画像を使う際は、 https://lang-ship.com/tools/image2data/ でヘッダファイルにして以下2行をコメントイン
#include "image.h"
#define USE_BACKGROUND_IMAGE

// タイマーの状態
enum TimerState {
    IDLE,       // 待機中
    COUNTING,   // カウントダウン中
    PAUSED,     // 一時停止中
    FINISHED    // 終了
};

TimerState currentState = IDLE;

// タイマーの時間 (秒単位)
long totalSeconds = 0;
long remainingSeconds = 0;
unsigned long lastUpdateTime = 0; // 最後に時間を更新した時刻

// 画面表示用のフォントID
#define TIME_FONT_ID 7    // 時間表示用 (7セグ風)
#define TEXT_FONT_ID 4    // テキストメッセージ用 (一般的なフォント)

// メロディの定義 (例: マリオのテーマの一部)
// 音階と音の長さ (ミリ秒) のペア
const int melody[] = {
    262, 100, // C4
    330, 100, // E4
    392, 100, // G4
    523, 200, // C5
    392, 100, // G4
    330, 100, // E4
    262, 200, // C4
    0,   100, // 無音
    // 適宜追加して、より楽しいメロディに
};
const int melodyLength = sizeof(melody) / sizeof(melody[0]) / 2;
int melodyIndex = 0; // メロディ再生中の現在のインデックス

// 関数プロトタイプ
void drawTime(long seconds);
void playMelodyTick(); // メロディを少しずつ再生する関数
void stopMelody();
void clearTimer();
void updateDisplay();
void drawBackground(); // 背景描画用の関数

void setup() {
    M5.begin();
    M5.Lcd.setRotation(1); // 画面を横向きに設定 (TypeCコネクタが右の場合)
    M5.Lcd.setTextDatum(MC_DATUM); // 中央揃え
    clearTimer(); // 初期表示
}

void loop() {
    M5.update(); // ボタンの状態を更新

    bool isPowerButtonPressed = M5.Axp.GetBtnPress() == 0x02;
    bool isButtonAPressed = M5.BtnA.wasPressed();

    // 電源ボタン（PMU経由）と表のボタン (BtnA) の処理
    if (isPowerButtonPressed || isButtonAPressed) {
        if (currentState == IDLE) {
            if (isPowerButtonPressed) {
                // 10秒からカウントダウン開始
                totalSeconds = 10;
            }
            if (isButtonAPressed) {
                // 60分 (3600秒) からカウントダウン開始
                totalSeconds = 60 * 60;
            }
            remainingSeconds = totalSeconds;
            currentState = COUNTING;
            lastUpdateTime = millis(); // カウント開始時刻を記録
            updateDisplay(); // 画面更新
        } else if (currentState == COUNTING) {
            // カウントダウン中に押すと一時停止
            currentState = PAUSED;
            updateDisplay(); // 画面更新
        } else if (currentState == PAUSED) {
            // 一時停止中に押すと再開
            currentState = COUNTING;
            lastUpdateTime = millis(); // 再開時刻を記録
            updateDisplay(); // 画面更新
        } else if (currentState == FINISHED) {
            // 終了中に押すとクリア
            clearTimer();
        }
    }

    // 上のボタン (BtnB) を押すとクリア
    if (M5.BtnB.wasPressed()) {
        clearTimer();
    }

    // カウントダウン処理
    if (currentState == COUNTING) {
        if (millis() - lastUpdateTime >= 1000) { // 1秒経過したら
            if (remainingSeconds > 0) {
                remainingSeconds--;
                lastUpdateTime = millis();
                updateDisplay(); // 1秒ごとに画面更新
            } else {
                currentState = FINISHED;
                updateDisplay(); // 終了時の画面更新
                melodyIndex = 0; // メロディを最初から再生
            }
        }
    } else if (currentState == FINISHED) {
        playMelodyTick(); // メロディを少しずつ再生
    }
}

// 残り時間を画面に表示する関数
void drawTime(long seconds) {
    int minutes = seconds / 60;
    int secs = seconds % 60;
    char timeStr[10];
    sprintf(timeStr, "%02d:%02d", minutes, secs);
    // 画面全体をクリアしてから描画することでちらつきを抑える
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextFont(TIME_FONT_ID); // 時間表示用フォントを設定
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.drawString(timeStr, M5.Lcd.width() / 2, M5.Lcd.height() / 2);
}

// 楽しいメロディを少しずつ再生する関数 (非ブロッキング)
void playMelodyTick() {
    static unsigned long lastToneTime = 0;
    static int currentMelodyDuration = 0;

    if (millis() - lastToneTime >= currentMelodyDuration) {
        // 次の音へ
        if (melodyIndex < melodyLength) {
            int toneFreq = melody[melodyIndex * 2];
            currentMelodyDuration = melody[melodyIndex * 2 + 1];

            if (toneFreq == 0) { // 無音の場合
                M5.Beep.mute();
            } else {
                M5.Beep.tone(toneFreq, currentMelodyDuration);
            }
            lastToneTime = millis();
            melodyIndex++;
        } else {
            melodyIndex = 0; // メロディの終わりに達したら最初に戻る
        }
    }
}

// メロディを停止する関数
void stopMelody() {
    M5.Beep.mute();
}

// タイマーをクリアする関数
void clearTimer() {
    currentState = IDLE;
    totalSeconds = 0;
    remainingSeconds = 0;
    stopMelody();
    updateDisplay(); // 画面更新
}

// 背景画像を描画する関数
void drawBackground() {
    M5.Lcd.fillScreen(BLACK);
#ifdef USE_BACKGROUND_IMAGE
    M5.Lcd.startWrite();
    M5.Lcd.pushImage(0, 0, imgWidth, imgHeight, img);
    M5.Lcd.endWrite();
#endif
}

// 全ての画面表示を管理する関数 (必要な時にだけ呼び出す)
void updateDisplay() {
    switch (currentState) {
        case IDLE:
            drawBackground();
            M5.Lcd.setTextSize(2);
            M5.Lcd.setTextFont(TEXT_FONT_ID); // テキスト用フォントを設定
            M5.Lcd.setTextColor(GREEN);//, BLACK);
            M5.Lcd.drawString("READY", M5.Lcd.width() / 2, M5.Lcd.height() / 2);
            break;
        case COUNTING:
        case PAUSED:
            M5.Lcd.fillScreen(BLACK); // 画面を一度クリア
            drawTime(remainingSeconds); // drawTime関数内でフォントを設定
            break;
        case FINISHED:
            M5.Lcd.fillScreen(BLACK); // 画面を一度クリア
            M5.Lcd.fillScreen(RED); // 派手な画面
            M5.Lcd.setTextSize(2);
            M5.Lcd.setTextFont(TEXT_FONT_ID); // テキスト用フォントを設定
            M5.Lcd.setTextColor(BLACK, RED);
            M5.Lcd.drawString("TIME UP!", M5.Lcd.width() / 2, M5.Lcd.height() / 2);
            break;
    }
}
