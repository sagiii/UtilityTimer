#include "settings.h"
#include <Preferences.h>

AppSettings settings;

static Preferences prefs;

void settingsLoad() {
    prefs.begin("utiltimer", true);
    settings.btnAIdx   = prefs.getInt("btnAIdx",   4);
    settings.pwrIdx    = prefs.getInt("pwrIdx",    0);
    settings.imgSource = (ImgSource)prefs.getInt("imgSource", 0);
    settings.mainTopic = prefs.getString("mainTopic", "minecraft");
    settings.subTopic  = prefs.getString("subTopic",  "mob");
    settings.showReady = prefs.getBool("showReady",   true);
    settings.proxyUrl  = prefs.getString("proxyUrl",  "");
    settings.fetchDate = prefs.getString("fetchDate", "");
    prefs.end();
}

void settingsSave() {
    prefs.begin("utiltimer", false);
    prefs.putInt("btnAIdx",   settings.btnAIdx);
    prefs.putInt("pwrIdx",    settings.pwrIdx);
    prefs.putInt("imgSource", (int)settings.imgSource);
    prefs.putString("mainTopic", settings.mainTopic);
    prefs.putString("subTopic",  settings.subTopic);
    prefs.putBool("showReady",   settings.showReady);
    prefs.putString("proxyUrl",  settings.proxyUrl);
    prefs.putString("fetchDate", settings.fetchDate);
    prefs.end();
}

long getBtnASeconds() {
    return (long)BTN_A_PRESETS[settings.btnAIdx] * 60;
}

long getPwrSeconds() {
    return (long)PWR_PRESETS[settings.pwrIdx];
}
