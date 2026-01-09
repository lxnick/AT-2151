/**
  ******************************************************************************************
  * Reference source: lib_ex_rtc.h
  ******************************************************************************************
*/


#ifndef TU_LIB_EX_RTC_H_
#define TU_LIB_EX_RTC_H_

/* Includes --------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_drv_twi.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrfx_gpiote.h"

/* Definition ------------------------------------------------------------*/
//#define RTC_DEBUG_ONE_MIN_INT		/* 1分単位で割り込みを発生させるためのデバッグFlag */

#define RTC_OSC_CLEARED				0x00

#define RTC_ALARM_ENABLE			0x00
#define RTC_ALARM_DISABLE			0x01

//exRTC event timer handler setting.
#define RTC_ALERT_TIME_SEC			0x00
#define RTC_ALERT_TIME_MIN			0x00

//exRTC slave address.
#define RTC_SLAVE_ADD				(0x32)		/* BL5372 Slave Address */

#define OSC_FLAG_MASK				0x10
#define OSC_CLEAR_MASK				0xFF
#define RTC_ALARM_FLAG_OFF_MASK		0x04
#define RTC_ALARM_FLAG_OFF			0x00
#define RTC_ALARM_INT_OFF			0x00
#define RTC_CLOCK_OUT_OFF			0x08		/* 32KHz Clock Output Disable */
#define RTC_12_24_SYSTEMS			0x20		/* 12/24 hour Display System */

#define RTC_AALE_ENABLE				0x80		/* ALARM-A Interrupt Enable Bit */
#ifdef RTC_DEBUG_ONE_MIN_INT
	#define RTC_PERIODIC_INTERRUPT		0x05		/* Priodic inpurrupt cycle select bit(1分で割り込み発生)  */
	//#define RTC_PERIODIC_INTERRUPT		0x04		/* Priodic inpurrupt cycle select bit(1秒で割り込み発生)  */
#else
	#define RTC_PERIODIC_INTERRUPT		0x06		/* Priodic inpurrupt cycle select bit(1時間で割り込み発生) */
#endif
/* 2022.01.26 Add RawData送信中のRTC Interrupt抑止処理追加 ++ */
#define RTC_AALE_DISABLE			0x00
#define RTC_PERIODIC_INT_DISABLE	0x00
/* 2022.01.26 Add RawData送信中のRTC Interrupt抑止処理追加 -- */

#define RTC_INTERNAL_ADDR(x)		( x << 4 )	/* BL5372は上位4bitに内部レジスタの値が入るため4bitシフトさせる */

//exRTC register. (BL5372)
#define RTC_SECONDS					RTC_INTERNAL_ADDR(0x00)		/* Second Counter */
#define RTC_MINUTES					RTC_INTERNAL_ADDR(0x01)		/* Minute Counter */
#define RTC_HOURS					RTC_INTERNAL_ADDR(0x02)		/* Hour Counter */
#define RTC_WEEKDAYS				RTC_INTERNAL_ADDR(0x03)		/* Day of the Week Counter */
#define RTC_DAYS					RTC_INTERNAL_ADDR(0x04)		/* Day Counter */
#define RTC_MONTHS					RTC_INTERNAL_ADDR(0x05)		/* Month Counter */
#define RTC_YEARS					RTC_INTERNAL_ADDR(0x06)		/* Year Counter */
#define RTC_TIME_TRIMMING			RTC_INTERNAL_ADDR(0x07)		/* Time Trimming Register */
#define RTC_ALARMA_MINUTE			RTC_INTERNAL_ADDR(0x08)		/* Alarm A (Minute Register) */
#define RTC_ALARMA_HOUR				RTC_INTERNAL_ADDR(0x09)		/* Alarm A (Hour Register) */
#define RTC_ALARMA_DWEEK			RTC_INTERNAL_ADDR(0x0A)		/* Alarm A (Day of the Week Register) */
#define RTC_ALARMB_MIN				RTC_INTERNAL_ADDR(0x0B)		/* Alarm A (Minute Register) */
#define RTC_ALARMB_HOUR				RTC_INTERNAL_ADDR(0x0C)		/* Alarm A (Hour Register) */
#define RTC_ALARMB_DWEEK			RTC_INTERNAL_ADDR(0x0D)		/* Alarm A (Day of the Week Register) */
#define RTC_CTRL_REG1				RTC_INTERNAL_ADDR(0x0E)		/* Control Register 1 */
#define RTC_CTRL_REG2				RTC_INTERNAL_ADDR(0x0F)		/* Control Register 2 */

#define EX_RTC_RUN_STOP_MASK		0x20
#define EX_RTC_ALARM_MASK			0xc0
#define EX_RTC_ALARM_INT_ENABLE		(1 << 7)

#define I2C_BUFFER_SIZE				17

#define EX_RTC_SEC_GET_MASK			0x7f
#define EX_RTC_MIN_GET_MASK			0x7f
#define EX_RTC_HOUR_GET_MASK		0x3f
#define EX_RTC_DAY_GET_MASK			0x3f
#define EX_RTC_MONTH_GET_MASK		0x1f
#define EX_RTC_YEAR_GET_MASK		0xff

/* 2022.05.18 Add 自動接続対応 ++ */
#define EX_RTC_WAKEUP				0
#define EX_RTC_NON_WAKEUP			1
/* 2022.05.18 Add 自動接続対応 -- */

typedef enum
{
	JAN = 1,
	FEB,
	MAR,
	APR,
	MAY,
	JUN,
	JUL,
	AUG,
	SEP,
	OCT,
	NOV,
	DEC
}MONTH_DATA;

typedef enum
{
	EX_RTC_ON = 0x00,
	EX_RTC_OFF = 0x20,
} EC_RTC_RUN_t;

/* Struct ----------------------------------------------------------------*/
typedef struct _data_time
{
	uint8_t sec;
	uint8_t min;
	uint8_t hour;
	uint8_t week;
	uint8_t day;
	uint8_t month;
	uint8_t year;
} DATE_TIME, *PDATE_TIME;

typedef struct _date_config
{
	uint8_t sec_alarm;
	uint8_t min_alarm;
	uint8_t hour_alarm;
	uint8_t day_alarm;
	uint8_t week_alarm;
} DATE_CONFIG, *PDATE_CONFIG;

/* 2022.05.16 Add 自動接続対応 -- */

#endif

