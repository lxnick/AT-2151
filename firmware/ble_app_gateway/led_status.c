#include "led_status.h"

#include "app_timer.h"
#include "bsp.h"
#include "nrf_log.h"

#include "nrf_delay.h"

#define LED_POWER_ON    0
#define LED_HEARTBEAT   1
#define LED_ADV         2
#define LED_UART        3
#define LED_COUNT       4

#define ADV_BLINK_MS    20
#define UART_BLINK_MS   50
#define HEARTBEAT_MS    1000


/* ================= LED mapping ================= */
#ifdef BOARD_PCA10056
#define LED_HEARTBEAT   BSP_BOARD_LED_0
#define LED_SCANNING    BSP_BOARD_LED_1
#define LED_MATCH       BSP_BOARD_LED_2
#define LED_ERROR       BSP_BOARD_LED_3
#endif

#ifdef BOARD_PCA10040
#define LED_POWER_ON_PIN    12   // P0.12
#define LED_HEARTBEAT_PIN   13   // P0.13
#define LED_ADV_PIN         14   // P0.14
#define LED_UART_PIN        15   // P0.15

#endif

void led_set_onfoff(int i, bool onoff);

static int led_pins[LED_COUNT] = 
{
    LED_POWER_ON_PIN,
    LED_HEARTBEAT_PIN,
    LED_ADV_PIN,
    LED_UART_PIN
};

/* ================= Timers ================= */
APP_TIMER_DEF(m_heartbeat_timer);
APP_TIMER_DEF(m_adv_led_timer);
APP_TIMER_DEF(m_uart_led_timer);

static void led_on(int pin)
{
#ifdef BOARD_PCA10056
    bsp_board_led_on(pin);
#endif
#ifdef BOARD_PCA10040
    nrf_gpio_pin_set(pin);
#endif
}

static void led_off(int pin)
{
#ifdef BOARD_PCA10056
    bsp_board_led_off(pin);
#endif
#ifdef BOARD_PCA10040
    nrf_gpio_pin_clear(pin);
#endif    
}

void led_set_onfoff(int index, bool onoff)
{
    if (index < 0 )
        return ;
    if (index >= LED_COUNT )
        return ;    

    if (onoff) 
        led_on(led_pins[index]);       
    else
        led_off(led_pins[index]);   
}


#if 0
static void led_invert(int pin)
{
#ifdef BOARD_PCA10056
    bsp_board_led_invert(pin);
#endif
#ifdef BOARD_PCA10040
    nrf_gpio_pin_toggle(pin);
#endif   
}
#endif

/* ================= Heartbeat ================= */
static void heartbeat_timer_handler(void * p_context)
{
    static bool on = false;
    on = !on;

    led_set_onfoff(LED_HEARTBEAT, on);
}

static void adv_led_timeout(void * p_context)
{
    led_set_onfoff(LED_ADV, false);
}

static void uart_led_timeout(void * p_context)
{
    led_set_onfoff(LED_UART, false);
}

static void led_blink(uint32_t led, app_timer_id_t timer_id, uint32_t ms)
{
    led_set_onfoff(led, true);

    /* 連續事件時，延長亮燈 */
    app_timer_stop(timer_id);
    app_timer_start(timer_id, APP_TIMER_TICKS(ms), NULL);
}

static void leds_gpio_init(void)
{
    nrf_gpio_cfg_output(LED_POWER_ON_PIN);
    nrf_gpio_cfg_output(LED_HEARTBEAT_PIN);
    nrf_gpio_cfg_output(LED_ADV_PIN);
    nrf_gpio_cfg_output(LED_UART_PIN);

    /* 預設全關 */
    nrf_gpio_pin_clear(LED_POWER_ON_PIN);
    nrf_gpio_pin_clear(LED_HEARTBEAT_PIN);
    nrf_gpio_pin_clear(LED_ADV_PIN);
    nrf_gpio_pin_clear(LED_UART_PIN);
}

/* ================= Public API ================= */

void led_status_init(void)
{
#ifdef BOARD_PCA10056
    bsp_board_init(BSP_INIT_LEDS);
#endif

#ifdef BOARD_PCA10040
    leds_gpio_init();
#endif    

    led_off(LED_POWER_ON_PIN);
    led_off(LED_HEARTBEAT_PIN);
    led_off(LED_ADV_PIN);
    led_off(LED_UART_PIN); 

    APP_ERROR_CHECK(
        app_timer_create(&m_heartbeat_timer,
                         APP_TIMER_MODE_REPEATED,
                         heartbeat_timer_handler));

    APP_ERROR_CHECK(
        app_timer_create(&m_adv_led_timer,
                         APP_TIMER_MODE_SINGLE_SHOT,
                         adv_led_timeout));

    APP_ERROR_CHECK(
        app_timer_create(&m_uart_led_timer,
                         APP_TIMER_MODE_SINGLE_SHOT,
                         uart_led_timeout));
}

/* main loop alive */
void led_heartbeat_start(void)
{
    /* 1 Hz heartbeat */
    app_timer_start(m_heartbeat_timer,  APP_TIMER_TICKS(HEARTBEAT_MS),  NULL);
}

void led_system_on(void)
{
    led_set_onfoff(LED_POWER_ON, true); 
}



void led_blink_adv(void)
{
    led_blink(LED_ADV, m_adv_led_timer, ADV_BLINK_MS);
}

void led_blink_uart(void)
{
    led_blink(LED_UART, m_uart_led_timer, UART_BLINK_MS);
}
