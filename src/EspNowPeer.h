#ifndef NTP_HELPER_H
#define NTP_HELPER_H

#include <NTPClient.h>
#include <WiFiUdp.h>

class NtpHelper {
public:
  NtpHelper(int timeZone, unsigned long updateInterval, const char* ntpServer = "pool.ntp.br");

  void begin();                      // Deve ser chamado no setup
  void updateNTP();
  void printDateTimeNTP();
  time_t printDateTimeNtpEpoch();
  String formatDateTimeNTP();
  String formatInputDateTimeNTP(time_t rawtime);
  String formatDateTimeNtpEpoch();
private:
  const char* ntpServerName;
  int timeZone;
  unsigned long updateInterval;

  WiFiUDP udp;
  NTPClient timeClient;
};

#endif