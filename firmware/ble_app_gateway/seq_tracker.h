#ifndef SEQUENCE_TRACKER_HEADER__
#define SEQUENCE_TRACKER_HEADER__

#include <stdint.h>
#include <stdbool.h>

/* 回傳 true = 新資料，false = 重複 / 舊資料 */
bool seq_tracker_accept(uint16_t device_id, uint8_t seq);

/* optional */
void seq_tracker_reset(void);

#endif