# 実装計画

TODO.md の内容を技術的な実装案に落とし込んだドキュメント。

> **全フェーズ実装完了（2026-05-24）**

---

## 全体方針

WiFi基盤 → 設定永続化 → Web設定UI → OTA → 画像フェッチ の順に段階的に積み上げる。
各フェーズは独立してマージ・動作確認できるように設計する。

---

## フェーズ 1: WiFi 基盤 + 設定永続化（NVS）✅

### WiFi 接続

- `WiFiManager` ライブラリを使用（`tzapu/WiFiManager`）
- 初回起動時または未設定時は AP モードに入り、M5 の画面に SSID を表示
- スマホから `192.168.4.1` に接続して SSID/パスワードを入力（キャプティブポータル）
- 設定完了後は自動で Station モードに切り替え

```
platformio.ini に追加:
  lib_deps += tzapu/WiFiManager@^2.0.17
```

### NVS（Non-Volatile Storage）

ESP32 の Preferences ライブラリで設定を Flash に保存。保存するキー：

| キー | 型 | 内容 |
|------|------|------|
| `btnAIdx` | int | BtnA プリセットの選択インデックス |
| `pwrIdx` | int | 電源ボタン プリセットの選択インデックス |
| `imgSource` | int | 0=static, 1=web_fetch |
| `mainTopic` | String | 画像取得のメイントピック |
| `subTopic` | String | 画像取得のサブトピック |
| `showReady` | bool | READY 文字の表示/非表示 |
| `proxyUrl` | String | Cloud Functions の URL |

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

## フェーズ 2: Web 設定 UI + タイマープリセット変更 ✅

### Web サーバー + mDNS

ESP32 の `WebServer` ライブラリで HTTP サーバーを立てる（ポート 80）。
`ESPmDNS` ライブラリ（ESP32 Arduino に同梱）で mDNS を有効化し、**`http://utiltimer.local`** でアクセスできるようにする。

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

HTML は `WebServer::on("/", ...)` 内にリテラル文字列として埋め込む（ファイルシステム不使用）。

### コード追加

```
src/
  web_ui.h/.cpp     # WebServer 初期化・ルーティング・HTML 生成
```

---

## フェーズ 3: OTA 対応 ✅

`ArduinoOTA` ライブラリ（ESP-IDF 同梱）を `setup()` で初期化。
WiFi 接続後に自動で待機状態になる。PlatformIO から以下でアップロード可能：

```ini
; platformio.ini に追加
upload_protocol = espota
upload_port = utiltimer.local
```

OTA 中は M5 画面に進捗バーを表示する。

```
src/
  ota_updater.h/.cpp  # ArduinoOTA ラッパー・進捗コールバック
```

---

## フェーズ 4: READY 画面リッチ化（画像 Web フェッチ）✅

### 方針

M5StickCPlus 単体で HTTPS + 画像デコードをするのはメモリ・証明書管理の観点から難しい。
**Cloud Functions (第2世代) をプロキシとして RGB565 バイナリを取得する**方式を採用する。

```
[M5StickCPlus] -- HTTPS --> [Cloud Functions] -- HTTPS --> [WikiMedia API / Wiki]
                                   ↑
                       画像取得・リサイズ・RGB565変換をここで処理
```

#### GCP プロジェクト構成

- **新規プロジェクト**を作成して個人請求先に紐づける（既存業務プロジェクトと分離）
- リージョン: `asia-northeast1`（東京）
- 有効化する API: Cloud Functions, Cloud Build, Artifact Registry, Cloud Run
- デプロイユーザーに `roles/iam.serviceAccountUser` を Compute Engine default SA に対して付与（Gen2 の Cloud Run 利用に必要）

#### コスト見積もり

| 項目 | 想定 | 無料枠 |
|------|------|--------|
| 呼び出し回数 | 〜10回/日 × 365 = 3,650回/年 | 200万回/月 |
| コンピュート時間 | 〜2秒/回 × 3,650 = 7,300秒/年 | 40万秒/月 |
| 通信 | 〜64KB/回 × 3,650 = 約235MB/年 | 5GB/月 |

→ **実質無料枠内に収まる**（READY 表示のたびに取得しても余裕あり）

#### Cloud Functions 実装（実際の実装）

```python
# proxy/main.py
import functions_framework, requests, struct, random, io
from PIL import Image

TARGET_W, TARGET_H = 240, 133
HEADERS = {"User-Agent": "UtilityTimer/1.0"}

def get_minecraft_images():
    # minecraft.fandom.com は categorymembers + imageinfo の2ステップが必要
    # (minecraft.wiki は MediaWiki API 非対応)
    api = "https://minecraft.fandom.com/api.php"
    # Step1: カテゴリメンバー取得
    members = requests.get(api, params={
        "action": "query", "list": "categorymembers",
        "cmtitle": "Category:Mob_images", "cmtype": "file",
        "cmlimit": "50", "format": "json",
    }, headers=HEADERS, timeout=10).json()["query"]["categorymembers"]
    # Step2: imageinfo で URL を取得
    titles = "|".join(m["title"] for m in members[:50])
    pages = requests.get(api, params={
        "action": "query", "titles": titles,
        "prop": "imageinfo", "iiprop": "url|mime|size",
        "format": "json",
    }, headers=HEADERS, timeout=10).json()["query"]["pages"]
    return [
        p["imageinfo"][0]["url"] for p in pages.values()
        if p.get("imageinfo") and p["imageinfo"][0].get("mime") == "image/png"
        and 5000 < p["imageinfo"][0].get("size", 0) < 2_000_000
    ]

def get_terraria_images(sub):
    # wiki.gg は aisearch を無視するため aifrom でサブトピック起点の検索
    ...

def get_generic_images(api_url, sub):
    # aisearch でファイルを検索（stardew, factorio, pokemon）
    ...

@functions_framework.http
def get_image(request):
    topic = request.args.get("topic", "minecraft").lower()
    sub   = request.args.get("sub",   "mob").lower()

    urls = dispatch_by_topic(topic, sub)  # 上記各関数に振り分け
    img_url = random.choice(urls)

    # RGBA→黒背景合成、マージン15px内に収まるようアスペクト比保持でリサイズ、中央配置
    MARGIN = 15
    MAX_W, MAX_H = TARGET_W - MARGIN * 2, TARGET_H - MARGIN * 2  # 210 x 103
    img = Image.open(io.BytesIO(requests.get(img_url).content)).convert("RGBA")
    bg = Image.new("RGB", img.size, (0, 0, 0))
    bg.paste(img, mask=img.split()[3])
    scale = min(MAX_W / bg.width, MAX_H / bg.height)
    new_w, new_h = int(bg.width * scale), int(bg.height * scale)
    src = bg.resize((new_w, new_h), Image.LANCZOS)
    canvas = Image.new("RGB", (TARGET_W, TARGET_H), (0, 0, 0))
    canvas.paste(src, ((TARGET_W - new_w) // 2, (TARGET_H - new_h) // 2))

    # RGB565: M5Display の pushImage はビッグエンディアン（バイトスワップ済み）を期待する
    out = bytearray()
    for r, g, b in canvas.getdata():
        val = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
        out += struct.pack(">H", val)  # BE: M5Display の仕様に合わせる

    return bytes(out), 200, {"Content-Type": "application/octet-stream", ...}
```

### 画像取得タイミング

READY 画面を表示するたびに毎回取得（日時キャッシュなし）。取得失敗時は前回 imgBuffer の内容を表示する。

### タイムゾーン自動検出

WiFi 接続直後に `http://ip-api.com/json?fields=offset` から UTC オフセット（秒）を取得し、`configTime()` に渡す。取得失敗時は JST (+9h) にフォールバック。

### メイントピック候補

| メイントピック | Wiki/ソース | サブトピック例 | 取得方式 |
|--------------|------------|--------------|---------|
| Minecraft | minecraft.fandom.com | mob, block, item, biome | categorymembers + imageinfo |
| Terraria | terraria.wiki.gg | boss, npc, weapon, armor | allimages + aifrom |
| Stardew Valley | stardewvalleywiki.com | crop, fish, character | allimages + aisearch |
| Factorio | wiki.factorio.com | machine, item, enemy | allimages + aisearch |
| Pokémon | bulbapedia.bulbagarden.net | gen1, legendary, starter | allimages + aisearch（未検証） |

### コード追加

```
src/
  image_fetcher.h/.cpp  # HTTPS GET でプロキシから RGB565 バイナリ取得・pushImage
```

---

## 依存ライブラリまとめ

```ini
lib_deps =
    m5stack/M5StickCPlus@^0.0.8
    tzapu/WiFiManager@^2.0.17
    ; WebServer, ArduinoOTA, ESPmDNS は ESP32 Arduino に同梱のため追記不要
```

---

## 残課題

- **WiFi 再設定手段**: 現状フラッシュ消去しかない。BtnB 長押しで WiFiManager リセット、または Web UI から「WiFi リセット」ボタンを追加予定
- **対応トピックの拡充・調整**: 各 Wiki のカテゴリ構造に合わせた細かいチューニング
- **pokemon（bulbapedia）の検証**: aisearch が有効かどうか未確認
