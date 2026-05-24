#include "image_fetcher.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <M5StickCPlus.h>
#include "settings.h"
#include "wifi_conn.h"
#include <time.h>

static uint16_t* imgBuffer = nullptr;

static const int IMG_W = 240;
static const int IMG_H = 133;

void imageFetcherSetup() {
    imgBuffer = (uint16_t*)malloc(IMG_W * IMG_H * 2);
    // 確保失敗時は nullptr のまま
}

bool shouldRefetchToday() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return false;
    }
    char today[12];
    strftime(today, sizeof(today), "%Y-%m-%d", &timeinfo);
    return settings.fetchDate != String(today);
}

void drawFetchedBackground() {
    if (!wifiIsConnected() || settings.proxyUrl.length() == 0) {
        return;
    }

    if (shouldRefetchToday()) {
        String url = settings.proxyUrl + "?topic=" + settings.mainTopic + "&sub=" + settings.subTopic;

        WiFiClientSecure client;
        client.setInsecure();

        HTTPClient http;
        http.begin(client, url);
        int code = http.GET();

        if (code == 200) {
            int len = http.getSize();
            if (len == IMG_W * IMG_H * 2 && imgBuffer != nullptr) {
                WiFiClient* stream = http.getStreamPtr();
                int read = 0;
                uint8_t* buf = (uint8_t*)imgBuffer;
                while (http.connected() && read < len) {
                    int avail = stream->available();
                    if (avail > 0) {
                        int toRead = (avail > (len - read)) ? (len - read) : avail;
                        stream->readBytes(buf + read, toRead);
                        read += toRead;
                    }
                }
                if (read == len) {
                    // 今日の日付を保存
                    struct tm timeinfo;
                    if (getLocalTime(&timeinfo)) {
                        char today[12];
                        strftime(today, sizeof(today), "%Y-%m-%d", &timeinfo);
                        settings.fetchDate = String(today);
                        settingsSave();
                    }
                }
            }
        }
        http.end();
    }

    if (imgBuffer != nullptr) {
        M5.Lcd.startWrite();
        M5.Lcd.pushImage(0, 1, IMG_W, IMG_H, imgBuffer);
        M5.Lcd.endWrite();
    }
}
