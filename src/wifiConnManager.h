#pragma once
#ifdef ESP32
#include <WiFi.h>
#include <esp_wifi.h>
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  #include <Arduino.h>
  #include "esp_now.h"
#else
  #error "This library only supports ESP32/ESP8266"
#endif
#include <Preferences.h>
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

class wifiConnManager
{
public:
    static wifiConnManager *instance; // Ponteiro estático para a instância

    uint8_t begin(const char *deviceName, const char *ssid, const char *wifiPwd, const char *otaPwd,
               uint8_t defaultChannel, IPAddress localIP, IPAddress gateway,
               IPAddress subnet, IPAddress dns, int netMode = ESPNOW);

    wifiConnManager();

    void setNetMode( int NetMode);
    bool isInUpdateMode();
    void handle(int netModeTimeoutMin, int NetMode);
    void printNetworkInfo();
    void onEspNowReady(std::function<void()> callback);
    void onWifiReady(std::function<void()> callback);

private:
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
