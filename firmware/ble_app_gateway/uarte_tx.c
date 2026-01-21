#include "uarte_tx.h"
#include "nrfx_uarte.h"

#define UARTE_TX_PIN  30   // 依你的板子
#define UARTE_RX_PIN  31

static nrfx_uarte_t m_uarte = NRFX_UARTE_INSTANCE(0);

void uarte_tx_init(void)
{
    nrfx_uarte_config_t cfg = NRFX_UARTE_DEFAULT_CONFIG;

    cfg.pseltxd = UARTE_TX_PIN;
    cfg.pselrxd = UARTE_RX_PIN;
    cfg.hwfc    = NRF_UARTE_HWFC_DISABLED;
    cfg.baudrate = NRF_UARTE_BAUDRATE_115200;
    cfg.interrupt_priority = 6;

    nrfx_uarte_init(&m_uarte, &cfg, NULL);
}

void uarte_tx_send(const uint8_t * buf, size_t len)
{
    nrfx_uarte_tx(&m_uarte, buf, len);
    while (nrfx_uarte_tx_in_progress(&m_uarte))
        ;
}
