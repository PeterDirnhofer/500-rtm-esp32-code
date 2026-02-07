#ifndef WIFI_PC_INTERFACE_H
#define WIFI_PC_INTERFACE_H

#include <string>

class WifiPcInterface {
public:
  static void start();
  static void startStation();
  static int send(const char *fmt, ...);
  static bool isActive();
};

#endif // WIFI_PC_INTERFACE_H
