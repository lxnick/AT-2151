#ifndef LED_STATUS_H__
#define LED_STATUS_H__

#include <stdint.h>
#include <stdbool.h>

/* 初始化 LED 與 timer */
void led_status_init(void);

/* Heartbeat（main loop alive） */
void led_status_heartbeat_start(void);

/* BLE scan 狀態 */
void led_status_scan_start(void);
void led_status_scan_activity(void);

/* 目標裝置 match */
void led_status_match(void);

/* Error */
void led_status_error(void);

void led_test(void);

void led_set_onfoff(int i, bool onoff);

#endif /* LED_STATUS_H__ */
