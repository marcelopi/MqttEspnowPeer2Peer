//****************************************************************
// THE ROUTER CAN ONLY BE AN ESP32 AND THE NEXT CLIENT MUST ALSO BE AN ESP32
// FROM THE SECOND PEER ONWARD, COMMUNICATION CAN OCCUR WITH ESP8266
//****************************************************************

#include <Arduino.h>
#include "MqttEspNowRouter.h"
#include "wifiConnManager.h"
#include "NtpHelper.h"
#include "config.h"
#include "def_router1.h"

#define ESPNOW 0
#define WIFI 1
#define HYBRID 2

uint8_t channel;

wifiConnManager wifiConnMng; // Connection manager instance
MqttEspNowRouter router;
NtpHelper ntp(-3, 1 * 1 * 60 * 1000, NTP_SERVER); // Timezone -3 and update every 3 minutes

bool ntpStarted = false;

unsigned long currentMillis = 0;
// NTP time broadcast interval for EspNow clients
unsigned long prevMillisSendNTP = 0;
unsigned long prevMillisUpdNTP = 0;
const long intervalSendNTP = 1 * 1 * 60 * 1000; // 1 min
const long intervalUpdNTP = 1 * 6 * 60 * 1000;  // 6 min

void setup()
{
  Serial.begin(115200);

  pinMode(outRefletorFundo, OUTPUT);
  digitalWrite(outRefletorFundo, LOW);

  wifiConnMng.onWifiReady([]() {
    Serial.println("📢 Event: Wi-Fi connected. Starting MqttEspNowRouter...");
    wifiConnMng.onEspNowReady([]() {
      ntp.begin();
      ntpStarted = true;
      Serial.println("✅ NTP initialized " + ntp.formatDateTimeNTP());
      router.begin(channel, channel, routerName, routerMac, mqttServerName, childrenPeers, MQTT_SERVER_IP, 1883, MQTT_USER, MQTT_PWD);
      Serial.println("📢 Event: ESP-NOW connected. Starting topic subscription...");

      // MQTT route from broker to master
      router.subscribe(mqttServerName, "PEER1", "BTN_ACTION_ROUTER", [](const String &message) {
        if (message == "ON") {
          digitalWrite(outRefletorFundo, HIGH);
          Serial.println("Turn ON at " + ntp.formatDateTimeNTP());
        } else if (message == "OFF") {
          digitalWrite(outRefletorFundo, LOW);
          Serial.println("Turn OFF at " + ntp.formatDateTimeNTP());
        } else {
          Serial.print("Invalid command: ");
          Serial.println(message);
        }
      }, ROUTE_MQTT);

      // MQTT route that will be sent via ESP-NOW to client00 MAC[0]
      router.subscribe(mqttServerName, childrenPeers[0].name, "BTN_ACTION_PEER1", [](const String &msg) {
        Serial.print("Message: ");
        Serial.println(msg);
        Serial.print("ESP-NOW: ");
        Serial.println(router.ready ? "Ready" : "Not Ready");

        if (msg == "ON" || msg == "OFF") {
          Serial.println("Preparing to send via ESP-NOW...");
          router.publishENow(routerName, childrenPeers[0].name, "BTN_ACTION_PEER1", msg);
          Serial.println("Command sent via ESP-NOW");
        } else {
          Serial.print("Invalid command: ");
          Serial.println(msg);
        }
      }, ROUTE_MQTT);

      // MQTT routes that will be sent via ESP-NOW to peers to enable Wi-Fi
      for (const auto &peer : chainPeers) {
        router.subscribe(mqttServerName, peer.name, "NET_MODE", [peer](const String &msg) {
          Serial.println("Callback triggered for " + String(routerName) + ">" + peer.name + "/NET_MODE");
          Serial.print("Message: ");
          Serial.println(msg);
          Serial.print("ESP-NOW: ");
          Serial.println(router.ready ? "Ready" : "Not Ready");

          if (msg == "UPDATE" || msg == "ESPNOW") {
            Serial.println("Preparing to send via ESP-NOW...");
            router.publishENow(routerName, peer.name, "NET_MODE", msg, childrenPeers[0].mac);
            Serial.println("Command sent via ESP-NOW");
          } else {
            Serial.print("Invalid command for " + peer.name);
          }
        }, ROUTE_MQTT);
      }

      // Message received via ESP-NOW that should be published to the broker
      for (const auto &peer : chainPeers) {
        router.subscribe(peer.name, routerName, "STATUS", [peer](const String &msg) {
          Serial.print("Received peer status at " + ntp.formatDateTimeNTP() + ": ");
          Serial.println(msg);
          router.publishMqtt(peer.name, routerName, "STATUS", msg);
        }, ROUTE_ESPNOW, childrenPeers[0].mac);
      }

    });

    wifiConnMng.printNetworkInfo();
  });

  std::vector<String> deviceOtaList;
  for (const auto &peer : chainPeers) {
    deviceOtaList.push_back(peer.name);
  }

  wifiConnMng.onUpdateMode([](const String &deviceName) {
    auto it = std::find_if(chainPeers.begin(), chainPeers.end(),
                            [&](const DeviceInfo &peer) { return peer.name == deviceName; });

    if (it == chainPeers.end()) {
      Serial.println("⚠️ Peer not found for UpdateMode.");
      return;
    }

    router.publishENow(routerName, it->name, "NET_MODE", "UPDATE", it->mac);
    Serial.println("🚀 UPDATE mode sent to " + it->name);
  });

  channel = wifiConnMng.begin(
      deviceName, WIFI_SSID, WIFI_PASSWORD, OTA_PASSWORD,
      CHANNEL_AUTO, wifiLocalIp, WIFI_GATEWAY, WIFI_SUBNET,
      WIFI_DNS, HYBRID, deviceOtaList);
}

void loop()
{
  currentMillis = millis();

  // Periodically update NTP
  if (ntpStarted && (currentMillis - prevMillisUpdNTP >= intervalUpdNTP)) {
    prevMillisUpdNTP = currentMillis;
    ntp.updateNTP();
    Serial.println("NTP updated at " + ntp.formatDateTimeNTP());
  }

  // Send NTP time to clients
  if (ntpStarted && (currentMillis - prevMillisSendNTP >= intervalSendNTP)) {
    prevMillisSendNTP = currentMillis;
    router.publishENow(routerName, "ALL", "NTPTIME", ntp.formatDateTimeNtpEpoch());
    Serial.println("📨 NTP time sent to peers at " + ntp.formatDateTimeNTP());
  }

  // Maintain OTA, peer verification, and reconnection
  if (router.ready) {
    wifiConnMng.handle(6, HYBRID);
    router.handlePeerVerification(1);
    router.handleReconnectMqtt(2);
  }
}
