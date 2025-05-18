#include "NtpManager.h"

// Inicializa o ponteiro estático
NtpManager* NtpManager::instance = nullptr;

NtpManager::NtpManager() {
    this->epoch = 0;
    this->lastSync = 0;
    this->ntpActive = false;
    NtpManager::instance = this; // Registra a instância ativa
}

void NtpManager::setEpochTime(unsigned long epochTime) {
    setTime(epochTime);
    this->lastSync = millis();
    this->epoch = epochTime;
    this->ntpActive = true;
}

String NtpManager::getDateTimeString() const {
    if (!this->ntpActive) return "NTP não sincronizado";

    time_t currentTime = this->epoch + ((millis() - this->lastSync) / 1000);
    return this->formatTime(currentTime);
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
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d %02d:%02d:%02d",
             day(t), month(t), year(t),
             hour(t), minute(t), second(t));
    return String(buffer);
}
