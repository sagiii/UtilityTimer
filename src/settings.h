#pragma once
#include <WString.h>

enum ImgSource : int { IMG_STATIC = 0, IMG_WEB_FETCH = 1 };

const int BTN_A_PRESETS[]    = {10, 20, 30, 45, 60, 75, 90, 120};
const int BTN_A_PRESET_COUNT = 8;
const int PWR_PRESETS[]      = {10, 20, 30, 40, 50, 60};
const int PWR_PRESET_COUNT   = 6;

struct AppSettings {
    int       btnAIdx;
    int       pwrIdx;
    ImgSource imgSource;
    String    mainTopic;
    String    subTopic;
    bool      showReady;
    String    proxyUrl;
    String    fetchDate;
};

extern AppSettings settings;

void settingsLoad();
void settingsSave();
long getBtnASeconds();
long getPwrSeconds();
