//****************************************************************
// O ROTEADOR SÓ PODE SER ESP-32 E O PRÓXIMO CLIENTE IDEM
// A PARTIR DO SEGUNDO PEER A COMUNICAÇÃO PODE OCORRER COM ESP8266
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

wifiConnManager wifiConnMng; // Instância do cliente Controlador de conexões
MqttEspNowRouter router;
NtpHelper ntp(-3, 1 * 1 * 60 * 1000, NTP_SERVER); // fuso horário -3 e atualização a cada 3 minutos


bool ntpStarted = false;

unsigned long currentMillis = 0;
// Intervalo de envio de hora no NTP para os clientes Esp Now
unsigned long prevMillisSendNTP = 0;
unsigned long prevMillisUpdNTP = 0;
const long intervalSendNTP = 1 * 1 * 60 * 1000; // 1 min
const long intervalUpdNTP = 1 * 6 * 60 * 1000; // 6 min
void setup()
{
  Serial.begin(115200);

  pinMode(outRefletorFundo, OUTPUT);
  digitalWrite(outRefletorFundo, LOW);

  wifiConnMng.onWifiReady([]() {
    Serial.println("📢 Evento: Wi-Fi está conectado. Iniciando MqttEspNowRouter...");
    ntp.begin();
    ntpStarted = true;
    Serial.println("✅ NTP inializado em " + ntp.formatDateTimeNTP());
    
    wifiConnMng.onEspNowReady([]() {
        router.begin(channel, channel, routerName, routerMac, mqttServerName, childrenPeers, MQTT_SERVER_IP, 1883, MQTT_USER, MQTT_PWD);
        Serial.println("📢 Evento: ESP-NOW está conectado. Iniciando subscrição dos tópicos...");
        // Rota MQTT do broker para o master
        router.subscribe(mqttServerName, "routerName", "BTN_REF_FUNDO", [](const String &message)
                        {
          if (message == "ON") {
            digitalWrite(outRefletorFundo, HIGH);
            Serial.println("Refletor do fundo ligado em "+ ntp.formatDateTimeNTP());
          } else if (message == "OFF") {
            digitalWrite(outRefletorFundo, LOW);
            Serial.println("Refletor do fundo desligado em "+ ntp.formatDateTimeNTP());
          } else {
            Serial.print("Comando inválido: ");
            Serial.println(message);
          } }, ROUTE_MQTT);

        // Rota MQTT que será enviada via ESPNOW para o client00 MAC[0]
        router.subscribe(mqttServerName, childrenPeers[0].name, "BTN_REF_A_SERV", [](const String &msg)
                        {
            Serial.println("Callback acionado para ROBOTECTRL>REFASERV/BTN_REF_A_SERV");
            Serial.print("Conteúdo da mensagem: ");
            Serial.println(msg);
            Serial.print("Estado do ESP-NOW: ");
            Serial.println(router.ready ? "Pronto" : "Não pronto");
            
            if(msg == "ON" || msg == "OFF") {
              Serial.println("Preparando para enviar via ESP-NOW...");
              router.publishENow(routerName,childrenPeers[0].name,"BTN_REF_A_SERV", msg);
              Serial.println("Comando enviado via ESP-NOW");
            } else {
              Serial.println("Mensagem inválida para Peer");
        } }, ROUTE_MQTT);

        // Rota MQTT que será enviada via ESPNOW para o client00 MAC[0] para habilitar wifi
        router.subscribe(mqttServerName, childrenPeers[0].name, "NET_MODE", [](const String &msg)
                        {
            Serial.println("Callback acionado.");
            Serial.print("Conteúdo da mensagem: ");
            Serial.println(msg);
            Serial.print("Estado do ESP-NOW: ");
            Serial.println(router.ready ? "Pronto" : "Não pronto");
            
            if(msg == "UPDATE" || msg == "ESPNOW") {
              Serial.println("Preparando para enviar via ESP-NOW...");
              router.publishENow(routerName,childrenPeers[0].name,"NET_MODE", msg);
              Serial.println("Comando enviado via ESP-NOW");
            } else {
              Serial.println("Mensagem inválida para Peer");
        } }, ROUTE_MQTT);

        // Mensagem recebida via ESPNOW que deve ser publicada no broker
        router.subscribe(childrenPeers[0].name, routerName, "STATUS", [](const String &msg)
                        {
                          Serial.print("Status do cliente1 recebido em " + ntp.formatDateTimeNTP() + ": ");
                          Serial.println(msg);
                          router.publishMqtt(childrenPeers[0].name, routerName, "STATUS", msg); // <-- publicar no broker
                        },
                        ROUTE_ESPNOW);

      });

    wifiConnMng.printNetworkInfo();
  });
  channel = wifiConnMng.begin(
      deviceName, WIFI_SSID, WIFI_PASSWORD, OTA_PASSWORD,
      CHANNEL_AUTO, wifiLocalIp, WIFI_GATEWAY, WIFI_SUBNET,
      WIFI_DNS, HYBRID);
}

void loop()
{
  currentMillis = millis();
  // Atualiza NTP periodicamente
  if (ntpStarted && (currentMillis - prevMillisUpdNTP >= intervalUpdNTP))
  {
    prevMillisUpdNTP = currentMillis;
    ntp.updateNTP();
    Serial.println("Atualizado NTP em " + ntp.formatDateTimeNTP());
  }
  // Envia NTP para os clientes
  if (ntpStarted && (currentMillis - prevMillisSendNTP >= intervalSendNTP))
  {
    prevMillisSendNTP = currentMillis;
    router.publishENow(routerName, "ALL", "NTPTIME", ntp.formatDateTimeNtpEpoch());
    Serial.println("📨 Enviando NTP para clientes em " + ntp.formatDateTimeNTP());
  }
  // Mantém OTA, peers e reconexão
  if (router.ready)
  {
    wifiConnMng.handle(6,HYBRID);
    router.handlePeerVerification(1);
    router.handleReconnectMqtt(2);
  }
}

