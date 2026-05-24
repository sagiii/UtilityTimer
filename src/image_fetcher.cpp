#include "image_fetcher.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <M5StickCPlus.h>
#include "settings.h"
#include "wifi_conn.h"

static uint16_t* imgBuffer = nullptr;

static const int IMG_W = 240;
static const int IMG_H = 133;

void imageFetcherSetup() {
    imgBuffer = (uint16_t*)malloc(IMG_W * IMG_H * 2);
}

void drawFetchedBackground() {
    if (!wifiIsConnected() || settings.proxyUrl.length() == 0) {
        return;
    }

    // フェッチ中表示
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextFont(2);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.drawString("Loading...", IMG_W / 2, IMG_H / 2);

    String url = settings.proxyUrl
                 + "?topic=" + settings.mainTopic
                 + "&sub="   + settings.subTopic;

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.begin(client, url);
    http.setTimeout(20000);
    int code = http.GET();

    bool ok = false;
    if (code == 200 && imgBuffer != nullptr) {
        int len = http.getSize();
        if (len == IMG_W * IMG_H * 2) {
            WiFiClient* stream = http.getStreamPtr();
            uint8_t* buf = (uint8_t*)imgBuffer;
            int read = 0;
            while (http.connected() && read < len) {
                int avail = stream->available();
                if (avail > 0) {
                    int toRead = (avail > len - read) ? len - read : avail;
                    stream->readBytes(buf + read, toRead);
                    read += toRead;
                }
            }
            ok = (read == len);
        }
    }
    http.end();

    // 成否にかかわらず imgBuffer に前回データがあれば表示
    if (imgBuffer != nullptr) {
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.startWrite();
        M5.Lcd.pushImage(0, 1, IMG_W, IMG_H, imgBuffer);
        M5.Lcd.endWrite();
    }
}
