#include "NtpHelper.h"

NtpHelper::NtpHelper(int timeZone, unsigned long updateInterval, const char *ntpServer)
    : ntpServerName(ntpServer), timeZone(timeZone), updateInterval(updateInterval)
#if defined(ESP8266)
    , timeClient(udp, ntpServerName.c_str(), timeZone * 3600, updateInterval)
#endif
{
}

void NtpHelper::begin()
{
#if defined(ESP32)
    configTime(timeZone * 3600, 0, ntpServerName.c_str());
#elif defined(ESP8266)
    timeClient.begin();
    timeClient.update();
#endif
    fetchTime();
}

void NtpHelper::updateNTP()
{
    if (fetchTime())
    {
        Serial.println("✅ Atualizada a hora do NTP");
    }
    else
    {
        Serial.println("⚠️ Falha na atualização do NTP");
    }
}

bool NtpHelper::fetchTime()
{
#if defined(ESP32)
    struct tm timeinfo;
    const unsigned long timeout = 5000;
    unsigned long start = millis();

    while (millis() - start < timeout)
    {
        if (getLocalTime(&timeinfo))
        {
            ntpUpdated = true;
            return true;
        }
        delay(100);
    }

    ntpUpdated = false;
    return false;

#elif defined(ESP8266)
    const unsigned long timeout = 5000;
    unsigned long start = millis();

    while (!timeClient.update())
    {
        timeClient.forceUpdate();
        if (millis() - start > timeout)
        {
            ntpUpdated = false;
            return false;
        }
        delay(100);
    }

    ntpUpdated = true;
    return true;
#endif
}

time_t NtpHelper::getEpochTime()
{
#if defined(ESP32)
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        return 0;
    }
    return mktime(&timeinfo) - timeZone * 3600;
#elif defined(ESP8266)
    return timeClient.getEpochTime();
#endif
}

void NtpHelper::printDateTimeNTP()
{
    time_t rawtime = getEpochTime();
    if (rawtime <= 10000) // Verifica data inválida
    {
        Serial.println("⚠️ Data inválida.");
        return;
    }

    struct tm *ti = localtime(&rawtime);
    String fData = (ti->tm_mday < 10 ? "0" : "") + String(ti->tm_mday) + "/" +
                   (ti->tm_mon + 1 < 10 ? "0" : "") + String(ti->tm_mon + 1) + "/" +
                   String(ti->tm_year + 1900);

    Serial.println(fData + " " + (ti->tm_hour < 10 ? "0" : "") + String(ti->tm_hour) + ":" +
                   (ti->tm_min < 10 ? "0" : "") + String(ti->tm_min) + ":" +
                   (ti->tm_sec < 10 ? "0" : "") + String(ti->tm_sec));
}

time_t NtpHelper::printDateTimeNtpEpoch()
{
    return getEpochTime();
}

String NtpHelper::formatDateTimeNtpEpoch()
{
    return String(getEpochTime());
}

String NtpHelper::formatDateTimeNTP()
{
    time_t rawtime = getEpochTime();
    if (rawtime <= 10000)
    {
        return "00/00/0000 00:00:00";
    }

    struct tm *ti = localtime(&rawtime);
    String fData = (ti->tm_mday < 10 ? "0" : "") + String(ti->tm_mday) + "/" +
                   (ti->tm_mon + 1 < 10 ? "0" : "") + String(ti->tm_mon + 1) + "/" +
                   String(ti->tm_year + 1900);

    String fHora = (ti->tm_hour < 10 ? "0" : "") + String(ti->tm_hour) + ":" +
                    (ti->tm_min < 10 ? "0" : "") + String(ti->tm_min) + ":" +
                    (ti->tm_sec < 10 ? "0" : "") + String(ti->tm_sec);

    return fData + " " + fHora;
}

String NtpHelper::formatInputDateTimeNTP(time_t rawtime)
{
    if (rawtime <= 10000)
    {
        return "00/00/0000 00:00:00";
    }

    struct tm *ti = localtime(&rawtime);
    String fDataHora = (ti->tm_mday < 10 ? "0" : "") + String(ti->tm_mday) + "/" +
                        (ti->tm_mon + 1 < 10 ? "0" : "") + String(ti->tm_mon + 1) + "/" +
                        String(ti->tm_year + 1900) + " " +
                        (ti->tm_hour < 10 ? "0" : "") + String(ti->tm_hour) + ":" +
                        (ti->tm_min < 10 ? "0" : "") + String(ti->tm_min) + ":" +
                        (ti->tm_sec < 10 ? "0" : "") + String(ti->tm_sec);

    return fDataHora;
}
