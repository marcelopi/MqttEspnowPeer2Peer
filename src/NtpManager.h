#ifndef NTP_MANAGER_H
#define NTP_MANAGER_H

#include <Arduino.h>
#include <TimeLib.h>

class NtpManager {
public:
    static void setEpochTime(unsigned long epoch) {
        setTime(epoch);
        _lastSync = millis();
        _epoch = epoch;
        _ntpActive = true;
    }

    // Retorna no formato dd/mm/aaaa hh:mi:ss
    static String getDateTimeString() {
        if (!_ntpActive) return "NTP não sincronizado";
        
        time_t currentTime = _epoch + ((millis() - _lastSync) / 1000);
        return _formatTime(currentTime);
    }

    // Imprime diretamente no Serial
    static void printDateTime() {
        if (!_ntpActive) {
            Serial.println("NTP não sincronizado");
            return;
        }
        Serial.println(getDateTimeString());
    }

    static bool isTimeSet() { return _ntpActive; }

private:
    static unsigned long _epoch;
    static unsigned long _lastSync;
    static bool _ntpActive;

    static String _formatTime(time_t t) {
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d %02d:%02d:%02d",
                 day(t), month(t), year(t),
                 hour(t), minute(t), second(t));
        return String(buffer);
    }
};

unsigned long NtpManager::_epoch = 0;
unsigned long NtpManager::_lastSync = 0;
bool NtpManager::_ntpActive = false;

#endif