# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## プロジェクト概要

M5StickCPlus 向けのシンプルなカウントダウンタイマー。PlatformIO + Arduino フレームワークで開発。

## よく使うコマンド

```bash
# ビルド
pio run

# ビルド＆書き込み
pio run --target upload

# シリアルモニタ
pio device monitor

# 書き込み＆モニタ (一括)
pio run --target upload && pio device monitor
```

## アーキテクチャ

エントリーポイントは `src/main.cpp` のみ。状態機械でタイマーの動作を管理している。

### タイマー状態

```
IDLE → COUNTING → PAUSED → COUNTING → FINISHED → IDLE
```

- `IDLE`: 待機中。背景画像と "READY" を表示
- `COUNTING`: カウントダウン中。1秒ごとに `updateDisplay()` を呼ぶ
- `PAUSED`: 一時停止。画面は最後の残り時間のまま
- `FINISHED`: タイムアップ。赤画面 + メロディをループ再生

### ボタン操作

| ボタン | 効果 |
|--------|------|
| BtnA (表) | IDLE: 60分スタート / COUNTING: 一時停止 / PAUSED: 再開 / FINISHED: クリア |
| 電源ボタン (PMU経由) | 上記と同様だが IDLE 時は 10秒スタート（テスト用） |
| BtnB (上) | 任意のタイミングでクリア |

### 表示の仕組み

`updateDisplay()` は**状態変化時のみ**呼び出す設計（毎フレーム描画しない）。これにより画面のちらつきを抑えている。フォントは時間表示に ID 7（7セグ風）、テキストに ID 4 を使用。

### メロディ再生

`playMelodyTick()` を `loop()` 内で毎フレーム呼び出す非ブロッキング方式。`melody[]` 配列に周波数と再生時間（ms）のペアを並べ、`melodyIndex` で進捗を管理。メロディ終端に達したら `melodyIndex = 0` でループ。

### 背景画像

`src/image.h` に RGB565 形式のピクセルデータを `PROGMEM` 配列として保持。`#define USE_BACKGROUND_IMAGE` を定義すると `drawBackground()` 内で `pushImage()` により描画される。画像データの生成は https://lang-ship.com/tools/image2data/ を使用。

## 背景画像を差し替えるには

1. 上記ツールで PNG → C ヘッダ変換
2. `src/image.h` を差し替え
3. `imgWidth` / `imgHeight` が画像サイズと一致していることを確認
4. `main.cpp` 冒頭の `#include "image.h"` と `#define USE_BACKGROUND_IMAGE` がコメントインされていることを確認
