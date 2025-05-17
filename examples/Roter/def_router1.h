#ifndef _def_router1_hpp_
#define _def_router1_hpp_

#define outRefletorFundo 5
const char* deviceName = "EspNowRouter01";
const IPAddress wifiLocalIp(192, 168, 1, 100);  
const char *mqttServerName = "Server";
const char *routerName = "Router1";
uint8_t routerMac[] = {0x??, 0x??, 0x??, 0x??, 0x??, 0x??};
const std::vector<DeviceInfo> childrenPeers = {
    {"Peer1",  {0x??, 0x??, 0x??, 0x??, 0x??, 0x??}},
};

#endif