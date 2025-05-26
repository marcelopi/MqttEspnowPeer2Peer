#ifndef NTP_MANAGER_H
#define NTP_MANAGER_H
#pragma once
#include <Arduino.h>
#include <TimeLib.h>




class NtpManager {
public:
    NtpManager(int timezoneOffset);
    
    void setEpochTime(unsigned long epochTime);
    time_t getEpochTime() const;

    String getDateTimeString() const;
    void printDateTime() const;

    bool isTimeSet() const;
    String formatTime(time_t t) const;

    // Singleton opcional
    static NtpManager* instance;

private:
    unsigned long epoch;
    unsigned long lastSync;
    int timezoneOffset;
    bool ntpActive;
};
#endif