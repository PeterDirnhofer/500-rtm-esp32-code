#ifndef UART
#define UART
void uartLoop(void *arg);
void uartInit();
void uartStart();
int uartSendData(const char *data);
void uartRcvLoop(void *unused);





#endif