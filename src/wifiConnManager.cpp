#include "wifiConnManager.h"
#include "OtaPage.h"

wifiConnManager *wifiConnManager::instance = nullptr;

wifiConnManager::wifiConnManager() : server(80)
{
    wifiConnManager::instance = this;
#ifdef ESP8266
    this->netMode = ESPNOW; // Corrigido: não pode ser feito no header
#endif
}


uint8_t wifiConnManager::begin(const char *deviceName, const char *ssid, const char *wifiPwd, const char *otaPwd,
                            uint8_t defaultChannel, IPAddress localIP, IPAddress gateway,
                            IPAddress subnet, IPAddress dns, int netMode, const std::vector<String>& deviceOtaList )
{
    this->deviceName = deviceName;
    this->ssid = ssid;
    this->wifiPwd = wifiPwd;
    this->otaPwd = otaPwd;
    this->currentChannel = defaultChannel;
    this->deviceOtaList = deviceOtaList;
      this->localIP = localIP;
    this->gateway = gateway;
    this->subnet = subnet;
    this->dns = dns;
    this->lastActivity = 0;
    this->startAttemptTime = millis();
    // Setando as preferências
    this->getNetMode(netMode);

    // Chamar startWiFi com base no modo carregado
    this->startWiFi(this->netMode);

    return this->currentChannel;
}

void wifiConnManager::getNetMode(int defaultNetMode){
#ifdef ESP32
    preferences.begin("wificon", false);

    // Se for HYBRID, sempre usar e gravar
    if (defaultNetMode == HYBRID) {
        this->netMode = HYBRID;
        preferences.putInt("mode", HYBRID);
        Serial.printf("\n📥 Forçando modo de Network: HYBRID\n");
    } else {
        if (!preferences.isKey("mode")) {
            this->netMode = defaultNetMode;
            preferences.putInt("mode", this->netMode);
            Serial.println("⚠️ Preferences sem valor. Setado modo padrão.");
        } else {
            this->netMode = preferences.getInt("mode", defaultNetMode);
        }

        const char* modeStr = "UNDEFINED";
        switch (this->netMode) {
            case 0: modeStr = "ESPNOW"; break;
            case 1: modeStr = "WIFI"; break;
            case 2: modeStr = "HYBRID"; break;
        }
        Serial.printf("\n📥 Modo de Network: %s\n", modeStr);
    }

#elif defined(ESP8266)
    EEPROM.begin(EEPROM_SIZE);

    // Se for HYBRID, sempre usar e gravar
    if (defaultNetMode == HYBRID) {
        this->netMode = HYBRID;
        EEPROM.write(EEPROM_ADDR_UPDATE_MODE, HYBRID);
        EEPROM.commit();
        Serial.printf("\n📥 Forçando modo de Network: HYBRID\n");
    } else {
        uint8_t val = EEPROM.read(EEPROM_ADDR_UPDATE_MODE);
        if (val == EEPROM_DEFAULT_VALUE) {
            this->netMode = defaultNetMode;
            EEPROM.write(EEPROM_ADDR_UPDATE_MODE, this->netMode);
            EEPROM.commit();
            Serial.println("⚠️ EEPROM sem valor. Setado modo padrão.");
        } else {
            this->netMode = val;
        }

        const char* modeStr = "UNDEFINED";
        switch (this->netMode) {
            case 0: modeStr = "ESPNOW"; break;
            case 1: modeStr = "WIFI"; break;
            case 2: modeStr = "HYBRID"; break;
        }
        Serial.printf("\n📥 Modo de Network: %s\n", modeStr);
    }
#endif
}


void wifiConnManager::setNetMode( int NetMode)
{
        this->netMode = NetMode;
#ifdef ESP32
        this->preferences.putInt("mode", this->netMode);
        
        preferences.end();
#elif defined(ESP8266)
        EEPROM.write(EEPROM_ADDR_WIFI_MODE, netMode);
        EEPROM.commit();
#endif
        Serial.println("✅ Atualização solicitada, reiniciando...");
        delay(1000);
        ESP.restart();
}


bool wifiConnManager::isInUpdateMode()
{
    return this->netMode;
}

void wifiConnManager::handle(int netModeTimeoutMin, int NetMode)
{
    if (this->netMode == WIFI && (millis() - lastActivity > (unsigned long)(netModeTimeoutMin * 60UL * 1000UL)))
        this->setNetMode(NetMode);
    if (this->netMode >= 1)
        ArduinoOTA.handle();
}

void wifiConnManager::startWiFi(int NetMode)
{
    if (NetMode == ESPNOW) 
    {
        Serial.println("\n🔄 Entrando em modo ESP-NOW");
        WiFi.mode(WIFI_STA);
        configureWiFiChannel();
     
        // Inicialização do ESP-NOW
#if defined(ESP32)
    esp_err_t initStatus = esp_now_init();
    if (initStatus != ESP_OK) {
        Serial.printf("⚠️ Falha ao iniciar ESP-NOW (código: 0x%X)\n", initStatus);
    }else{
        notifyEspNowReady();
    }
#elif defined(ESP8266)
    if (esp_now_init() != 0) {
        Serial.println("⚠️ Falha ao iniciar ESP-NOW no ESP8266");
    }else{
        esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
        notifyEspNowReady();
    }
#endif

    }
    else if (NetMode == WIFI) 
    {
        Serial.println("\n🔄 Entrando em modo WI-FI");
        WiFi.hostname(deviceName);
        WiFi.config(localIP, gateway, subnet, dns);
        Serial.println("\n✅ Iniciando Wifi");
        WiFi.begin(ssid, wifiPwd);
        configureWiFiChannel();
        Serial.print("Conectando");
        unsigned long startAttemptTime = millis();
        while (WiFi.status() != WL_CONNECTED &&
               millis() - startAttemptTime < netModeTimeout)
        {
            delay(500);
            Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("\n✅ Wi-fi Conectado.");
            notifyWifiReady();
            setupOTA();
        }
        else
        {
            Serial.println("\n⚠️ Falha ao conectar ao Wi-fi");
            Serial.println("\nRetornando ao modo ESP-NOW");
            this->startWiFi(0);
        }
        lastActivity = millis();
    } else if (NetMode == HYBRID) 
    {
        WiFi.mode(WIFI_AP_STA);
        Serial.println("\n✅ Iniciando Wifi");
        WiFi.hostname(deviceName);
        WiFi.config(localIP, gateway, subnet, dns);
        WiFi.begin(ssid, wifiPwd);
        configureWiFiChannel();

        Serial.print("Conectando");
        unsigned long startAttemptTime = millis();
        while (WiFi.status() != WL_CONNECTED &&
               millis() - startAttemptTime < netModeTimeout)
        {
            delay(500);
            Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("\n✅ Wi-fi Conectado.");
            notifyWifiReady();
            setupOTA();
        }
        else
        {
            Serial.println("\n⚠️ Falha ao conectar ao Wi-fi");
            Serial.println("\nRetornando ao modo ESP-NOW");
            this->startWiFi(0);
        }
        // Inicialização do ESP-NOW
#if defined(ESP32)
    esp_err_t initStatus = esp_now_init();
    if (initStatus != ESP_OK) {
        Serial.printf("⚠️ Falha ao iniciar ESP-NOW (código: 0x%X)\n", initStatus);
    }else{
        notifyEspNowReady();
        setupWebServer();
    }

#elif defined(ESP8266)
    if (esp_now_init() != 0) {
        Serial.println("⚠️ Falha ao iniciar ESP-NOW no ESP8266");
    }else{
        notifyEspNowReady();
        setupWebServer();
    }
#endif
    }
    this->printNetworkInfo();
}
void wifiConnManager::setupWebServer() {
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "text/html", generateHtml(this->deviceOtaList));
    });

    server.on("/updateMode", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!request->hasHeader("X-Device-Name")) {
            request->send(400, "text/plain", "Faltando header: X-Device-Name");
            return;
        }

        String deviceName = request->header("X-Device-Name");

        // Verifica se está na lista permitida
        auto it = std::find(deviceOtaList.begin(), deviceOtaList.end(), deviceName);
        if (it == deviceOtaList.end()) {
            request->send(404, "text/plain", "Dispositivo não autorizado.");
            return;
        }

        if (onUpdateModeCallback) {
            onUpdateModeCallback(deviceName);
            request->send(200, "text/plain", "Modo UPDATE enviado para " + deviceName);
        } else {
            request->send(500, "text/plain", "Callback não configurado.");
        }
    });

    server.begin();
}

void wifiConnManager::onUpdateMode(UpdateModeCallback cb) {
    onUpdateModeCallback = cb;
}
void wifiConnManager::configureWiFiChannel()
{
    Serial.println("\n✅ Configurando o canal...");
    if (this->netMode == 0) // ESPNOW
    {
        this->currentChannel = getWiFiChannel(ssid);
    }
    else if (this->netMode >= 1) // WIFI ou HYBRID
    {
#if defined(ESP32)
        if (this->currentChannel == CHANNEL_AUTO)
            this->currentChannel = WiFi.channel();
#elif defined(ESP8266)
        // ESP8266 não possui WiFi.channel(), manter canal atual
#endif
    }

    Serial.print("\n Identificado o canal...");
    Serial.println(this->currentChannel);
    Serial.print("\n Ip Local: ");
    Serial.println(WiFi.localIP());

#ifdef ESP32
    esp_wifi_set_channel(this->currentChannel, WIFI_SECOND_CHAN_NONE);
#elif defined(ESP8266)
    WiFi.disconnect(); 
    delay(100);
    wifi_set_channel(this->currentChannel);
#endif
}

void wifiConnManager::onWifiReady(std::function<void()> callback)
{
    wifiReadyCallback = callback;
    if (wifiReady && wifiReadyCallback) {
        wifiReadyCallback(); // executa na hora se já estiver pronto
    }
}


void wifiConnManager::notifyWifiReady() {
    wifiReady = true;
    if (wifiReadyCallback) {
        wifiReadyCallback();
    }
}


void wifiConnManager::onEspNowReady(std::function<void()> callback)
{
    espNowReadyCallback = callback;
    if (espNowReady && espNowReadyCallback) {
        espNowReadyCallback(); // executa na hora se já estiver pronto
    }
}


void wifiConnManager::notifyEspNowReady() {
    espNowReady = true;
    if (espNowReadyCallback) {
        espNowReadyCallback();
    }
}
int32_t wifiConnManager::getWiFiChannel(const char *ssid)
{
    int32_t n = WiFi.scanNetworks();
    if (n > 0)
    {
        for (uint8_t i = 0; i < n; i++)
        {
            if (!strcmp(ssid, WiFi.SSID(i).c_str()))
            {
                Serial.print("\n✅ Detectado canal de wifi em ");
#if defined(ESP32)
                Serial.println(WiFi.channel(i));
                return WiFi.channel(i);
#elif defined(ESP8266)
                Serial.println(WiFi.channel(i)); // Também é suportado
                return WiFi.channel(i);
#endif
            }
        }
    }
    return 0;
}

void wifiConnManager::printNetworkInfo() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    Serial.printf("\n⚙️ MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.print("⚙️ MAC Array format:{");
    for (int i = 0; i < 6; i++) {
        Serial.print("0x");
        if (mac[i] < 0x10)
            Serial.print("0");
        Serial.print(mac[i], HEX);
        if (i < 5)
            Serial.print(", ");
    }
    Serial.println("}");
    Serial.print("⚙️IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("⚙️ Canal Wi-Fi atual: ");
    Serial.println(WiFi.channel());
}
void wifiConnManager::setupOTA()
{
    Serial.println("\n✅Iniciando OTA");

    ArduinoOTA.setHostname(deviceName);
    ArduinoOTA.setPassword(otaPwd);

    ArduinoOTA.onStart([this]() { startOTA(); });
    ArduinoOTA.onEnd([this]() { endOTA(); });
    ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total)
                          { progressOTA(progress, total); });
    ArduinoOTA.onError([this](ota_error_t error) { errorOTA(error); });

    ArduinoOTA.begin();
    Serial.println("\n✅ OTA iniciado.");
}

void wifiConnManager::startOTA()
{
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "flash" : "filesystem";
    Serial.println("Start updating " + type);
}

void wifiConnManager::endOTA()
{
    Serial.println("\nEnd");
    Serial.println("Compilação OTA Finalizada");
    Serial.println("Retornando ao modo de networt: ESPNOW");
    this->setNetMode(ESPNOW);
}

void wifiConnManager::progressOTA(unsigned int progress, unsigned int total)
{
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
}

void wifiConnManager::errorOTA(ota_error_t error)
{
    Serial.printf("Error[%u]: ", error);

    if (error == OTA_AUTH_ERROR)
        Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
        Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
        Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
        Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
        Serial.println("End Failed");
}
