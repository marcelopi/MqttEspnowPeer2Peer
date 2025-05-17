#ifndef _config_router_hpp_
#define _config_router_hpp_
#define CHANNEL_AUTO 255

const char* WIFI_SSID = "xxx";
const char* WIFI_PASSWORD = "xxx";
const char* OTA_PASSWORD = "xxx";

const char* NTP_SERVER = "pool.ntp.br";
const char* MQTT_SERVER_IP = "192.168.1.xx";
const char* MQTT_USER = "xxx";
const char* MQTT_PWD = "xxx";

const IPAddress WIFI_GATEWAY(192, 168, 1, xxx);       
const IPAddress WIFI_SUBNET(255, 255, 255, 0);      
const IPAddress WIFI_DNS(8, 8, 8, 8);    
uint8_t WIFI_CHANNEL = CHANNEL_AUTO;     // SET CHANNEL_AUTO PARA AUTODETECÇÃO     
uint8_t ESP_NOW_CHANNEL = CHANNEL_AUTO; // SET CHANNEL_AUTO PARA AUTODETECÇÃO 
#endif
