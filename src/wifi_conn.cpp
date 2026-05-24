#include "wifi_conn.h"
#include <M5StickCPlus.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>

void wifiSetup() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextFont(2);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.drawString("Connecting WiFi...", M5.Lcd.width() / 2, M5.Lcd.height() / 2);

    WiFiManager wm;
    wm.setConfigPortalTimeout(180);
    bool connected = wm.autoConnect("M5Timer-Setup");

    if (!connected) {
        // タイムアウト: WiFi なしで続行
        return;
    }

    MDNS.begin("utiltimer");
    configTime(9 * 3600, 0, "pool.ntp.org");
}

bool wifiIsConnected() {
    return WiFi.status() == WL_CONNECTED;
}
