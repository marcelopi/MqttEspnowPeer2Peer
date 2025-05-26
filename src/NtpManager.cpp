#include "NtpManager.h"

// Inicializa o ponteiro estático
NtpManager* NtpManager::instance = nullptr;

NtpManager::NtpManager(int timezoneOffset) {
    this->epoch = 0;
    this->lastSync = 0;
    this->ntpActive = false;
    this->timezoneOffset = timezoneOffset * 3600; // Conversão de horas para segundos
    NtpManager::instance = this; // Registra a instância ativa
}

void NtpManager::setEpochTime(unsigned long epochTime) {
    setTime(epochTime); // Mantém o controle interno (TimeLib ou time.h)
    this->lastSync = millis();
    this->epoch = epochTime;
    this->ntpActive = true;
}

String NtpManager::getDateTimeString() const {
    if (!this->ntpActive) return "NTP não sincronizado";

    time_t currentTime = getEpochTime();
    return this->formatTime(currentTime);
}

time_t NtpManager::getEpochTime() const {
    if (!this->ntpActive) return 0;
    return this->epoch + ((millis() - this->lastSync) / 1000);
}

void NtpManager::printDateTime() const {
    if (!this->ntpActive) {
        Serial.println("NTP não sincronizado");
        return;
    }
    Serial.println(this->getDateTimeString());
}

bool NtpManager::isTimeSet() const {
    return this->ntpActive;
}

String NtpManager::formatTime(time_t t) const {
    t += timezoneOffset; // Aplica o timezone

    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d %02d:%02d:%02d",
             day(t), month(t), year(t),
             hour(t), minute(t), second(t));
    return String(buffer);
}
