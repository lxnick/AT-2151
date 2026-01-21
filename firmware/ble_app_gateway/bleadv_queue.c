#include "bleadv_queue.h"
#include <string.h>

static bleadv_packet_t m_queue[ADV_QUEUE_SIZE];
static volatile uint8_t m_head = 0;
static volatile uint8_t m_tail = 0;
static volatile uint8_t m_count = 0;

void bleadv_queue_init(void)
{
    m_head = 0;
    m_tail = 0;
    m_count = 0;
}

bool bleadv_queue_is_full(void)
{
    return (m_count >= ADV_QUEUE_SIZE);
}

bool bleadv_queue_is_empty(void)
{
    return (m_count == 0);
}

bool bleadv_queue_push(const bleadv_packet_t * pkt)
{
    if (bleadv_queue_is_full())
    {
        return false;  
    }

    memcpy(&m_queue[m_head], pkt, sizeof(bleadv_packet_t));

    m_head = (m_head + 1) % ADV_QUEUE_SIZE;
    m_count++;

    return true;
}

bool bleadv_queue_pop(bleadv_packet_t * pkt)
{
    if (bleadv_queue_is_empty())
    {
        return false;
    }

    memcpy(pkt, &m_queue[m_tail], sizeof(bleadv_packet_t));

    m_tail = (m_tail + 1) % ADV_QUEUE_SIZE;
    m_count--;

    return true;
}