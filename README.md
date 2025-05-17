# MqttEspnowPeer2Peer  
**Hybrid MQTT/ESP-NOW Protocol for Distributed IoT**  
# Inspiration and Core Concept

This system was designed to unify MQTT and ESP-NOW paradigms, creating a transparent communication layer where:
ESP-NOW acts as "Wireless MQTT": Messages are routed using topic patterns (source/destination/action), simulating MQTT's publish/subscribe model without requiring a central broker.
Full Abstraction: Developers interact with a single API while the library automatically chooses between:
ESP-NOW: For local peer-to-peer communication (ESP32/ESP8266)
MQTT: For cloud/remote connectivity (ESP32 Router only)

[![GitHub Sponsors](https://img.shields.io/badge/Sponsor-30363D?style=for-the-badge&logo=GitHub-Sponsors&logoColor=white)](https://github.com/sponsors/marcelopi)

[![PDF Documentation](https://img.shields.io/badge/Download-Full_Documentation-blue)](docs/EspnowMqttPeer2Peer.pdf)


## 🌟 Core Features

| Feature                  | ESP32 Router | ESP32 Peer | ESP8266 Peer |
|--------------------------|:------------:|:----------:|:------------:|
| MQTT Broker Support      | ✅           | ❌         | ❌           |
| ESP-NOW Routing          | ✅           | ✅         | ✅           |
| Hybrid Communication     | ✅           | ✅         | Limited      |
| OTA Updates              | ✅           | ✅         | ✅           |

---

## 📦 Repository Structure
project-root/
├── src/ # Core library code
├── examples/ # Usage samples
├── docs/ # Documentation
│ └── ESPNowMqttPeer2Peer.pdf
└── library.json # Package metadata
## 📨 Message pattern format
[SOURCE]>[DESTINATION]/[ACTION]|[PAYLOAD]

## 🚀 Quick Start

### Router (ESP32 Only)
```cpp
#include <MqttEspNowRouter.h>
#include "config.h"

wifiConnManager wifi;
MqttEspNowRouter router;
std::vector<DeviceInfo> children = {{"CLIENT1", {0xAA,0xBB,0xCC,0xDD,0xEE,0xFF}}};

void setup() {
  // 1. Configure WiFi Connection Manager
  wifi.onWifiReady([]() { // WiFi Connected Callback
    Serial.println("WiFi ready! Initializing NTP...");
    
    // 2. Configure ESP-NOW after WiFi
    wifi.onEspNowReady([]() { // ESP-NOW Ready Callback
      Serial.println("ESP-NOW ready! Starting router...");
      
      // 3. Initialize router with dependencies
      router.begin(
        channel, channel, 
        routerName, routerMac,
        mqttServerName, children,
        MQTT_SERVER, 1883
      );

      // 4. Setup MQTT routes AFTER initialization
      router.subscribe(mqttServerName, "REFFUNDO", "BTN_REF_FUNDO", 
        [](String msg) {
          digitalWrite(RELAY_PIN, msg == "ON" ? HIGH : LOW);
        }, ROUTE_MQTT
      );
    });
  });

  // 5. Start connection process
  wifi.begin(deviceName, WIFI_SSID, WIFI_PASS, OTA_PASS, 
            CHANNEL_AUTO, localIP, gateway, subnet, dns, HYBRID);
}

*********************************************************************************************

void loop() {
  router.handlePeerVerification(5);
  router.handleReconnectMqtt(10);
}

#include <EspNowPeer.h>
#include "config.h"

wifiConnManager wifi;
EspNowPeer peer;
std::vector<DeviceInfo> routers = {{"ROUTER1", {0x00,0x11,0x22,0x33,0x44,0x55}}};

void setup() {
  // 1. Configure ESP-NOW first
  wifi.onEspNowReady([]() { // ESP-NOW Ready Callback
    Serial.println("ESP-NOW initialized!");
    
    // 2. Start peer with router list
    peer.begin(
      wifi.getChannel(), 
      "SENSOR01", 
      routers, 
      {}
    );

    // 3. Setup message handlers
    peer.subscribe("ROUTER1", "SENSOR01", "LED_CTRL", 
      [](String msg) {
        digitalWrite(LED_PIN, msg.toInt());
      }
    );
  });

  // 4. Start network connection
  wifi.begin(deviceName, WIFI_SSID, WIFI_PASS, OTA_PASS,
            CHANNEL_AUTO, localIP, gateway, subnet, dns, ESPNOW);
}

void loop() {
  peer.handlePeerVerification(3);
}

