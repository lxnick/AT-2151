#include "uarte_pusher.h"
#include "nrfx_uarte.h"
#include <string.h>

/* ---------- Config ---------- */

#define UARTE_TX_PIN  30   // 依你的板子
#define UARTE_RX_PIN  31

#define UARTE_PUSHER_BUF_SIZE 512

/* ---------- Static ---------- */

static nrfx_uarte_t m_uarte = NRFX_UARTE_INSTANCE(0);

static uint8_t  m_buf[UARTE_PUSHER_BUF_SIZE];
static volatile size_t m_head = 0;
static volatile size_t m_tail = 0;
static volatile bool   m_tx_busy = false;

/* ---------- Helpers ---------- */

static size_t buf_used(void)
{
    if (m_head >= m_tail)
        return m_head - m_tail;
    else
        return UARTE_PUSHER_BUF_SIZE - (m_tail - m_head);
}

static size_t buf_free(void)
{
    return (UARTE_PUSHER_BUF_SIZE - 1) - buf_used();
}

static void kick_tx(void);

/* ---------- IRQ handler ---------- */

static void uarte_evt_handler(nrfx_uarte_event_t const * p_evt, void * p_ctx)
{
    if (p_evt->type == NRFX_UARTE_EVT_TX_DONE)
    {
        m_tx_busy = false;
        kick_tx();
    }
}

/* ---------- Init ---------- */

void uarte_pusher_init(void)
{
    nrfx_uarte_config_t cfg = NRFX_UARTE_DEFAULT_CONFIG;
    cfg.pseltxd = UARTE_TX_PIN;
    cfg.pselrxd = UARTE_RX_PIN;
    cfg.hwfc    = NRF_UARTE_HWFC_DISABLED;
 //   cfg.baudrate = NRF_UARTE_BAUDRATE_115200;
    cfg.baudrate = NRF_UARTE_BAUDRATE_921600;
    cfg.interrupt_priority = 6;

    nrfx_uarte_init(&m_uarte, &cfg, uarte_evt_handler);
}

/* ---------- Core ---------- */

static void kick_tx(void)
{
    if (m_tx_busy)
        return;

    size_t used = buf_used();
    if (used == 0)
        return;

    size_t len;
    if (m_head > m_tail)
        len = m_head - m_tail;
    else
        len = UARTE_PUSHER_BUF_SIZE - m_tail;

    m_tx_busy = true;
    nrfx_uarte_tx(&m_uarte, &m_buf[m_tail], len);

    m_tail = (m_tail + len) % UARTE_PUSHER_BUF_SIZE;
}

bool uarte_pusher_push(const uint8_t * data, size_t len)
{
    if (len > buf_free())
        return false;   // overflow → 丟

    for (size_t i = 0; i < len; i++)
    {
        m_buf[m_head] = data[i];
        m_head = (m_head + 1) % UARTE_PUSHER_BUF_SIZE;
    }

    kick_tx();
    return true;
}

/* ---------- Status ---------- */

bool uarte_pusher_is_busy(void)
{
    return m_tx_busy;
}

size_t uarte_pusher_bytes_free(void)
{
    return buf_free();
}
