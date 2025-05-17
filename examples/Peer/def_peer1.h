#ifndef _def_refaserv_hpp_
#define _def_refaserv_hpp_

#define outRefletorAreaServico 5
const char* deviceName = "EspNowCli01";
const IPAddress wifiLocalIp(192, 168, 1, 101);  
const uint8_t macRouter[] = {0x??, 0x??, 0x??, 0x??, 0x??, 0x??};
const char *localName = "Peer1";
const std::vector<DeviceInfo> childrenPeers = {
    {"Peer2",  {0x??, 0x??, 0x??, 0x??, 0x??, 0x??}},
    {"Peer3",  {0x??, 0x??, 0x??, 0x??, 0x??, 0x??}},
    };
const std::vector<DeviceInfo> Routers = {
    {"Router1", {0x3C, 0x71, 0xBF, 0x6A, 0x32, 0x20}},
    };
#endif