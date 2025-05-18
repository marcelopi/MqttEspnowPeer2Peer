#ifndef NTP_MANAGER_H
#define NTP_MANAGER_H

#include <Arduino.h>
#include <TimeLib.h>

class NtpManager {
public:
    NtpManager();

    void setEpochTime(unsigned long epoch);
    String getDateTimeString() const;
    void printDateTime() const;
    bool isTimeSet() const;

    static NtpManager* instance; // Ponteiro para a instância ativa

private:
    unsigned long epoch;
    unsigned long lastSync;
    bool ntpActive;

    String formatTime(time_t t) const;
};

#endif
