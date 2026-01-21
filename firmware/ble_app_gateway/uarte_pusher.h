#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void uarte_pusher_init(void);

/* 非阻塞：丟資料進 buffer */
bool uarte_pusher_push(const uint8_t * data, size_t len);

/* 狀態 */
bool uarte_pusher_is_busy(void);
size_t uarte_pusher_bytes_free(void);
