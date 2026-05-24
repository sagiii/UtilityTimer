## やりたい改善

### 完了済み ✅
- [x] WiFi設定できるようにする（WiFiManager でキャプティブポータル）
- [x] タイマーの時間をweb経由で設定可能にする（`http://utiltimer.local` で設定UI）
  - [x] BtnA の時間の選択肢（分）：10, 20, 30, 45, 60, 75, 90, 120
  - [x] 電源ボタン の時間の選択肢（秒）：10, 20, 30, 40, 50, 60
- [x] 設定のnon volatile化（ESP32 Preferences / NVS）
- [x] OTA対応（ArduinoOTA、`utiltimer.local` で書き込み可能）
- [x] READY画面の背景コンテンツをリッチに
  - [x] WebフェッチかつランダムなゲームWiki素材を日替わりで表示（READY表示のたびに取得）
  - [x] メイントピック選択（minecraft, terraria, stardew, factorio, pokemon）
  - [x] サブトピックはWeb UI から自由入力
  - [x] プロキシ（GCP Cloud Functions）で WikiMedia → RGB565 変換
  - [x] アスペクト比を保って中央配置（マージン15px）
  - [x] READY文字のon/off

### 未着手 🔲
- [ ] WiFi再設定手段（現状フラッシュ消去しかない）
  - BtnB 長押しで WiFiManager リセット、または Web UI から「WiFiリセット」ボタン
- [ ] 対応トピックの拡充・調整
  - 各 Wiki のカテゴリ構造に合わせた細かいチューニング
  - pokemon（bulbapedia）は未検証
