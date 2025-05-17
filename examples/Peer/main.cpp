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
    eNowPeer.begin(channel, localName, Routers, childrenPeers);
      if (!wifiConnMng.isInUpdateMode()){
        // Sincronização NTP
        eNowPeer.subscribe(Routers[0].name, "ALL", "NTPTIME", [](String message) {
          unsigned long epoch = message.toInt();
          NtpManager::setEpochTime(epoch);
          Serial.print("Tempo atualizado: ");
          NtpManager::printDateTime();
        });

        // Controle do refletor
        eNowPeer.subscribe(Routers[0].name, localName, "BTN_REF_A_SERV", [](String message) {
          if (message == "ON") {
            digitalWrite(outRefletorAreaServico, HIGH);
            Serial.print("Refletor da área de serviço ligado em ");
          } else if (message == "OFF") {
            digitalWrite(outRefletorAreaServico, LOW);
            Serial.print("Refletor da área de serviço desligado em ");
          } else {
            Serial.print("Comando inválido: ");
            Serial.println(message);
          }
          NtpManager::printDateTime();
        });

        // Alternância de modo de rede
        eNowPeer.subscribe(Routers[0].name, localName, "NET_MODE", [](String message) {
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
      Serial.println("\n Enviando mensagem de status em ");
      NtpManager::printDateTime();
      eNowPeer.publishENow("REFASERV", // Origem
                           "ROUTER01", // Destino
                           "STATUS",   // Ação
                           "Online");  // Mensagem
      eNowPeer.handlePeerVerification(1);
    }
  }
  wifiConnMng.handle(6,ESPNOW);
}
