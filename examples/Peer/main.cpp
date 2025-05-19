//****************************************************************
// O ROTEADOR SÓ PODE SER ESP-32 E O PRÓXIMO CLIENTE IDEM
// A PARTIR DO SEGUNDO PEER A COMUNICAÇÃO PODE OCORRER COM ESP8266
//****************************************************************

#include <Arduino.h>
#include "EspNowPeer.h"
#include "wifiConnManager.h"
#include "NtpManager.h"
#include "config.h"
#include "def_peer1.h"

#define ESPNOW 0
#define WIFI 1
#define HYBRID 2
uint8_t channel; 
// INSTANCIANDO OS OBJETOS

wifiConnManager wifiConnMng; // Instância do cliente Controlador de conexões
EspNowPeer eNowPeer;         // Instância do cliente ESP-NOW
NtpManager ntp;
bool ntpStarted = false;

unsigned long currentMillis = 0;
// Intervalo de envio de status para o router
unsigned long prevMillisSendStatus = 0;
const long intervalSendStatus = 1 * 0.5 * 60 * 1000; // 0.5 min

void setup()
{
  Serial.begin(115200);

  pinMode(outRefletorAreaServico, OUTPUT);
  digitalWrite(outRefletorAreaServico, LOW);

  wifiConnMng.onEspNowReady([]() {
    Serial.println("📢 Evento: ESP-NOW está pronto. Iniciando EspNowPeer...");
    eNowPeer.begin(channel, localName, Parents, childrenPeers);
      if (!wifiConnMng.isInUpdateMode()){
        // Sincronização NTP
        eNowPeer.subscribe(Parents[0].name, "ALL", "NTPTIME", [](String message) {
          unsigned long epoch = message.toInt();
          ntp.setEpochTime(epoch);
          Serial.print("Tempo atualizado: ");
          ntp.printDateTime();
          Serial.println("⏭️ Repassando mensagem para o próximo da cadeia...");
          eNowPeer.publishENow(localName, // Origem
                              "ALL", // Destino
                              "NTPTIME",   // Ação
                              message);  // Mensagem
        });

        // Repassa status do filho para router
        eNowPeer.subscribe(childrenPeers[0].name, localName, "STATUS", [](String message) {
          Serial.println("⏮️ Repassando status do filho para o router");
          eNowPeer.publishENow(childrenPeers[0].name, // Origem
                              Parents[0].name, // Destino
                              "STATUS",   // Ação
                              "Online");  // Mensagem
          ntp.printDateTime();
        });

        // Controle do refletor
        eNowPeer.subscribe(Parents[0].name, localName, "ACTION_BUTTON_PEER1", [](String message) {
          if (message == "ON") {
            digitalWrite(outRefletorAreaServico, HIGH);
            Serial.print("Led ligado em ");
            ntp.printDateTime();
          } else if (message == "OFF") {
            digitalWrite(outRefletorAreaServico, LOW);
            Serial.print("led desligado em ");
            ntp.printDateTime();
          } else {
            Serial.print("Comando inválido: ");
            Serial.println(message);
          }
          ntp.printDateTime();
        });

        // Alternância de modo de rede
        eNowPeer.subscribe(Parents[0].name, localName, "NET_MODE", [](String message) {
          if (message == "UPDATE") {
            Serial.println("Solicitado conexão Wi-Fi para atualização OTA");
            wifiConnMng.setNetMode(WIFI);
          } else if (message == "ESPNOW") {
            Serial.println("Solicitado retorno ao modo ESP-NOW");
            wifiConnMng.setNetMode(ESPNOW);
          } else {
            Serial.print("Comando inválido: ");
            Serial.println(message);
          }
        });
      }
  });

  channel = wifiConnMng.begin(
      deviceName, WIFI_SSID, WIFI_PASSWORD, OTA_PASSWORD,
      CHANNEL_AUTO, wifiLocalIp, WIFI_GATEWAY, WIFI_SUBNET,
      WIFI_DNS, ESPNOW);

  if (wifiConnMng.isInUpdateMode())
  {
    Serial.println("Modo de atualização detectado.");
    wifiConnMng.handle(6, ESPNOW);
    return;
  }
}


void loop()
{
  // Enviar mensagem de status para o router
  currentMillis = millis();
  if ((unsigned long)(currentMillis - prevMillisSendStatus) >= intervalSendStatus)
  {
    prevMillisSendStatus = millis();
    if (!wifiConnMng.isInUpdateMode())
    {
      Serial.println("\n Enviando mensagem de status para Pais em ");
      ntp.printDateTime();
        for (const auto& peer : Parents) {
          eNowPeer.publishENow(localName, // Origem
                               peer.name, // Destino
                              "STATUS",   // Ação
                              "Online");  // Mensagem
        }

      eNowPeer.handlePeerVerification(1);
    }
  }
  wifiConnMng.handle(6,ESPNOW);
}
