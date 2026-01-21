#ifndef UARTE_TX_HEADER
#define UARTE_TX_HEADER

#include <stdint.h>
#include <stddef.h>

void uarte_tx_init(void);
void uarte_tx_send(const uint8_t * buf, size_t len);

#endif