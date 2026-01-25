#include "led_status.h"

#include "app_timer.h"
#include "bsp.h"
#include "nrf_log.h"

#include "nrf_delay.h"

/* ================= LED mapping ================= */
#ifdef BOARD_PCA10056
#define LED_HEARTBEAT   BSP_BOARD_LED_0
#define LED_SCANNING    BSP_BOARD_LED_1
#define LED_MATCH       BSP_BOARD_LED_2
#define LED_ERROR       BSP_BOARD_LED_3
#endif

#ifdef BOARD_PCA10040
#define LED_HEARTBEAT   12   // P0.12
#define LED_SCANNING    13   // P0.13
#define LED_MATCH       14   // P0.14
#define LED_ERROR       15   // P0.15

#endif

/* ================= Timers ================= */
APP_TIMER_DEF(m_heartbeat_timer);
APP_TIMER_DEF(m_scan_timer);
APP_TIMER_DEF(m_match_timer);

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

    if (on)
        led_on(LED_HEARTBEAT);
    else
        led_off(LED_HEARTBEAT);
}

/* ================= Scan LED ================= */
static void scan_timer_handler(void * p_context)
{
    static bool on = false;
    on = !on;

    if (on)
        led_on(LED_SCANNING);
    else
        led_off(LED_SCANNING);
}

/* ================= Match LED ================= */
static void match_timer_handler(void * p_context)
{
    led_off(LED_MATCH);
}

static void leds_gpio_init(void)
{
    nrf_gpio_cfg_output(LED_HEARTBEAT);
    nrf_gpio_cfg_output(LED_SCANNING);
    nrf_gpio_cfg_output(LED_MATCH);
    nrf_gpio_cfg_output(LED_ERROR);

    /* 預設全關 */
    nrf_gpio_pin_clear(LED_HEARTBEAT);
    nrf_gpio_pin_clear(LED_SCANNING);
    nrf_gpio_pin_clear(LED_MATCH);
    nrf_gpio_pin_clear(LED_ERROR);
}

/* ================= Public API ================= */

void led_test(void)
{
    nrf_delay_ms(1000);
    led_on(LED_HEARTBEAT);    

    nrf_delay_ms(1000);  
    led_on(LED_SCANNING);       

    nrf_delay_ms(1000);  
    led_on(LED_MATCH);        

    nrf_delay_ms(1000);  
    led_on(LED_ERROR);
}

void led_set_onfoff(int index, bool onoff)
{
    int pins[] = {LED_HEARTBEAT,LED_SCANNING,LED_MATCH,LED_ERROR};

    if (index < 0 )
        index = 0;
    if (index > 3 )
        index = 3;     

    if (onoff) 
        led_on(pins[index]);       
    else
        led_off(pins[index]);   
}


void led_status_init(void)
{
#ifdef BOARD_PCA10056
    bsp_board_init(BSP_INIT_LEDS);
#endif

#ifdef BOARD_PCA10040
    leds_gpio_init();
#endif    

    led_off(LED_HEARTBEAT);
    led_off(LED_SCANNING);
    led_off(LED_MATCH);
    led_off(LED_ERROR); 

    APP_ERROR_CHECK(
        app_timer_create(&m_heartbeat_timer,
                         APP_TIMER_MODE_REPEATED,
                         heartbeat_timer_handler));

    APP_ERROR_CHECK(
        app_timer_create(&m_scan_timer,
                         APP_TIMER_MODE_REPEATED,
                         scan_timer_handler));

    APP_ERROR_CHECK(
        app_timer_create(&m_match_timer,
                         APP_TIMER_MODE_SINGLE_SHOT,
                         match_timer_handler));
}

/* main loop alive */
void led_status_heartbeat_start(void)
{
    /* 1 Hz heartbeat */
    app_timer_start(m_heartbeat_timer,
                    APP_TIMER_TICKS(500),
                    NULL);
}

/* scanning started */
void led_status_scan_start(void)
{
    /* slow blink: 1 Hz */
    app_timer_start(m_scan_timer,
                    APP_TIMER_TICKS(500),
                    NULL);
}

/* ADV packet seen */
void led_status_scan_activity(void)
{
    /* temporarily speed up blink */
    app_timer_stop(m_scan_timer);
    app_timer_start(m_scan_timer,
                    APP_TIMER_TICKS(100),
                    NULL);
}

/* target device matched */
void led_status_match(void)
{
    led_on(LED_MATCH);

    app_timer_start(m_match_timer,
                    APP_TIMER_TICKS(200),
                    NULL);
}

/* fatal or scan error */
void led_status_error(void)
{
    led_on(LED_ERROR);
}
