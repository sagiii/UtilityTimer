#include "wifi_conn.h"
#include <M5StickCPlus.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>

// ip-api.com から UTC オフセット（秒）を取得。失敗時は JST(+9h) を返す
static long detectTimezoneOffset() {
    HTTPClient http;
    http.begin("http://ip-api.com/json?fields=offset");
    http.setTimeout(5000);
    int code = http.GET();
    if (code != 200) {
        http.end();
        return 9L * 3600;
    }
    String body = http.getString();
    http.end();

    // "offset":32400 の数値部分を簡易パース
    int idx = body.indexOf("\"offset\":");
    if (idx < 0) return 9L * 3600;
    int start = idx + 9;
    while (start < (int)body.length() && body[start] == ' ') start++;
    int end = start;
    if (end < (int)body.length() && body[end] == '-') end++;
    while (end < (int)body.length() && isDigit(body[end])) end++;
    if (end == start) return 9L * 3600;
    return body.substring(start, end).toInt();
}

void wifiSetup() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextFont(2);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.drawString("Connecting WiFi...", M5.Lcd.width() / 2, M5.Lcd.height() / 2);

    WiFiManager wm;
    wm.setConfigPortalTimeout(180);
    if (!wm.autoConnect("M5Timer-Setup")) {
        return;
    }

    MDNS.begin("utiltimer");

    long tzOffset = detectTimezoneOffset();
    configTime(tzOffset, 0, "pool.ntp.org");
}

bool wifiIsConnected() {
    return WiFi.status() == WL_CONNECTED;
}
