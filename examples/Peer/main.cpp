//****************************************************************
// THE ROUTER MUST BE AN ESP32 AND THE NEXT CLIENT AS WELL.
// FROM THE SECOND PEER ONWARD, COMMUNICATION CAN BE WITH ESP8266.
//****************************************************************

#include <Arduino.h>
#include "EspNowPeer.h"
#include "wifiConnManager.h"
#include "NtpManager.h"
#include "config.h"
#include "def_refaserv.h"

#define ESPNOW 0
#define WIFI 1
#define HYBRID 2
uint8_t channel;

// OBJECT INSTANCES

wifiConnManager wifiConnMng; // Network connection manager instance
EspNowPeer eNowPeer;         // ESP-NOW client instance
NtpManager ntp(-3);          // Timezone GMT-3 (Brasília)
bool ntpStarted = false;

unsigned long currentMillis = 0;
// Status message send interval to the router
unsigned long prevMillisSendStatus = 0;
const long intervalSendStatus = 1 * 0.5 * 60 * 1000; // 0.5 min

void setup()
{
  Serial.begin(115200);

  pinMode(outRefletorAreaServico, OUTPUT);
  digitalWrite(outRefletorAreaServico, LOW);

  wifiConnMng.onEspNowReady([]() {
    Serial.println("📢 Event: ESP-NOW is ready. Starting EspNowPeer...");
    eNowPeer.begin(channel, localName, Parents, childrenPeers);

    if (!wifiConnMng.isInUpdateMode()) {
      // NTP Synchronization
      eNowPeer.subscribe(Parents[0].name, "ALL", "NTPTIME", [](String message) {
        unsigned long epoch = message.toInt();
        ntp.setEpochTime(epoch);
        Serial.print("Time updated: ");
        ntp.printDateTime();
        Serial.println("⏭️ Forwarding message to the next peer in the chain...");
        // eNowPeer already handles forwarding for 'ALL'
      });

      // Forward child's status to router
      eNowPeer.subscribe(childrenPeers[0].name, localName, "STATUS", [](String message) {
        Serial.println("⏮️ Forwarding child status to the router");
        eNowPeer.publishENow(childrenPeers[0].name, // Source
                             Parents[0].name,        // Destination
                             "STATUS",               // Action
                             "Online");              // Message
        ntp.printDateTime();
      });

      // Control the reflector (outdoor light)
      eNowPeer.subscribe(Parents[0].name, localName, "BTN_ACTION_PEER1", [](String message) {
        if (message == "ON") {
          digitalWrite(outRefletorAreaServico, HIGH);
          Serial.print("Service area light turned ON at ");
          ntp.printDateTime();
        } else if (message == "OFF") {
          digitalWrite(outRefletorAreaServico, LOW);
          Serial.print("Service area light turned OFF at ");
          ntp.printDateTime();
        } else {
          Serial.print("Invalid command: ");
          Serial.println(message);
        }
        ntp.printDateTime();
      });

      // Network mode switching
      eNowPeer.subscribe(Parents[0].name, localName, "NET_MODE", [](String message) {
        if (message == "UPDATE") {
          Serial.println("Wi-Fi connection requested for OTA update");
          wifiConnMng.setNetMode(WIFI);
        } else if (message == "ESPNOW") {
          Serial.println("Switching back to ESP-NOW mode");
          wifiConnMng.setNetMode(ESPNOW);
        } else {
          Serial.print("Invalid command: ");
          Serial.println(message);
        }
      });

      for (const auto& peer : childrenPeers) {
        eNowPeer.subscribe(Parents[0].name, peer.name, "NET_MODE", [peer](const String &msg) {
          Serial.println("Callback triggered for " + String(localName) + ">" + peer.name + "/NET_MODE");
          Serial.print("Message content: ");
          Serial.println(msg);
          if (msg == "UPDATE" || msg == "ESPNOW") {
            Serial.println("Preparing to send via ESP-NOW...");
            eNowPeer.publishENow(localName, peer.name, "NET_MODE", msg);
            Serial.println("Command sent via ESP-NOW");
          } else {
            Serial.println("Invalid message for " + peer.name);
          }
        });
      }
    }
  });

  channel = wifiConnMng.begin(
      deviceName, WIFI_SSID, WIFI_PASSWORD, OTA_PASSWORD,
      CHANNEL_AUTO, wifiLocalIp, WIFI_GATEWAY, WIFI_SUBNET,
      WIFI_DNS, ESPNOW);

  if (wifiConnMng.isInUpdateMode()) {
    Serial.println("Update mode detected.");
    wifiConnMng.handle(6, ESPNOW);
    return;
  }
}

void loop()
{
  // Send status message to the router
  currentMillis = millis();
  if ((unsigned long)(currentMillis - prevMillisSendStatus) >= intervalSendStatus) {
    prevMillisSendStatus = millis();
    if (!wifiConnMng.isInUpdateMode()) {
      Serial.println("\n Sending status message to Parents at ");
      ntp.printDateTime();
      for (const auto& peer : Parents) {
        eNowPeer.publishENow(localName, // Source
                             peer.name,  // Destination
                             "STATUS",   // Action
                             "Online");  // Message
      }

      eNowPeer.handlePeerVerification(1);
    }
  }
  wifiConnMng.handle(6, ESPNOW);
}
