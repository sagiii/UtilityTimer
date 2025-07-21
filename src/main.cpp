#include <M5StickCPlus.h>

// タイマーの状態
enum TimerState {
    IDLE,
    COUNTING,
    PAUSED,
    FINISHED
};

TimerState currentState = IDLE;

// タイマーの時間 (秒単位)
long totalSeconds = 0;
long remainingSeconds = 0;

// 画面表示用のフォント
#define FONT_TEXT (4)
#define FONT_7SEG (7) // 画面に大きく表示するためのフォント

// メロディの定義 (例: マリオのテーマの一部)
// 音階と音の長さ (ミリ秒) のペア
const int melody[] = {
    262, 100, // C4
    330, 100, // E4
    392, 100, // G4
    523, 200, // C5
    // 適宜追加
};
const int melodyLength = sizeof(melody) / sizeof(melody[0]) / 2;

// 関数プロトタイプ
void drawTime(long seconds);
void playMelody();
void stopMelody();
void clearTimer();

void setup() {
    M5.begin();
    M5.Lcd.setRotation(1); // 画面を横向きに設定 (TypeCコネクタが右の場合)
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setTextDatum(MC_DATUM); // 中央揃え
    clearTimer();
}

void loop() {
    M5.update(); // ボタンの状態を更新

    // 上のボタン (BtnB) を押すとクリア
    if (M5.BtnB.wasPressed()) {
        clearTimer();
    }

    // 表のボタン (BtnA) の処理
    if (M5.BtnA.wasPressed()) {
        switch (currentState) {
            case IDLE:
                // 60分 (3600秒) からカウントダウン開始
                totalSeconds = 10;// * 60;
                remainingSeconds = totalSeconds;
                currentState = COUNTING;
                M5.Lcd.fillScreen(BLACK);
                break;
            case COUNTING:
                // カウントダウン中に押すと一時停止
                currentState = PAUSED;
                break;
            case PAUSED:
                // 一時停止中に押すと再開
                currentState = COUNTING;
                break;
            case FINISHED:
                // 終了中に押すとクリア
                clearTimer();
                break;
        }
    }

    // カウントダウン処理
    if (currentState == COUNTING) {
        if (remainingSeconds > 0) {
            drawTime(remainingSeconds);
            delay(1000); // 1秒待機
            remainingSeconds--;
        } else {
            currentState = FINISHED;
            M5.Lcd.fillScreen(RED); // 派手な画面
            M5.Lcd.setTextColor(BLACK, RED);
            M5.Lcd.setTextFont(FONT_TEXT);
            M5.Lcd.drawString("TIME UP!", M5.Lcd.width() / 2, M5.Lcd.height() / 2);
            playMelody(); // メロディを再生
        }
    } else if (currentState == PAUSED) {
        // 一時停止中は何もせず、現在の時刻を表示し続ける
        drawTime(remainingSeconds);
    } else if (currentState == FINISHED) {
        // 終了状態ではメロディを繰り返し再生
        playMelody();
        delay(100); // メロディのループ間隔
    } else if (currentState == IDLE) {
        // 初期状態では "READY" を表示
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setTextColor(GREEN, BLACK);
        M5.Lcd.setTextFont(FONT_TEXT);
        M5.Lcd.drawString("READY", M5.Lcd.width() / 2, M5.Lcd.height() / 2);
    }
}

// 残り時間を画面に表示する関数
void drawTime(long seconds) {
    M5.Lcd.fillScreen(BLACK);
    int minutes = seconds / 60;
    int secs = seconds % 60;
    char timeStr[10];
    sprintf(timeStr, "%02d:%02d", minutes, secs);
    M5.Lcd.setTextFont(FONT_7SEG);
    M5.Lcd.drawString(timeStr, M5.Lcd.width() / 2, M5.Lcd.height() / 2);
}

// 楽しいメロディを再生する関数
void playMelody() {
    for (int i = 0; i < melodyLength; i++) {
        M5.Beep.tone(melody[i * 2], melody[i * 2 + 1]);
        delay(melody[i * 2 + 1] * 1.2); // 音の間隔を少し開ける
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
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setTextFont(FONT_TEXT);
    M5.Lcd.drawString("READY", M5.Lcd.width() / 2, M5.Lcd.height() / 2);
}
