#ifndef WIFI_PC_INTERFACE_H
#define WIFI_PC_INTERFACE_H

#include <string>

class WifiPcInterface {
public:
  static void start();
  static void startStation(const char *ssid, const char *password);
  static int send(const char *fmt, ...);
  static bool isActive();
};

#endif // WIFI_PC_INTERFACE_H
