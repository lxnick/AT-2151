#ifndef BLE_ADV_QUEUE_HEADER__
#define BLE_ADV_QUEUE_HEADER__

#include <stdbool.h>

#include "bleadv_packet.h"

#define ADV_QUEUE_SIZE 16   // 可依需求調整（2 的次方最好）

void bleadv_queue_init(void);

/* ISR / scan callback 使用 */
bool bleadv_queue_push(const bleadv_packet_t * pkt);

/* main loop 使用 */
bool bleadv_queue_pop(bleadv_packet_t * pkt);

bool bleadv_queue_is_empty(void);
bool bleadv_queue_is_full(void);

#endif