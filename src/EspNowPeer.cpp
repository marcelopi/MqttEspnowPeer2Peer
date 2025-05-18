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
        bool success = esp_now_add_peer((uint8_t*)peerMac, ESP_NOW_ROLE_COMBO, 6, nullptr, 0);
        //bool success = esp_now_add_peer((uint8_t*)peerMac, ESP_NOW_ROLE_COMBO, this->espnowChannel, nullptr, 0);
        if (success) {
            Serial.println("✅ Peer adicionado com sucesso (ESP8266).");
        } else {
            Serial.println("❌ Falha ao adicionar peer (ESP8266).");
        }
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
#if defined(ESP8266)
    int msgLen = sizeof(fullMsg); // ESP8266 exige uint8_t
#else
    size_t msgLen = fullMsg.length();  // ESP32 aceita size_t
#endif

    if (destination == "ALL") {
        for (const auto& peer : childrenPeers) {
            Serial.printf("🔁 Encaminhando para '%s'\n", peer.name.c_str());
#if defined(ESP8266)
    bool result = esp_now_send(const_cast<uint8_t*>(peer.mac), (uint8_t *) &msgData, msgLen);
    if (!result) {
        Serial.println("❌ Falha no envio ESP-NOW (ESP8266).");
    }
#else
    esp_err_t result = esp_now_send(peer.mac, (uint8_t*)msgData, msgLen);
    if (result != ESP_OK) {
        Serial.printf("❌ Falha no envio ESP-NOW (ESP32), código: 0x%X\n", result);
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
    bool result = esp_now_send(const_cast<uint8_t*>(peerMac), (uint8_t *) &msgData, msgLen);
    if (!result) {
        Serial.println("❌ Falha no envio ESP-NOW (ESP8266).");
    }
#else
    esp_err_t result = esp_now_send(peerMac, (uint8_t*)msgData, msgLen);
    if (result != ESP_OK) {
        Serial.printf("❌ Falha no envio ESP-NOW (ESP32), código: 0x%X\n", result);
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
        payload = String((const char*)data, len);  // Mantém original para ESP32
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

    // Primeiro tenta rota local
    for (int i = 0; i < instance->routeCount; ++i) {
        const Route& r = instance->routes[i];
        if (r.destination == destination) {
            if (r.handler) {
                r.handler(message);
                return;
            }
        }
    }
    // Subscreve PING para todos os filhos e envia PONG
    if (destination == instance->localName && action == "PING") {
        instance->publishENow(destination, source, "PING", "PONG");
        Serial.println("🔁 Respondido PING com PONG");
        return;
    }

    if (destination == instance->localName && action == "PONG") {
        Serial.println("✅ Recebido PONG");
        // Atualiza estado online aqui se quiser
        // Exemplo: procurar o peer pelo nome e marcar online = true
        return;
    }

    // Se chegou aqui, não encontrou destino local, tenta repassar
    if (destination == "ALL" && source != instance->localName) {
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

void EspNowPeer::handlePeerVerification(int timeoutMin) {
    unsigned long timeoutMillis = (unsigned long)timeoutMin * 60 * 1000;

    if (millis() - _lastActivity > timeoutMillis) {
        _lastActivity = millis();
        Serial.println("🔍 Verificação de peers...");

        // Verifica e adiciona peers se necessário
        auto verifyPeerList = [&](std::vector<DeviceInfo>& peers) {
            for (auto& peer : peers) {
                if (!esp_now_is_peer_exist(peer.mac)) {
                    Serial.print("➕ Peer ausente, adicionando: ");
                    Serial.println(peer.name);
                    addPeer(peer.mac);
                }
            }
        };

        verifyPeerList(routers);
        verifyPeerList(childrenPeers);

        // Envia PING para todos os roteadores
        for (auto& router : routers) {
            router.lastPingSent = millis();
            router.online = false;  // Será atualizado se receber PONG
            publishENow(this->localName, router.name, "PING", "PING");
            Serial.print("📤 Enviado PING para ");
            Serial.println(router.name);
        }

        // Envia PING para todos os filhos (peer children)
        for (auto& child : childrenPeers) {
            child.lastPingSent = millis();
            child.online = false;
            publishENow(this->localName, child.name, "PING", "PING");
            Serial.print("📤 Enviado PING para filho ");
            Serial.println(child.name);
        }

    }
}




