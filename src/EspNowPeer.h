#pragma once
#include <vector>
#ifdef ESP32
#include <WiFi.h>
#include <Arduino.h>
#include "esp_now.h"
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
    const char *localName = nullptr;
    std::vector<DeviceInfo> childrenPeers; 
    std::vector<DeviceInfo> routers{}; 
    const char *wfSsid = nullptr;
    uint8_t espnowChannel = -1;
    static EspNowPeer* instance;
    
    int32_t getWiFiChannel(const char *ssid) {
#ifdef ESP8266
        struct bss_info *bssScan;
        if (wifi_station_get_ap_info(&bssScan)) {
            return bssScan->channel;
        }
        return -1;
#else
        wifi_ap_record_t apInfo;
        if (esp_wifi_sta_get_ap_info(&apInfo) == ESP_OK) {
            return apInfo.primary;
        }
        return -1;
#endif
    }

    void addPeer(const uint8_t *peerMac) {
#ifdef ESP8266
        bool alreadyExists = false; // esp_now_is_peer_exist() não está disponível
#else
        bool alreadyExists = esp_now_is_peer_exist(peerMac);
#endif

        if (!alreadyExists) {
#ifdef ESP8266
            if (esp_now_add_peer(peerMac, ESP_NOW_ROLE_COMBO, espnowChannel, NULL, 0) != 0) {
                Serial.println("❌ Falha ao adicionar peer (ESP8266)");
            } else {
                Serial.println("✅ Peer adicionado (ESP8266)");
            }
#else
            esp_now_peer_info_t peer = {};
            memcpy(peer.peer_addr, peerMac, 6);
            peer.channel = espnowChannel;
            peer.encrypt = false;

            if (esp_now_add_peer(&peer) != ESP_OK) {
                Serial.println("❌ Falha ao adicionar peer (ESP32)");
            } else {
                Serial.println("✅ Peer adicionado (ESP32)");
            }
#endif
        }
    }

    static void onReceive(const uint8_t* mac, const uint8_t* data, int len);
    const uint8_t* getPeerMacByName(const String& name) const;
    Route routes[MAX_ROUTES];
    int routeCount = 0;
};
