#ifndef SPI
#define SPI

void my_post_setup_cb(spi_slave_transaction_t *trans);
void my_post_trans_cb(spi_slave_transaction_t *trans);

void vspiDacInit();
void vspiDacStart();
void vspiDacLoop(void *);

uint16_t endianSwap(uint16_t);
void vspiSendDac(uint16_t, uint16_t *, spi_device_handle_t);

esp_err_t spiSend(spi_host_device_t, spi_slave_transaction_t *, TickType_t, spi_slave_transaction_t *);
void spiSendEmpty(spi_host_device_t, TickType_t, uint8_t *);

#endif
