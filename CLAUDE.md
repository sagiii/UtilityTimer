# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## プロジェクト概要

M5StickCPlus 向けのカウントダウンタイマー。PlatformIO + Arduino フレームワークで開発。
WiFi 接続後は `http://utiltimer.local` でタイマー時間・背景画像などをブラウザから設定可能。

## よく使うコマンド

```bash
# ビルド
pio run

# USB 書き込み
pio run --target upload

# OTA 書き込み（WiFi 接続済みの場合）
# platformio.ini に upload_protocol = espota / upload_port = utiltimer.local を設定してから
pio run --target upload

# シリアルモニタ
pio device monitor

# Cloud Functions デプロイ
gcloud functions deploy get-image \
  --gen2 --runtime=python312 --region=asia-northeast1 \
  --source=./proxy --entry-point=get_image \
  --trigger-http --allow-unauthenticated \
  --memory=256MB --timeout=30s
```

## アーキテクチャ

### ソースファイル構成

```
src/
  main.cpp            # setup/loop・タイマー状態機械・メロディ・表示
  settings.h/.cpp     # NVS（Preferences）による設定永続化
  wifi_conn.h/.cpp    # WiFiManager・mDNS・NTP+タイムゾーン自動取得
  web_ui.h/.cpp       # WebServer（http://utiltimer.local）設定UI
  ota_updater.h/.cpp  # ArduinoOTA（utiltimer.local 経由で書き込み）
  image_fetcher.h/.cpp# Cloud Functions からRGB565画像をフェッチ・表示
  image.h             # 静的背景画像データ（RGB565・バイトスワップ済み）
proxy/
  main.py             # GCP Cloud Functions（WikiMedia→RGB565変換プロキシ）
  requirements.txt
```

### タイマー状態機械

```
IDLE → COUNTING → PAUSED → COUNTING → FINISHED → IDLE
```

- `IDLE`: 待機中。背景画像 + "READY" 表示。Web フェッチ有効時は毎回画像を取得
- `COUNTING`: カウントダウン中。1秒ごとに `updateDisplay()` を呼ぶ
- `PAUSED`: 一時停止。画面は最後の残り時間のまま
- `FINISHED`: タイムアップ。赤画面 + メロディをループ再生

### ボタン操作

| ボタン | 効果 |
|--------|------|
| BtnA (表) | IDLE: タイマー開始（NVS設定値の分） / COUNTING: 一時停止 / PAUSED: 再開 / FINISHED: クリア |
| 電源ボタン (PMU経由) | 上記と同様だが IDLE 時は NVS設定値の秒でスタート（テスト用） |
| BtnB (上) | 任意のタイミングでクリア |

### 表示の仕組み

`updateDisplay()` は**状態変化時のみ**呼び出す設計（毎フレーム描画しない）。フォントは時間表示に ID 7（7セグ風）、テキストに ID 4 を使用。

### メロディ再生

`playMelodyTick()` を `loop()` 内で毎フレーム呼び出す非ブロッキング方式。`melody[]` 配列に周波数と再生時間（ms）のペアを並べ、`melodyIndex` で進捗を管理。

### 背景画像（静的）

`src/image.h` に RGB565 形式のピクセルデータを `PROGMEM` 配列として保持。
**バイト順は M5Display が期待するバイトスワップ済み形式**（ビッグエンディアンパック）。
画像データの生成は https://lang-ship.com/tools/image2data/ を使用。

### 背景画像（Web フェッチ）

`drawFetchedBackground()` が IDLE 表示のたびに呼ばれる。
- Cloud Functions URL に `?topic=minecraft&sub=mob` で GET
- レスポンスは 240×133×2 bytes の RGB565 バイナリ（バイトスワップ済み）
- マージン15px内にアスペクト比を保って収め、黒背景中央に配置済み
- フェッチ中は "Loading..." 表示、失敗時は前回画像を維持

### NVS 設定キー（Preferences namespace: `utiltimer`）

| キー | 型 | デフォルト |
|------|-----|-----------|
| `btnAIdx` | int | 4（60分） |
| `pwrIdx` | int | 0（10秒） |
| `imgSource` | int | 0（静的） |
| `mainTopic` | String | "minecraft" |
| `subTopic` | String | "mob" |
| `showReady` | bool | true |
| `proxyUrl` | String | "" |

### タイムゾーン自動取得

WiFi 接続後に `http://ip-api.com/json?fields=offset` で UTC オフセット（秒）を取得し `configTime` に設定。失敗時は JST(+9h) にフォールバック。

## 静的背景画像を差し替えるには

1. https://lang-ship.com/tools/image2data/ で PNG → C ヘッダ変換
2. `src/image.h` を差し替え（`imgWidth` / `imgHeight` がサイズと一致していること）
3. `main.cpp` 冒頭の `#include "image.h"` と `#define USE_BACKGROUND_IMAGE` がコメントインされていることを確認
