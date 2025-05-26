#ifdef ESP32
#pragma once
#include <Arduino.h>
#include <vector>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>


// Declarações externas
extern const char otaPageTemplate[] PROGMEM;
extern std::vector<String>& devices;
extern AsyncWebServer server;

// Protótipo da função
String generateHtml(const std::vector<String>& devices); 
#endif