#include "ota_updater.h"
#include <ArduinoOTA.h>
#include <M5StickCPlus.h>

void otaSetup() {
    ArduinoOTA.setHostname("utiltimer");

    ArduinoOTA.onStart([]() {
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setTextDatum(MC_DATUM);
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextFont(2);
        M5.Lcd.setTextColor(WHITE);
        M5.Lcd.drawString("OTA Updating...", M5.Lcd.width() / 2, M5.Lcd.height() / 2);
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        char buf[16];
        sprintf(buf, "%u%%", progress * 100 / total);
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setTextDatum(MC_DATUM);
        M5.Lcd.setTextSize(2);
        M5.Lcd.setTextFont(2);
        M5.Lcd.setTextColor(WHITE);
        M5.Lcd.drawString(buf, M5.Lcd.width() / 2, M5.Lcd.height() / 2);
    });

    ArduinoOTA.onEnd([]() {
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setTextDatum(MC_DATUM);
        M5.Lcd.setTextSize(2);
        M5.Lcd.setTextFont(2);
        M5.Lcd.setTextColor(GREEN);
        M5.Lcd.drawString("Done!", M5.Lcd.width() / 2, M5.Lcd.height() / 2);
    });

    ArduinoOTA.begin();
}

void otaHandle() {
    ArduinoOTA.handle();
}
