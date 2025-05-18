#pragma once
#include <vector>

#ifdef ESP32
#include <WiFi.h>
#pragma once

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  #include <Arduino.h>
  #include "esp_now.h"
#else
  #error "This library only supports ESP32/ESP8266"
#endif

#include <esp_wifi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
extern "C"
{
    #include <user_interface.h> // para wifi_set_channel()
    #include <espnow.h>
}
#endif

#define MAX_ROUTES 10
#define MAX_TOPIC_LEN 64

typedef std::function<void(String)> LocalHandler;

struct Route
{
    String source;
    String destination;
    String action;
    uint8_t mac[6];
    LocalHandler handler;
};

struct DeviceInfo {
    String name;
    uint8_t mac[6];
    bool online = false;
    unsigned long lastPingSent = 0;
    unsigned long lastPongReceived = 0;

    DeviceInfo(const String& n, std::initializer_list<uint8_t> m)
        : name(n) {
        std::copy(m.begin(), m.end(), mac);
    }
};

class EspNowPeer {
public:
    EspNowPeer();
    using MessageHandler = void(*)(const String& source, 
                                 const String& destination,
                                 const String& action,
                                 const String& message);
    
    void begin(uint8_t espnowChannel, const char *localName, const std::vector<DeviceInfo>& routers, const std::vector<DeviceInfo>& childrenPeers);
    void publishENow(const String& source,
                    const String& destination, 
                    const String& action,
                    const String& message);
    void handlePeerVerification(int timeoutMin);
    void subscribe(const String &source, const String &destination, const String &action, LocalHandler handler);
private:
    uint32_t _lastActivity;
    const uint8_t *routerMac = 0;
    const char *localName= nullptr;
    std::vector<DeviceInfo> childrenPeers; 
    std::vector<DeviceInfo> routers{}; 
    const char *wfSsid= nullptr;
    uint8_t espnowChannel = -1;
    static EspNowPeer* instance;
    int32_t getWiFiChannel(const char *ssid);
    void addPeer(const uint8_t *peerMac);
    static void onReceive(const uint8_t* mac, const uint8_t* data, int len);
    const uint8_t* getPeerMacByName(const String& name) const;
    Route routes[MAX_ROUTES];
    int routeCount = 0;
};