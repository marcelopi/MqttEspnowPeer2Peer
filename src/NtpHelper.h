#ifndef NTP_HELPER_H
#define NTP_HELPER_H

#pragma once

#include <Arduino.h>

#if defined(ESP32)
  #include <WiFi.h>
  #include <time.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <WiFiUdp.h>
  #include <NTPClient.h>
#endif

class NtpHelper
{
public:
    NtpHelper(int timeZone = -3, unsigned long updateInterval = 3600000, const char *ntpServer = "pool.ntp.org");

    void begin();
    void updateNTP();

    void printDateTimeNTP();
    time_t printDateTimeNtpEpoch();
    String formatDateTimeNTP();
    String formatDateTimeNtpEpoch();
    String formatInputDateTimeNTP(time_t rawtime);

private:
    String ntpServerName;
    int timeZone;
    unsigned long updateInterval;

#if defined(ESP8266)
    WiFiUDP udp;
    NTPClient timeClient;
#endif

    bool ntpUpdated = false;

    time_t getEpochTime();
    bool fetchTime();
};


#endif
