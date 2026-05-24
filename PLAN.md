# 実装計画

TODO.md の内容を技術的な実装案に落とし込んだドキュメント。

---

## 全体方針

WiFi基盤 → 設定永続化 → Web設定UI → OTA → 画像フェッチ の順に段階的に積み上げる。
各フェーズは独立してマージ・動作確認できるように設計する。

---

## フェーズ 1: WiFi 基盤 + 設定永続化（NVS）

### WiFi 接続

- `WiFiManager` ライブラリを使用（`tzapu/WiFiManager`）
- 初回起動時または未設定時は AP モードに入り、M5 の画面に SSID を表示
- スマホから `192.168.4.1` に接続して SSID/パスワードを入力
- 設定完了後は自動で Station モードに切り替え

```
platformio.ini に追加:
  lib_deps += tzapu/WiFiManager@^2.0.17
```

### NVS（Non-Volatile Storage）

ESP32 の Preferences ライブラリで設定を Flash に保存。保存するキー：

| キー | 型 | 内容 |
|------|------|------|
| `btnA_idx` | int | BtnA プリセットの選択インデックス |
| `pwr_idx` | int | 電源ボタン プリセットの選択インデックス |
| `img_src` | int | 0=static, 1=web_fetch |
| `main_topic` | String | 画像取得のメイントピック |
| `sub_topic` | String | 画像取得のサブトピック |
| `show_ready` | bool | READY 文字の表示/非表示 |
| `fetch_date` | String | 最後に画像を取得した日付 (YYYY-MM-DD) |

WiFi 認証情報は WiFiManager が内部で NVS に保存するため別途管理不要。

### コード構造変更

現在の単一ファイル構成からモジュール分割：

```
src/
  main.cpp          # setup/loop のみ残す
  settings.h/.cpp   # Preferences ラッパー、デフォルト値定義
  wifi_conn.h/.cpp  # WiFiManager の初期化・再接続ロジック
  image.h           # 既存の静的画像データ（変更なし）
```

---

## フェーズ 2: Web 設定 UI + タイマープリセット変更

### Web サーバー + mDNS

ESP32 の `WebServer` ライブラリで HTTP サーバーを立てる（ポート 80）。
`ESPmDNS` ライブラリ（ESP32 Arduino に同梱）で mDNS を有効化し、**`http://utilitytimer.local`** でアクセスできるようにする。

```cpp
#include <ESPmDNS.h>

// WiFi 接続後に呼ぶ
MDNS.begin("utiltimer");   // → http://utiltimer.local
```

M5 の画面には `utiltimer.local` を表示する（IP 表示は不要になる）。
ArduinoOTA も mDNS 経由で解決されるため、OTA の `upload_port` も `utiltimer.local` で固定できる。

### エンドポイント設計

| メソッド | パス | 内容 |
|---------|------|------|
| GET | `/` | 設定画面 HTML |
| GET | `/api/settings` | 現在の設定を JSON で返す |
| POST | `/api/settings` | 設定を更新して NVS に保存 |
| POST | `/api/restart` | デバイスを再起動 |

### 設定画面の項目

- **BtnA プリセット選択**: ラジオボタン `10 / 20 / 30 / 45 / 60 / 75 / 90 / 120 分`
- **電源ボタン プリセット選択**: ラジオボタン `10 / 20 / 30 / 40 / 50 / 60 秒`
- **画像ソース**: 静的(`image.h`) か Web フェッチかを選択
- **メイントピック**: ドロップダウン（後述のプリセット候補から）
- **サブトピック**: テキスト入力
- **READY 文字**: チェックボックス（表示/非表示）

HTML は `WebServer::on("/", ...)` 内にリテラル文字列として埋め込む（ファイルシステム不使用）。

### コード追加

```
src/
  web_ui.h/.cpp     # WebServer 初期化・ルーティング・HTML 生成
```

---

## フェーズ 3: OTA 対応

### 実装

`ArduinoOTA` ライブラリ（ESP-IDF 同梱）を `setup()` で初期化。
WiFi 接続後に自動で待機状態になる。PlatformIO から以下でアップロード可能：

```ini
; platformio.ini に追加
upload_protocol = espota
upload_port = <デバイスのIP>
```

OTA 中は M5 画面に進捗バーを表示する。

```
src/
  ota_updater.h/.cpp  # ArduinoOTA ラッパー・進捗コールバック
```

---

## フェーズ 4: READY 画面リッチ化（画像 Web フェッチ）

### 方針

M5StickCPlus 単体で HTTPS + 画像デコードをするのはメモリ・証明書管理の観点から難しい。
**Cloud Functions (第2世代) をプロキシとして RGB565 バイナリを取得する**方式を採用する。

```
[M5StickCPlus] -- HTTP --> [Cloud Functions] -- HTTPS --> [WikiMedia API / Wiki]
                                  ↑
                      画像取得・リサイズ・RGB565変換をここで処理
```

#### GCP プロジェクト構成

- **新規プロジェクト**を作成して会社アカウントの個人請求先に紐づける（既存業務プロジェクトと分離）
- リージョン: `asia-northeast1`（東京）で M5 からのレイテンシ最小化
- 有効化する API: Cloud Functions, Cloud Build, Artifact Registry

#### コスト見積もり

| 項目 | 想定 | 無料枠 |
|------|------|--------|
| 呼び出し回数 | 1回/日 × 365 = 365回/年 | 200万回/月 |
| コンピュート時間 | 〜2秒/回 × 365 = 730秒/年 | 40万秒/月 |
| 通信 | 〜500KB/回 × 365 = 約180MB/年 | 5GB/月 |

→ **実質無料枠内に収まる**

#### デプロイ方法

```bash
# 初回セットアップ
gcloud projects create utilitytimer-proxy --name="UtilityTimer Proxy"
gcloud config set project utilitytimer-proxy
gcloud services enable cloudfunctions.googleapis.com cloudbuild.googleapis.com

# デプロイ
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

認証なし (`--allow-unauthenticated`) で公開するが、URL が推測困難なランダム文字列になるため実用上の問題は低い。気になる場合は API キーをクエリパラメータで簡易認証する。

#### Cloud Functions 実装スケッチ

```
proxy/
  main.py           # エントリーポイント
  requirements.txt  # functions-framework, requests, Pillow
```

```python
# proxy/main.py
import functions_framework
import requests
import struct
from PIL import Image
import io, random

WIKI_APIS = {
    "minecraft": "https://minecraft.wiki/api.php",
    "terraria":  "https://terraria.wiki.gg/api.php",
}

@functions_framework.http
def get_image(request):
    topic = request.args.get("topic", "minecraft")
    sub   = request.args.get("sub", "mob")

    # 1. WikiMedia API でカテゴリ内画像リストを取得
    api_url = WIKI_APIS.get(topic, WIKI_APIS["minecraft"])
    resp = requests.get(api_url, params={
        "action": "query", "list": "allimages",
        "aicat": f"Category:{sub.capitalize()}",  # Wiki ごとに要調整
        "ailimit": "50", "format": "json",
    }, timeout=10)
    images = resp.json().get("query", {}).get("allimages", [])

    if not images:
        return b"", 404

    # 2. ランダムに 1 枚選んでダウンロード
    url = random.choice(images)["url"]
    img_data = requests.get(url, timeout=15).content

    # 3. リサイズ
    img = Image.open(io.BytesIO(img_data)).convert("RGBA")
    # 透過部分を黒で合成
    bg = Image.new("RGB", img.size, (0, 0, 0))
    bg.paste(img, mask=img.split()[3])
    img = bg.resize((240, 133), Image.LANCZOS)

    # 4. RGB565（リトルエンディアン）に変換
    out = bytearray()
    for r, g, b in img.getdata():
        val = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
        out += struct.pack("<H", val)

    return bytes(out), 200, {"Content-Type": "application/octet-stream"}
```

```
# proxy/requirements.txt
functions-framework==3.*
requests==2.*
Pillow==10.*
```

プロキシの処理：
1. メイントピック + サブトピックを受け取る
2. WikiMedia API でカテゴリ内画像を検索
3. ランダムに 1 枚を選択してダウンロード
4. 240×133 px にリサイズ（M5StickCPlus の画面サイズ）
5. RGB565（リトルエンディアン）に変換してバイナリで返却

M5 側 API コール例：
```
GET http://<proxy>/image?topic=minecraft&sub=mob&date=20260524
→ 240×133×2 bytes のバイナリ
```

### 日替わり制御

RTC または WiFi 時刻同期（`configTime()`）で現在日付を取得。
NVS の `fetch_date` と比較して日付が変わっていたら再取得。

### メイントピック候補（事前定義リスト）

Web フェッチ方式が有効な、無料かつ CC ライセンスの透過 PNG 素材が豊富な Wiki ベースのコンテンツ：

| メイントピック | Wiki/ソース | サブトピック例 |
|--------------|------------|--------------|
| Minecraft | minecraft.wiki | mob, block, item, biome |
| Terraria | terraria.wiki.gg | boss, npc, weapon, armor |
| Stardew Valley | stardewvalleywiki.com | crop, fish, character |
| Factorio | wiki.factorio.com | machine, item, enemy |
| Pokémon | bulbapedia.bulbagarden.net | gen1, legendary, starter |

サブトピックはテキスト自由入力だが、メイントピック選択時にヒント文字列をプレースホルダーとして表示する。

### コード追加

```
src/
  image_fetcher.h/.cpp  # HTTP GET でプロキシから RGB565 バイナリ取得・pushImage
```

### 静的 / Web フェッチ 切り替え

`drawBackground()` を以下のように分岐：

```cpp
void drawBackground() {
    if (settings.imgSource == IMG_STATIC) {
        // 既存の image.h を使う
        M5.Lcd.pushImage(0, 0, imgWidth, imgHeight, img);
    } else {
        // 日替わり画像をフェッチして表示
        imageFetcher.drawCachedOrFetch();
    }
}
```

---

## 依存ライブラリまとめ

```ini
lib_deps =
    m5stack/M5StickCPlus@^0.0.8
    tzapu/WiFiManager@^2.0.17       ; フェーズ1
    ; WebServer, ArduinoOTA は ESP32 Arduino に同梱のため追記不要
```

---

## 未決事項・検討ポイント

- Cloud Functions の URL を M5 側にどう持たせるか（Web 設定 UI で入力、NVS に保存が自然）
- WiFi 未接続時のフォールバック（静的 image.h に自動切り替えが自然）
- M5StickCPlus の RAM は約 520 KB。画像 1 枚 (240×133×2 = 63,840 bytes) をバッファするには余裕があるが、WebServer と同時保持は要検証
- OTA 時は既存のタイマー処理を止める必要あり（`ArduinoOTA.handle()` は loop 内で呼ぶ）
