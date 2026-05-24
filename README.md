# UtilityTimer

M5StickCPlus で動くシンプルなカウントダウンタイマー。  
WiFi 接続後はブラウザからタイマー時間や背景画像の設定が可能。

---

## 基本操作

| ボタン | 動作 |
|--------|------|
| BtnA（表） | 待機中: タイマー開始 / カウント中: 一時停止 / 一時停止中: 再開 / 終了時: クリア |
| BtnB（上） | 任意のタイミングでクリア |
| 電源ボタン | BtnA と同様（待機中の開始時間のみ異なる。テスト用） |

---

## セットアップ

### 1. ファームウェアのビルドと書き込み

[PlatformIO](https://platformio.org/) が必要。VS Code 拡張または CLI で利用可能。

```bash
# ビルド
pio run

# M5StickCPlus を USB 接続してから書き込み
pio run --target upload

# シリアルモニタ（デバッグ確認用）
pio device monitor
```

### 2. WiFi 設定（初回のみ・デバイス側）

初回起動時（または WiFi 未設定時）はデバイスが **AP モード**に入る。

1. M5 の画面に表示された SSID（例: `M5Timer-Setup`）にスマホ/PCで接続
2. 設定画面が自動でポップアップ表示される（キャプティブポータル）
   - iOS/macOS: 「ネットワークにサインイン」が自動表示
   - Android: 「Wi-Fi ネットワークにサインイン」通知をタップ
   - Windows: タスクバーの通知をクリック
   - 自動表示されない場合は `http://192.168.4.1` を手動で開く
3. 接続したい WiFi の SSID とパスワードを入力して保存
4. デバイスが自動で再起動し、指定の WiFi に接続される
5. 接続後は IDLE 画面に `utiltimer.local` と表示される

> 再設定したい場合は BtnB を長押し（予定）またはフラッシュを消去して再書き込み。

### 3. タイマー・画像の設定（Web UI）

デバイスが WiFi 接続済みの状態で、**同じネットワーク内のブラウザ**から設定できる。

1. ブラウザで `http://utiltimer.local` を開く
2. 以下の項目を設定して保存

| 設定項目 | 内容 |
|----------|------|
| BtnA タイマー時間 | 10 / 20 / 30 / 45 / 60 / 75 / 90 / 120 分から選択 |
| 電源ボタン タイマー時間 | 10 / 20 / 30 / 40 / 50 / 60 秒から選択 |
| 背景画像ソース | 静的（書き込み済み画像）/ Web フェッチ（READY 表示のたびにランダム取得）から選択 |
| メイントピック | Web フェッチ時の画像カテゴリ（Minecraft など） |
| サブトピック | カテゴリを絞り込むワード（mob, block など） |
| READY 文字 | IDLE 画面の文字表示 ON/OFF |
| プロキシ URL | Web フェッチ用 Cloud Functions の URL |

---

## OTA アップデート（WiFi 書き込み）

WiFi 接続済みのデバイスには USB なしで書き込める。

1. `platformio.ini` に以下を追加（mDNS で名前解決されるため IP 不要）

```ini
upload_protocol = espota
upload_port = utiltimer.local
```

2. 通常通り書き込み

```bash
pio run --target upload
```

---

## 背景画像を差し替える（静的）

1. [lang-ship.com/tools/image2data](https://lang-ship.com/tools/image2data/) で PNG を RGB565 の C ヘッダに変換
2. `src/image.h` を差し替え（`imgWidth` / `imgHeight` がサイズと一致していること）
3. `main.cpp` 冒頭の `#include "image.h"` と `#define USE_BACKGROUND_IMAGE` がコメントインされていること
4. 再ビルド・書き込み

---

## プロキシサーバーのセットアップ（Web フェッチ機能を使う場合）

背景画像の Web フェッチには GCP Cloud Functions が必要。READY 画面表示のたびにランダムな画像を取得する。

### 前提

- Google アカウント（請求先アカウント設定済み）
- [Google Cloud CLI (`gcloud`)](https://cloud.google.com/sdk/docs/install) のインストールと認証

```bash
gcloud auth login
```

### GCP プロジェクト作成

```bash
# プロジェクト作成（ID は任意・グローバルユニークである必要あり）
gcloud projects create utilitytimer-proxy --name="UtilityTimer Proxy"

# 作成したプロジェクトをデフォルトに設定
gcloud config set project utilitytimer-proxy

# 請求先アカウントを紐づけ（請求先 ID は Google Cloud Console で確認）
gcloud billing projects link utilitytimer-proxy \
  --billing-account=XXXXXX-XXXXXX-XXXXXX

# 必要な API を有効化
gcloud services enable cloudfunctions.googleapis.com cloudbuild.googleapis.com artifactregistry.googleapis.com run.googleapis.com

# デプロイユーザーに serviceAccountUser ロールを付与（Gen2 が Cloud Run を使うために必要）
gcloud iam service-accounts add-iam-policy-binding \
  $(gcloud projects describe $(gcloud config get-value project) --format='value(projectNumber)')-compute@developer.gserviceaccount.com \
  --member="user:$(gcloud config get-value account)" \
  --role="roles/iam.serviceAccountUser"
```

### Cloud Functions のデプロイ

```bash
# proxy/ ディレクトリに移動してデプロイ
gcloud functions deploy get-image \
  --gen2 \
  --runtime=python312 \
  --region=asia-northeast1 \
  --source=./proxy \
  --entry-point=get_image \
  --trigger-http \
  --allow-unauthenticated \
  --memory=256MB \
  --timeout=30s
```

デプロイ完了後、表示される URL（`https://get-image-XXXXXXXX-an.a.run.app`）を控える。

### デバイスへの URL 設定

デバイスの Web UI（`http://utiltimer.local`）を開き、**プロキシ URL** 欄に上記 URL を貼り付けて保存。

### 動作確認

ブラウザから直接叩いて RGB565 バイナリが返ってくれば OK。

```
https://<functions-url>?topic=minecraft&sub=mob
```

---

## 対応メイントピック

| トピック名 | ソース Wiki | サブトピック例 |
|-----------|------------|--------------|
| minecraft | minecraft.fandom.com | mob, block, item, biome |
| pokemon | bulbapedia.bulbagarden.net | gen1, legendary, starter |
| terraria | terraria.wiki.gg | boss, npc, weapon, armor |
| stardew | stardewvalleywiki.com | crop, fish, character |
| factorio | wiki.factorio.com | machine, item, enemy |
