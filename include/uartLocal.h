#ifndef UART_LOCAL
#define UART_LOCAL
void uartLoop(void *arg);
void uartInit();
void uartStart();
//int logMonitor(const char *data);
int uartSend (const char* fmt,... );
extern "C" void uartRcvLoop(void *unused);
//int my_message (const char* fmt,... );
#endif