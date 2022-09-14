#ifndef UART
#define UART
void uartLoop(void *arg);
void uartInit();
void uartStart();
int logMonitor(const char *data);
extern "C" void uartRcvLoop(void *unused);
#endif