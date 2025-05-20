#include "EspNowPeer.h"

EspNowPeer *EspNowPeer::instance = nullptr;

EspNowPeer::EspNowPeer() {
    instance = this; // Registra a instância ativa
}

void EspNowPeer::begin(uint8_t espnowChannel, const char *localName, const std::vector<DeviceInfo>& routers, const std::vector<DeviceInfo>& childrenPeers) {
    // Armazenar configurações
    this->localName = localName;
    this->routers = routers;
    this->childrenPeers = childrenPeers;
    this->espnowChannel = espnowChannel;

    Serial.println("🟢 Inicializando peers no ESP-NOW...");

    // Adicionar roteadores
    for (const auto& peer : routers) {
        Serial.print("Adicionado peer ");
        Serial.println(peer.name);
        addPeer(peer.mac);
    }

    // Adicionar peers filhos
    for (const auto& peer : childrenPeers) {
        Serial.print("Adicionado peer ");
        Serial.println(peer.name);
        addPeer(peer.mac);
    }


// Registrar callback de recebimento
#if defined(ESP32)
    esp_now_register_recv_cb([](const uint8_t *mac, const uint8_t *data, int len) {
        if (instance) {
            instance->onReceive(mac, data, len);
        }
    });
#elif defined(ESP8266)
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb([](uint8_t *mac, uint8_t *data, uint8_t len) {
        if (instance) {
            // O tipo do parâmetro 'len' é uint8_t no ESP8266
            instance->onReceive(mac, data, static_cast<int>(len)); // Converte para int, se necessário
        }
    });
#endif

    Serial.println("✅ ESP-NOW configurado com sucesso!");
}


void EspNowPeer::addPeer(const uint8_t *peerMac) {
#if defined(ESP32)
    if (!esp_now_is_peer_exist(peerMac)) {
        esp_now_peer_info_t peer = {};
        memcpy(peer.peer_addr, peerMac, 6);
        peer.channel = this->espnowChannel;
        peer.encrypt = false;
        esp_err_t result = esp_now_add_peer(&peer);
        if (result != ESP_OK) {
            Serial.print("Falha ao adicionar peer (ESP32): ");
            Serial.println(result);
            return;
        } else {
            Serial.println("✅ Peer adicionado com sucesso (ESP32).");
        }
    } else {
        Serial.println("Peer já existe (ESP32).");
    }

#elif defined(ESP8266)
    // Definir role como COMBO (envia e recebe)
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

    // Verificar se o peer já está adicionado
    if (esp_now_is_peer_exist((uint8_t*)peerMac)) {
        Serial.println("ℹ️ Peer já está registrado (ESP8266).");
    } else {
        bool success = esp_now_add_peer((uint8_t*)peerMac, ESP_NOW_ROLE_COMBO, this->espnowChannel, NULL, 0);
         Serial.println("✅ Peer adicionado com sucesso (ESP8266).");
    }
#endif
}

void EspNowPeer::subscribe(const String &source, const String &destination, const String &action, LocalHandler handler) {
    if (routeCount >= MAX_ROUTES) {
        Serial.println("⚠️ Número máximo de rotas atingido.");
        return;
    }

    Route& route = routes[routeCount++];
    route.source = source;
    route.destination = destination;
    route.action = action;
    route.handler = handler;

    Serial.printf("🔗 Rota registrada: src=%s dest=%s act=%s\n", source.c_str(), destination.c_str(), action.c_str());
}



void EspNowPeer::publishENow(const String &source, const String &destination, const String &action, const String &message) {
    String fullMsg = source + ">" + destination + "/" + action + "|" + message;
    const uint8_t* msgData = (uint8_t *)fullMsg.c_str();

    // Mantido igual para ESP32
    size_t msgLen = fullMsg.length();

    if (destination == "ALL") {
        for (const auto& peer : childrenPeers) {
            Serial.printf("🔁 Encaminhando para '%s'\n", peer.name.c_str());

#if defined(ESP8266)
            // ========== Código ESP8266 ==========
            uint8_t* rawData = const_cast<uint8_t*>(msgData); // Remover const
            bool result = esp_now_send(
                const_cast<uint8_t*>(peer.mac), // MAC sem const
                rawData,                       // Dados sem const
                msgLen                         // Tamanho
            );
            Serial.print("Resultado do envio (ESP8266)");
            Serial.println(result);
#else
            esp_err_t result = esp_now_send(peer.mac, msgData, msgLen);
            if (result != ESP_OK) {
                Serial.printf("❌ Falha no envio (ESP32), código: 0x%X\n", result);
            }
#endif

            Serial.println("📨 Mensagem enviada:");
            Serial.println(fullMsg);
        }
    } else {
        const uint8_t* peerMac = getPeerMacByName(destination);
        if (peerMac == nullptr) {
            Serial.printf("⚠️ Dispositivo '%s' não encontrado.\n", destination.c_str());
            return;
        }

#if defined(ESP8266)
        // ========== Código ESP8266 ==========
        uint8_t* rawData = const_cast<uint8_t*>(msgData); // Remover const
        bool result = esp_now_send(
            const_cast<uint8_t*>(peerMac), // MAC sem const
            rawData,                       // Dados sem const
            msgLen                         // Tamanho
        );
        Serial.print("Resultado do envio (ESP8266)");
        Serial.println(result);
#else
        esp_err_t result = esp_now_send(peerMac, msgData, msgLen);
        if (result != ESP_OK) {
            Serial.printf("❌ Falha no envio (ESP32), código: 0x%X\n", result);
        }
#endif

        Serial.println("📨 Mensagem enviada:");
        Serial.println(fullMsg);
    }
}
void EspNowPeer::onReceive(const uint8_t* mac, const uint8_t* data, int len) {
    if (!instance) return;

    String payload;

    #if defined(ESP32)
        payload = String((const char*)data, len);  
    #elif defined(ESP8266)
        // Alternativa para ESP8266
        char* buffer = new char[len + 1];
        memcpy(buffer, data, len);
        buffer[len] = '\0';
        payload = String(buffer);
        delete[] buffer;
    #endif



    // Exemplo de formato: "src|dest|action|mensagem"
    int idx1 = payload.indexOf('>');
    int idx2 = payload.indexOf('/', idx1 + 1);
    int idx3 = payload.indexOf('|', idx2 + 1);

    if (idx1 < 0 || idx2 < 0 || idx3 < 0) {
        Serial.println("❌ Formato inválido da mensagem ESP-NOW");
        Serial.print("Payload recebido:");
        Serial.println(payload);
        return;
    }

    String source = payload.substring(0, idx1);
    String destination = payload.substring(idx1 + 1, idx2);
    String action = payload.substring(idx2 + 1, idx3);
    String message = payload.substring(idx3 + 1);

    Serial.println("\n📩=== Mensagem Recebida ===");
    Serial.print("Origem: ");
    Serial.println(source);
    Serial.print("Destino: ");
    Serial.println(destination);
    Serial.print("Ação: ");
    Serial.println(action);
    Serial.print("Mensagem: ");
    Serial.println(message);

    // Subscreve PING e responde com PONG
    if (destination == instance->localName && action == "PING") {
        instance->publishENow(destination, source, "PONG", "PONG");
        Serial.println("🔁 Respondido PING com PONG");
        return;
    // Subscreve PONG e registrando lastPongReceived e peer.online = true
    } else if (destination == instance->localName && action == "PONG") {
        Serial.println("✅ Recebendo e tratando mensagem PONG...");
        String cleanedSource = source;
        cleanedSource.trim();
        for (auto& peer : instance->childrenPeers) { 
            String peerName = peer.name;
            peerName.trim();
            if (strcmp(peerName.c_str(), cleanedSource.c_str()) == 0) {
                Serial.print("🌍 Atualizado status online para peer:");
                Serial.println(peer.name);
                unsigned long now = millis();
                peer.lastPongReceived = now; 
                peer.online = true; 
                break;
            }
        }
        return;
    } else if (destination == instance->localName){
        for (int i = 0; i < instance->routeCount; ++i) {
            const Route& r = instance->routes[i];
            if (r.action == action) {
                if (r.handler) {
                    r.handler(message);
                    return;
                }
            }
        }
    // Se chegou aqui, não encontrou destino local, tenta repassar
    } else if (destination == "ALL" && source != instance->localName) {
        instance->publishENow(source, "ALL", action, message);
        Serial.println("Mensagem ALL repassada para o próximo cliente");
    } else {
        instance->publishENow(source, destination, action, message);
        Serial.println("Mensagem repassada para o próximo cliente");
    }
}


const uint8_t* EspNowPeer::getPeerMacByName(const String& name) const {
    String cleanedName = name;
    cleanedName.trim();
    Serial.print("🔍 Procurando MAC de: ");
    Serial.println(cleanedName);
    for (const auto& peer : routers) {
        String peerName = peer.name;
        peerName.trim();
        if (strcmp(peerName.c_str(), cleanedName.c_str()) == 0) {
            Serial.print("👉 Encontrado: ");
            Serial.printf("\n MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", peer.mac[0], peer.mac[1], peer.mac[2], peer.mac[3], peer.mac[4], peer.mac[5]);
            return peer.mac;
        }
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

int32_t EspNowPeer::getWiFiChannel(const char *ssid) {
    if (int32_t n = WiFi.scanNetworks()) {
        for (uint8_t i = 0; i < n; i++) {
            if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
                Serial.print("Detectado canal de wifi em ");
                Serial.println(WiFi.channel(i));
                return WiFi.channel(i);
            }
        }
    }
    return 0;
}

void EspNowPeer::handlePeerVerification(int timeoutMin) 
{
    unsigned long now = millis();

    if (now - this->_lastActivity < timeoutMin * 60 * 1000) return;

    this->_lastActivity = now;
    Serial.println("🔍 Verificando estado dos peers...");

    for (auto& peer : childrenPeers) {
        // Remover peer se estiver offline
        if (!peer.online) {
            Serial.printf("🔄 Tentando reparar peer offline: %s\n", peer.name.c_str());
            
            #if defined(ESP32)
                esp_now_del_peer(peer.mac); // Remove o peer antigo
            #elif defined(ESP8266)
                esp_now_del_peer((uint8_t*)peer.mac); 
            #endif
            
            addPeer(peer.mac); // Re-adiciona o peer
        }

        // Verificar registro do peer 
        if (!esp_now_is_peer_exist(peer.mac)) {
            Serial.print("🔄 Peer não registrado, adicionando novamente: ");
            Serial.println(peer.name);
            addPeer(peer.mac);
        }

        // Envia um PING via ESP-NOW a cada 30s
        if (now - peer.lastPingSent > 30 * 1000) {
            peer.lastPingSent = now;
            publishENow(localName, peer.name, "PING", "PING");
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
                Serial.println("🌍 Peer " + peer.name + " voltou online.");
                peer.online = true;
            }
        }
    }
}




