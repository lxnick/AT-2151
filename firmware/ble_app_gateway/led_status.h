#ifndef LED_STATUS_H__
#define LED_STATUS_H__

#include <stdint.h>
#include <stdbool.h>





/* 初始化 LED 與 timer */
void led_status_init(void);


void led_system_on(void);

void led_heartbeat_start(void);
void led_blink_adv(void);
void led_blink_uart(void);

#endif /* LED_STATUS_H__ */
