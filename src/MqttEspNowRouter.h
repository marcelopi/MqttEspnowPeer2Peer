#ifndef MQTT_ESPNOW_ROUTER_H
#define MQTT_ESPNOW_ROUTER_H
#pragma once
#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include "esp_now.h"
#include <esp_wifi.h> 
#else
#include <ESP8266WiFi.h>
#include <espnow.h> 
#endif
#include <AsyncMqttClient.h>
#include <vector>

#define MAX_ROUTES 10
#define MAX_TOPIC_LEN 64

typedef std::function<void(String)> LocalHandler;

enum RouteType
{
    ROUTE_MQTT,
    ROUTE_ESPNOW,
    ROUTE_BOTH
};

struct Route
{
    String source;
    String destination;
    String action;
    uint8_t mac[6];
    LocalHandler handler;
    RouteType type;
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


class MqttEspNowRouter
{
public:
    MqttEspNowRouter();
    bool ready = false;
    void begin(uint8_t wifiChannel, uint8_t espnowChannel, const char *routerName, const uint8_t *routerMac, const char *mqttServerName, const std::vector<DeviceInfo>& childrenPeers,
                            const char *mqttServer, uint16_t mqttPort, const char *mqttUser, const char *mqttPwd);
    
    void subscribe(const String &source, const String &destination, const String &action, LocalHandler handler, RouteType type);
    void subscribeMqttTopic(const String &topic, uint8_t qos = 1);
    void publishMqtt(const String &source, const String &destination, const String &action, const String &message);
    void publishENow(const String &source, const String &destination, const String &action, const String &message);
    void handleReconnectMqtt(int timeoutMin);
    void handlePeerVerification(int timeoutMin);
    void onWiFiConnected();
private:
    uint32_t lastActivity;
    AsyncMqttClient mqttClient;
    static MqttEspNowRouter *instance;
    unsigned long lastReconnectAttempt = 0;
    uint8_t wifiChannel = -1;
    uint8_t espnowChannel = -1;
    std::vector<DeviceInfo> childrenPeers; 
    const char *mqttServerName= nullptr;
    const char *routerName= nullptr;
    const uint8_t *routerMac = nullptr;
    const char *wfDeviceName = nullptr;
    const char *otaPwd = nullptr;
    const char *mqttServer = nullptr;
    uint16_t mqttPort = 1883;
    const char *mqttUser = nullptr;
    const char *mqttPwd = nullptr;
    unsigned int handleMqttTimeoutMin = 2;
    static void mqttCallback(char *topic, char *payload, AsyncMqttClientMessageProperties props, size_t len, size_t, size_t);
    static void espNowReceiveStatic(const uint8_t *mac, const uint8_t *data, int len);
    void configureMQTT();
    void configureESPNOW();
    void addPeer(const uint8_t *peerMac);
    void handleMqttMessage(char *topic, char *payload, unsigned int length);
    void handleEspNowMessage(const uint8_t *mac, const uint8_t *data, int len);
    const uint8_t* getPeerMacByName(const String& name) const;
    bool isLocalMac(const uint8_t *mac);
    void processPendingRoutes();
    struct PendingRoute
    {
        String source;
        String destination;
        String action;
        RouteType type; // ROUTE_MQTT, ROUTE_ESPNOW, ROUTE_BOTH
        uint8_t mac[6]; // Se não aplicável (MQTT puro), preencher com 0
    };
    std::vector<PendingRoute> pendingRoutes;
    Route routes[MAX_ROUTES];
    int routeCount = 0;
};

#endif