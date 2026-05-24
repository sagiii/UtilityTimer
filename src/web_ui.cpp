#include "web_ui.h"
#include <WebServer.h>
#include "settings.h"

static WebServer server(80);

static String buildHtml() {
    String html = "<!DOCTYPE html><html><head><meta charset='utf-8'>"
                  "<title>UtilityTimer 設定</title>"
                  "<style>body{font-family:sans-serif;max-width:480px;margin:20px auto;padding:0 12px}"
                  "h2{margin-bottom:8px}fieldset{margin-bottom:12px;padding:8px}"
                  "label{display:inline-block;margin:2px 6px}"
                  "input[type=text]{width:100%;box-sizing:border-box}"
                  "button{padding:8px 24px;font-size:1em}"
                  "</style></head><body>"
                  "<h2>UtilityTimer 設定</h2>"
                  "<form method='POST' action='/api/settings'>";

    // BtnA 時間
    html += "<fieldset><legend>BtnA タイマー時間 (分)</legend>";
    for (int i = 0; i < BTN_A_PRESET_COUNT; i++) {
        String checked = (i == settings.btnAIdx) ? " checked" : "";
        html += "<label><input type='radio' name='btnAIdx' value='" + String(i) + "'" + checked + "> "
                + String(BTN_A_PRESETS[i]) + "</label>";
    }
    html += "</fieldset>";

    // 電源ボタン時間
    html += "<fieldset><legend>電源ボタン タイマー時間 (秒)</legend>";
    for (int i = 0; i < PWR_PRESET_COUNT; i++) {
        String checked = (i == settings.pwrIdx) ? " checked" : "";
        html += "<label><input type='radio' name='pwrIdx' value='" + String(i) + "'" + checked + "> "
                + String(PWR_PRESETS[i]) + "</label>";
    }
    html += "</fieldset>";

    // 背景画像ソース
    html += "<fieldset><legend>背景画像ソース</legend>";
    html += "<label><input type='radio' name='imgSource' value='0'"
            + String(settings.imgSource == IMG_STATIC ? " checked" : "") + "> 静的</label>";
    html += "<label><input type='radio' name='imgSource' value='1'"
            + String(settings.imgSource == IMG_WEB_FETCH ? " checked" : "") + "> Web フェッチ</label>";
    html += "</fieldset>";

    // メイントピック
    html += "<fieldset><legend>メイントピック</legend><select name='mainTopic'>";
    const char* topics[] = {"minecraft", "terraria", "stardew", "factorio", "pokemon"};
    for (int i = 0; i < 5; i++) {
        String sel = (settings.mainTopic == topics[i]) ? " selected" : "";
        html += "<option value='" + String(topics[i]) + "'" + sel + ">" + String(topics[i]) + "</option>";
    }
    html += "</select></fieldset>";

    // サブトピック
    html += "<fieldset><legend>サブトピック</legend>"
            "<input type='text' name='subTopic' value='" + settings.subTopic + "'></fieldset>";

    // READY 文字
    String readyChecked = settings.showReady ? " checked" : "";
    html += "<fieldset><legend>READY 文字</legend>"
            "<label><input type='checkbox' name='show_ready' value='1'" + readyChecked + "> 表示する</label>"
            "</fieldset>";

    // プロキシ URL
    html += "<fieldset><legend>プロキシ URL</legend>"
            "<input type='text' name='proxyUrl' value='" + settings.proxyUrl + "' placeholder='https://...'>"
            "</fieldset>";

    html += "<button type='submit'>保存</button></form></body></html>";
    return html;
}

static void handleRoot() {
    server.send(200, "text/html; charset=utf-8", buildHtml());
}

static void handlePostSettings() {
    if (server.hasArg("btnAIdx")) {
        int v = server.arg("btnAIdx").toInt();
        if (v >= 0 && v < BTN_A_PRESET_COUNT) settings.btnAIdx = v;
    }
    if (server.hasArg("pwrIdx")) {
        int v = server.arg("pwrIdx").toInt();
        if (v >= 0 && v < PWR_PRESET_COUNT) settings.pwrIdx = v;
    }
    if (server.hasArg("imgSource")) {
        settings.imgSource = (ImgSource)server.arg("imgSource").toInt();
    }
    if (server.hasArg("mainTopic")) {
        settings.mainTopic = server.arg("mainTopic");
    }
    if (server.hasArg("subTopic")) {
        settings.subTopic = server.arg("subTopic");
    }
    settings.showReady = server.hasArg("show_ready");
    if (server.hasArg("proxyUrl")) {
        settings.proxyUrl = server.arg("proxyUrl");
    }

    settingsSave();
    server.sendHeader("Location", "/", true);
    server.send(303);
}

void webUiSetup() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/settings", HTTP_POST, handlePostSettings);
    server.begin();
}

void webUiHandle() {
    server.handleClient();
}
