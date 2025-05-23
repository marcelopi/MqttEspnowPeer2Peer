#include "NtpHelper.h"

NtpHelper::NtpHelper(int timeZone, unsigned long updateInterval, const char* ntpServer)
  : ntpServerName(ntpServer),
    timeZone(timeZone),
    updateInterval(updateInterval),
    timeClient(udp, ntpServerName)
{
}

void NtpHelper::begin() {
  timeClient.setUpdateInterval(updateInterval);
  timeClient.setTimeOffset(timeZone * 3600);
  timeClient.begin();
  updateNTP();
}



void NtpHelper::updateNTP() {
  const unsigned long timeout = 5000; // 5 segundos
  unsigned long start = millis();

  while (!timeClient.update()) {
    timeClient.forceUpdate();
    if (millis() - start > timeout) {
      Serial.println("⚠️ Atualização desnecessária do NTP");
      return;
    }
  }

  Serial.println("✅ Atualizada a hora do NTP");
}



void NtpHelper::printDateTimeNTP() {
  time_t rawtime = timeClient.getEpochTime();
  struct tm *ti = localtime(&rawtime);
  String fData = (ti->tm_mday < 10 ? "0" : "") + String(ti->tm_mday) + "/" +
                 (ti->tm_mon + 1 < 10 ? "0" : "") + String(ti->tm_mon + 1) + "/" +
                 String(ti->tm_year + 1900);

  Serial.println(fData + " " + timeClient.getFormattedTime());
}

time_t NtpHelper::printDateTimeNtpEpoch() {
  return timeClient.getEpochTime();
}

String NtpHelper::formatDateTimeNtpEpoch() {
  return String(timeClient.getEpochTime());
}

String NtpHelper::formatDateTimeNTP() {
  time_t rawtime = timeClient.getEpochTime();
  struct tm *ti = localtime(&rawtime);
  String fData = (ti->tm_mday < 10 ? "0" : "") + String(ti->tm_mday) + "/" +
                 (ti->tm_mon + 1 < 10 ? "0" : "") + String(ti->tm_mon + 1) + "/" +
                 String(ti->tm_year + 1900);

  return fData + " " + timeClient.getFormattedTime();
}

String NtpHelper::formatInputDateTimeNTP(time_t rawtime) {
  struct tm *ti = localtime(&rawtime);
  String fDataHora = (ti->tm_mday < 10 ? "0" : "") + String(ti->tm_mday) + "/" +
                     (ti->tm_mon + 1 < 10 ? "0" : "") + String(ti->tm_mon + 1) + "/" +
                     String(ti->tm_year + 1900) + " " +
                     (ti->tm_hour < 10 ? "0" : "") + String(ti->tm_hour) + ":" +
                     (ti->tm_min < 10 ? "0" : "") + String(ti->tm_min);

  return fDataHora;
}