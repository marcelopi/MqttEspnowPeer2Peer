#ifdef ESP32
#include "MqttEspNowRouter.h"
#define CHANNEL_AUTO 255
MqttEspNowRouter *MqttEspNowRouter::instance = nullptr;

MqttEspNowRouter::MqttEspNowRouter()
{
  instance = this;
}
void MqttEspNowRouter::begin(uint8_t wifiChannel, uint8_t espnowChannel, const char *routerName, const uint8_t *routerMac, const char *mqttServerName,const std::vector<DeviceInfo>& childrenPeers,
                            const char *mqttServer, uint16_t mqttPort, const char *mqttUser, const char *mqttPwd)
{
    // 1. Armazenar parâmetros de configuração
    this->wifiChannel = wifiChannel;
    this->childrenPeers = childrenPeers;
    this->wfDeviceName = wfDeviceName;
    this->otaPwd = otaPwd;
    this->mqttServer = mqttServer;
    this->mqttPort = mqttPort;
    this->mqttUser = mqttUser;
    this->mqttPwd = mqttPwd;
    this->espnowChannel = espnowChannel;
    this->mqttServerName = mqttServerName;
    this->routerName = routerName;
    this->routerMac = routerMac;

    Serial.println("\n✅ Configuração do MQTT após WiFi conectado..");
    configureESPNOW();
    configureMQTT();
    this->ready = true;
    Serial.println("✅ Sistema totalmente inicializado!");


}

void MqttEspNowRouter::addPeer(const uint8_t *peerMac){
    if (!esp_now_is_peer_exist(peerMac)) {
        esp_now_peer_info_t peer = {};
        memcpy(peer.peer_addr, peerMac, 6);
        peer.channel = espnowChannel;
        peer.encrypt = false;

        esp_err_t result = esp_now_add_peer(&peer);
        if (result != ESP_OK) {
            Serial.print("Falha ao adicionar peer em addPeer: ");
            Serial.println(result);
            return; // Evita reinício desnecessário
        } else {
            Serial.println("✅ Peer adicionado com sucesso.");
        }
    } else {
        Serial.println("Peer já existente.");
    }
}

void MqttEspNowRouter::configureESPNOW() {
    esp_now_register_send_cb([](const uint8_t *mac, esp_now_send_status_t status) {
        if (status != ESP_NOW_SEND_SUCCESS) {
            Serial.println("Erro no envio ESP-NOW");
        }
    });
    esp_now_register_recv_cb(espNowReceiveStatic);
};

void MqttEspNowRouter::configureMQTT() {
    Serial.println("Configurando cliente MQTT...");

    mqttClient.onConnect([this](bool sessionPresent) {
        Serial.println("\n✅ Conectado ao broker MQTT!");
        if (this->ready) {
            this->processPendingRoutes();
        }
    });

    mqttClient.onDisconnect([](AsyncMqttClientDisconnectReason reason) {
        Serial.print("Desconectado do MQTT. Motivo: ");
        Serial.println(static_cast<int>(reason));
    });

    mqttClient.onMessage(mqttCallback);
    mqttClient.setServer(mqttServer, mqttPort);

    if (mqttUser && mqttPwd) {
        mqttClient.setCredentials(mqttUser, mqttPwd);
    }

    mqttClient.connect();
}

void MqttEspNowRouter::processPendingRoutes()
{
  if (!ready || !mqttClient.connected())
  {
    Serial.println("processPendingRoutes: Aguardando redes prontas (ESP-NOW ou MQTT).");
    return;
  }

  Serial.println("Processando rotas pendentes...");

  for (const PendingRoute &pending : pendingRoutes)
  {
    subscribe(pending.source, pending.destination, pending.action, nullptr, pending.type);
  }

  pendingRoutes.clear();
}

void MqttEspNowRouter::subscribeMqttTopic(const String &topic, uint8_t qos)
{
  if (!mqttClient.connected())
  {
    Serial.println("MQTT não conectado. Assinatura adiada.");
    return;
  }

  mqttClient.subscribe(topic.c_str(), qos);
  Serial.print("Inscrito no tópico: ");
  Serial.println(topic);
}

void MqttEspNowRouter::publishMqtt(const String &source, const String &destination, const String &action, const String &message)
{
  if (mqttClient.connected())
  {
    String topic = source + ">" + destination + "/" + action;
    mqttClient.publish(topic.c_str(), 0, false, message.c_str());
    Serial.print("✅ Publicada mensagem no MQTT: ");
    Serial.print(topic);
    Serial.print(" => ");
    Serial.println(message);
  }
  else
  {
    Serial.println("MQTT não conectado. Não foi possível publicar.");
  }
}

void MqttEspNowRouter::publishENow(const String &source,
                                   const String &destination, const String &action,
                                   const String &message)
{
  if (!ready)
  {
    Serial.println("ESP-NOW não está pronto");
    return;
  }
  String fullMsg = source + ">" + destination + "/" + action + "|" + message;


    if (destination == "ALL"){
        // Verificação dos peers filhos
        for (const auto& peer : childrenPeers) {
            Serial.print(" Encaminhado mensagem para '");
            Serial.println(peer.name);
            esp_err_t result = esp_now_send(peer.mac, (uint8_t *)fullMsg.c_str(), fullMsg.length());
            Serial.println("📨 Enviada mensagem para peer:");
            Serial.println(fullMsg);
            if (result != ESP_OK) {
                Serial.print("Falha no envio ESP-NOW, código: ");
                Serial.println(result);
            }
        }
    } else {
        const uint8_t* peerMac = getPeerMacByName(destination);
        if (peerMac == nullptr) {
            Serial.print("⚠️ Dispositivo com nome '");
            Serial.print(destination);
            Serial.println("' não encontrado.");
            return;
        }

        esp_err_t result = esp_now_send(peerMac, (uint8_t *)fullMsg.c_str(), fullMsg.length());
        Serial.println("✅ Publicada mensagem ESP-NOW para peer:");
        Serial.println(fullMsg);
        if (result != ESP_OK)
        {
          Serial.print("⚠️ Falha no envio ESP-NOW, código: ");
          Serial.println(result);
        }
    }
}
void MqttEspNowRouter::mqttCallback(char *topic, char *payload, AsyncMqttClientMessageProperties props, size_t len, size_t, size_t)
{
  String topicStr(topic);
  String msg = String(payload).substring(0, len);
  Serial.print("Recebido tópico: ");
  Serial.println(topicStr);
  Serial.print("msg: ");
  Serial.println(msg);
  if (instance)
    instance->handleMqttMessage(topic, payload, len);
}

void MqttEspNowRouter::espNowReceiveStatic(const uint8_t *mac, const uint8_t *data, int len)
{
  Serial.println("📩Recebendo dados ESP-NOW...");
  if (instance)
    instance->handleEspNowMessage(mac, data, len);
}
void MqttEspNowRouter::handleEspNowMessage(const uint8_t *mac, const uint8_t *data, int len)
{
  Serial.println("✅ Recebendo mensagem de peer ESP-NOW...");
  String received;
  for (int i = 0; i < len; i++)
    received += (char)data[i];

  int sepIndex = received.indexOf('|');
  if (sepIndex == -1)
  {
    Serial.println("Mensagem ESP-NOW mal formatada (sem '|')");
    return;
  }

  int sep1 = received.indexOf('>');
  int sep2 = received.indexOf('/', sep1 + 1);
  int sep3 = received.indexOf('|', sep2 + 1);
  String source = "";
  String destination = "";
  String action = "";
  String message = "";
  if (sep1 != -1 && sep2 != -1 && sep3 != -1)
  {
    source = received.substring(0, sep1);
    destination = received.substring(sep1 + 1, sep2);
    action = received.substring(sep2 + 1, sep3);
    message = received.substring(sep3 + 1);
  }
  String topic = source + ">" + destination + "/" + action;

  Serial.print("Mensagem recebida via ESP-NOW:\n  Tópico: ");
  Serial.println(topic);
  Serial.print("  Mensagem: ");
  Serial.println(message);

  if (routeCount == 0)
  {
    Serial.println("Nenhuma rota configurada. Ignorando mensagem.");
    return;
  }

  // Loop para encontrar a rota correspondente
  for (int i = 0; i < routeCount; i++)
  {
  
    if ((routes[i].type == ROUTE_ESPNOW || routes[i].type == ROUTE_BOTH) &&
        routes[i].source == source &&
        routes[i].destination == destination &&
        routes[i].action == action &&
        !isLocalMac(routes[i].mac)) 
    {
      Serial.println("Invoca o handler para o tópico correspondente");
      routes[i].handler(message);
      break;
    }
  }
}

void MqttEspNowRouter::subscribe(const String &source, const String &destination, const String &action, LocalHandler handler, RouteType type)
{
  
  if (routeCount >= MAX_ROUTES)
    return;
  String topic = source + ">" + destination + "/" + action;
  const uint8_t* peerMac = getPeerMacByName(source);
  if (peerMac == nullptr) {
      Serial.print("⚠️ Dispositivo com nome '");
      Serial.print(source);
      Serial.println("' não encontrado.");
      return;
  }
  routes[routeCount].source = source;
  routes[routeCount].destination = destination;
  routes[routeCount].action = action;
  memcpy(routes[routeCount].mac, peerMac, 6);
  routes[routeCount].handler = handler;
  routes[routeCount].type = type;

  bool mqttOk = (type == ROUTE_ESPNOW) || mqttClient.connected();
  bool espNowOk = (type == ROUTE_MQTT) || ready;

  if (!mqttOk || (!espNowOk && (type == ROUTE_ESPNOW || type == ROUTE_BOTH) && !isLocalMac(peerMac)))
  {
    Serial.print("⏳ Tópico: ");
    Serial.print(topic);
    Serial.println(" adicionada à fila pendente.");

    PendingRoute pending;
    pending.source = source;
    pending.destination = destination;
    pending.action = action;
    pending.type = type;
    memcpy(pending.mac, peerMac, 6);
    pendingRoutes.push_back(pending);

    routeCount++;
    return;
  }

  // MQTT Subscription
  if ((type == ROUTE_MQTT || type == ROUTE_BOTH) && mqttClient.connected())
  {
    Serial.println("✅ Subscrevendo os tópicos MQTT...");
    subscribeMqttTopic(topic);
  }

  // Adição de peer ESP-NOW
  if ((type == ROUTE_ESPNOW || type == ROUTE_BOTH) && !isLocalMac(peerMac))
  {
      Serial.println("✅ Subscrevendo os tópicos ESP-NOW...");
      addPeer(peerMac);
  }

  routeCount++;
}


void MqttEspNowRouter::handleMqttMessage(char *topic, char *payload, unsigned int length)
{
  String topicStr(topic);
  String msg = String(payload).substring(0, length);
  Serial.print("✅ Recebendo mensagem MQTT Tópico recebido: '");
  Serial.print(topicStr);
  Serial.println("'");

  for (int i = 0; i < routeCount; i++)
  {
    String topic = String(routes[i].source) + ">" + String(routes[i].destination) + "/" + String(routes[i].action);
    Serial.print("Verificando rota #");
    Serial.print(i);
    Serial.print(": '");
    Serial.print(topic);
    Serial.print("' vs '");
    Serial.print(topicStr);
    Serial.println("'");

    if (topic == topicStr &&
        (routes[i].type == ROUTE_MQTT || routes[i].type == ROUTE_BOTH))
    {
      Serial.println("Rota correspondente encontrada!");
      routes[i].handler(msg); // Chama o callback
      break;
    }
  }
}

bool MqttEspNowRouter::isLocalMac(const uint8_t *mac)
{
  uint8_t routerMac[6];
  WiFi.macAddress(routerMac);
  return memcmp(mac, routerMac, 6) == 0 || memcmp(mac, "\0\0\0\0\0\0", 6) == 0;
}

const uint8_t* MqttEspNowRouter::getPeerMacByName(const String& name) const {
    String cleanedName = name;
    cleanedName.trim();
    Serial.print("🔍 Procurando MAC de: ");
    Serial.println(cleanedName);
    if (strcmp(routerName, cleanedName.c_str()) == 0 || strcmp(mqttServerName, cleanedName.c_str()) == 0 ) {
        Serial.print("👉 Encontrado: ");
        Serial.printf("\n MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", routerMac[0], routerMac[1], routerMac[2], routerMac[3], routerMac[4], routerMac[5]);
        return routerMac;
    } 
    for (const auto& peer : childrenPeers) {
        String peerName = peer.name;
        peerName.trim();
        if (strcmp(peerName.c_str(), cleanedName.c_str()) == 0) {
            Serial.print("👉 Encontrado: ");
            Serial.printf("\n MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", peer.mac[0], peer.mac[1], peer.mac[2], peer.mac[3], peer.mac[4], peer.mac[5]);
            return peer.mac;
        }
    }
    return nullptr;
}


void MqttEspNowRouter::handleReconnectMqtt(int timeoutMin)
{
  if (!mqttClient.connected() && WiFi.isConnected())
  {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > timeoutMin*60*1000)
    {
      Serial.println("Tentando reconectar ao MQTT...");
      mqttClient.connect();
      lastReconnectAttempt = now;
    }
  }
}

void MqttEspNowRouter::handlePeerVerification(int timeoutMin) 
{
    unsigned long now = millis();

    if (now - this->lastActivity < timeoutMin * 60 * 1000) return;

    this->lastActivity = now;
    Serial.println("🔍 Verificando estado dos peers...");

    for (auto& peer : childrenPeers) {
        if (!esp_now_is_peer_exist(peer.mac)) {
            Serial.print("🔄 Peer não registrado, adicionando novamente: ");
            Serial.println(peer.name);
            addPeer(peer.mac);
        }

        // Envia um PING via ESP-NOW a cada 30s
        if (now - peer.lastPingSent > 30 * 1000) {
            peer.lastPingSent = now;
            publishENow(routerName, peer.name, "PING", "PING");
            Serial.println("📨 Enviado PING para " + peer.name);
        }

        // Verifica se o peer está online ou offline (sem resposta por 2 minutos)
        if (now - peer.lastPongReceived > 120 * 1000) {
            if (peer.online) {
                Serial.println("⚠️ Peer " + peer.name + " está offline.");
                peer.online = false;
            }
        } else {
            if (!peer.online) {
                Serial.println("✅ Peer " + peer.name + " voltou online.");
                peer.online = true;
            }
        }
    }
}
#endif

