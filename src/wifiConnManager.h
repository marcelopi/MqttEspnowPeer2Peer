#pragma once
#ifdef ESP32
#include <WiFi.h>
#include <esp_wifi.h>
#include <Arduino.h>
#include "esp_now.h"
#include <Preferences.h>
#include <vector>
#include <ESPAsyncWebServer.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
extern "C" {
#include <user_interface.h> // para wifi_set_channel()
}
#include <espnow.h>
#include <EEPROM.h>
#define EEPROM_SIZE 1
#define EEPROM_ADDR_WIFI_MODE 0
#define EEPROM_ADDR_UPDATE_MODE 1

#define EEPROM_DEFAULT_VALUE 0xFF
#endif
#include <ArduinoOTA.h>

#define CHANNEL_AUTO 255
#define ESPNOW 0
#define WIFI 1
#define HYBRID 2

using UpdateModeCallback = std::function<void(const String& deviceName)>;


class wifiConnManager
{
public:
    static wifiConnManager *instance; // Ponteiro estático para a instância
    UpdateModeCallback onUpdateModeCallback;
    uint8_t begin(const char *deviceName, const char *ssid, const char *wifiPwd, const char *otaPwd,
               uint8_t defaultChannel, IPAddress localIP, IPAddress gateway,
               IPAddress subnet, IPAddress dns, int netMode = ESPNOW, const std::vector<String>& deviceOtaList = {});

    wifiConnManager();

    void setNetMode( int NetMode);
    bool isInUpdateMode();
    void handle(int netModeTimeoutMin, int NetMode);
    void printNetworkInfo();
    void onEspNowReady(std::function<void()> callback);
    void onWifiReady(std::function<void()> callback);
#ifdef ESP32
    void setupWebServer();
#endif
    void onUpdateMode(UpdateModeCallback cb);
private:
#ifdef ESP32
    AsyncWebServer server;
#endif
    const char *deviceName;
    const char *ssid;
    const char *wifiPwd;
    const char *otaPwd;
    IPAddress localIP;
    IPAddress gateway;
    IPAddress subnet;
    IPAddress dns;
    uint8_t currentChannel;
    int netMode;
    bool espNowReady = false;
    bool wifiReady = false;
    std::vector<String> deviceOtaList;
    uint32_t lastActivity;
    unsigned long startAttemptTime;
    const unsigned long netModeTimeout = 10*60*1000;
    std::function<void()> espNowReadyCallback;
    std::function<void()> wifiReadyCallback;
    void notifyEspNowReady();
    void notifyWifiReady();
    void getNetMode(int defaultNetMode);
    void startWiFi(int netMode = ESPNOW);
    void configureWiFiChannel();
    int32_t getWiFiChannel(const char *ssid);
    void setupOTA();
    void startOTA();
    void endOTA();
    void progressOTA(unsigned int progress, unsigned int total);
    void errorOTA(ota_error_t error);
#ifdef ESP32
    Preferences preferences;
#endif
};
