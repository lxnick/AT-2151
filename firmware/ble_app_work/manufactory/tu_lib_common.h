/**
  ******************************************************************************************
  * Reference source: lib_common.h
  ******************************************************************************************
*/

#ifndef TU_LIB_COMMON_H_
#define TU_LIB_COMMON_H_

/* Includes --------------------------------------------------------------*/
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "tu_lib_ex_rtc.h"
#include "definition.h"

#ifdef __cplusplus
extern "C"{
#endif

/* Typedef ---------------------------------------------------------------*/
typedef uint8_t ST_ID;
typedef uint8_t EVT_ID;

/* Definition ------------------------------------------------------------*/
/*!! aboslute ON pairing mode*/
#define PAIRING_MODE
/*!! aboslute ON ACK_TIMER_ON*/
#define INIT_3_ACK_TIMER_ON
/*!! aboslute OFF FORCE_PAIRING*/
//#define FORCE_PAIRING

/*!! abolute OFF display daily mode log*/
//#define DIALY_ACC_LOG_ON

//#define EX_RTC_ONE_MIN_INT
//#define BLE_CMD_DEBUG_WRTIE_EVENT 

#define WDT_SLEEP_RUN_HALT_STOP		1
#define WDT_SLEEP_RUN_HALT_RUN		9

#ifdef    WDT_SLEEP_RUN_HALT_STOP
#define WATCH_DOG_BEHAIVOR	WDT_SLEEP_RUN_HALT_STOP
#else     
#define WATCH_DOG_BEHAIVOR	WDT_SLEEP_RUN_HALT_RUN
#endif		

//#define  UICR_TEST_DEBUG
//#define  BLE_CMD_DEBUG_WRTIE_EVENT

//Reset Reason 
#define RESETPIN					(uint32_t)(1 << 0)
#define RESETDOG					(uint32_t)(1 << 1)
#define RESETSREQ					(uint32_t)(1 << 2)
#define RESETLOCKUP					(uint32_t)(1 << 3)
#define RESETSYSOFF					(uint32_t)(1 << 16)
#define RESETSYSOFF_DEBUG			(uint32_t)(1 << 18)
#define RESETPOR_BOR				(uint32_t)(0)
#define RESET_MASK					(RESETPIN | RESETDOG | RESETSREQ | RESETLOCKUP | RESETSYSOFF | RESETSYSOFF_DEBUG | RESETPOR_BOR) 

#define RESET_RESON_REG_CLEAR		(0xFFFFFFFF)
#define BLE_NOTIFY_SIG				(1)
#define BLE_NOTIFY_SIG_NG			(2)
#define BLE_NOTIFY_OFFSET			(0x00)
#define BLE_SET_VALUE_OFFSET		(0x00)
#define BLE_PINCODE_MATCH_SUCCESS	(0)
#define BLE_PINCODE_MATCH_ERROR		(2)
#define WAIT_HFCLK_STABLE_MS		(10)
#define I2C_BUS_CEHCK_ERROR_CODE	(0xff)

#define DEBUG_READ_LOG_RETRY_SWITCH	(1)

/* debug test define */
//#define APP_IT_TEST
//#define UTC_DEBUG_MODE

#ifdef APP_IT_TEST
/* APPとの結合試験 */
#define TEST_MODE_RESULT_ERROR
#define TEST_NOTIFY_ERROR
#define TEST_CNT_UPDATE_RETRY_ERROR
#define TEST_SPI_ERROR
#define TEST_SPI_INIT_ERROR
#define TEST_I2C_ERROR
#define TEST_I2C_INIT_ERROR
#endif

/* debug test mode on/off */
#ifdef  UTC_DEBUG_MODE
/* 単体テスト用 */
#define TEST_BATTERY_TO_ERROR			/* Battery Error Test */
#define TEST_INIT_TO_ERROR				/* Flash Operatrion Error Test */
#define TEST_PLAYER_SET_TO_ERROR		/* Player Setting Error Test */
#define TEST_TIME_SET_TO_ERROR			/* Timer Setting Error Test */
#define TEST_PINCODE_CLEAR_TO_ERROR		/* Pincode Clear Error Test */
#define TEST_PINCODE_WRITE_TO_ERROR		/* Pincode Write Error Test */
#define TEST_AUTHRIZE_ERROR				/* Authrize Error Test */
#define TEST_FLASH_WEITE_ERROR			/* Flash Write Error Test */
#define TEST_WATCHDOG_TIMER				/* WatchDog Timer Test */
/*test stabilization*/
#define TEST_RANDOM_MAC
#endif

/* Offset save UICR REGSITER */
#define UICR_REGISTER				0x10001000
/* UICR REGSITER First address */
#define UICR_NRF_FW0				0x014
#define UICR_ADRESS					(UICR_REGISTER|UICR_NRF_FW0)
#define UICR_INITIAL_VALUE			0x00000000
#define UICR_ACC_X_OFFSET_ADDRESS	27
#define UICR_ACC_Y_OFFSET_ADDRESS	28
#define UICR_ACC_Z_OFFSET_ADDRESS	29
#define WAIT_UICR_ERASE_MS			100
#define WAIT_UICR_ERASE_MARGIN_MS	WAIT_UICR_ERASE_MS * 2
#define WAIT_UICR_WRITE_US			100
#define WAIT_UICR_WRITE_MARGIN_US	WAIT_UICR_WRITE_US * 2

#define LIB_ERR_CHECK LibErrorCheck

#define DCDC_ENABLE		(1)
#define DCDC_DISABLE	(0)

/* Enum ------------------------------------------------------------------*/
/* Error発生時のID */
typedef enum
{
	SDH_SOFTDEV = 0x00,				/* 0x00 SoftDevice */
	BLE_STACK_DEFA,					/* 0x01 SoftDevice BLE Set Config */
	BLE_SOFTDEV,					/* 0x02 SoftDevice BLE Enable */
	DCDC_POWER,						/* 0x03 DCDC Power Init */
	LOG_INIT_ERROR,					/* 0x04 DebugLog Init */
	RAM_RETAIN_ERROR,				/* 0x05 Retention RAM Setting */
	I2C_BUS_ERROR,					/* 0x06 I2C Bus Check Error */
	APP_TIMER_INIT,					/* 0x07 App Timer Init */
	APP_TIMER_CREATE,				/* 0x08 App Timer Create */
	ATFIFO_CREATE,					/* 0x09 ATFIFO Init */
	RESET_REASON,					/* 0x0A Get Reset Reason Error */
	GPIO_ERROR,						/* 0x0B GPIO Error */
	WDT_INIT,						/* 0x0C WatchDog Init */
	WDT_CHANNEL,					/* 0x0D WatchDog Channel Alloc */
	I2C_INIT,						/* 0x0E I2C Init */
	I2C_WRITE,						/* 0x0F I2C Write(No USED) */
	I2C_READ,						/* 0x10 I2C Read */
	GPIOTE_INT_RTC_SET,				/* 0x11 I2C RTC GPIOE Init Error */
	ADV_INIT_ERROR,					/* 0x12 Advertising Init Error */
	GATT_INIT_ERROR,				/* 0x13 GATT Error */
	GATT_SHOES_QW,					/* 0x14 GATT Shoes Sensor Service QW Error */
	GATT_SHOES_UUID,				/* 0x15 GATT Shoes Sensor Service UUID Error */
	GATT_SHOES_SERVICE,				/* 0x16 GATT Shoes Sensor Service Add Error */
	GATT_MODE_UUID,					/* 0x17 GATT Mode Service UUID Error */
	GATT_MODE_SERVICE,				/* 0x18 GATT Mode Service Add Error */
	GATT_DAILY_UUID,				/* 0x19 GATT Daily Service UUID Error */
	GATT_DAILY_SERVICE,				/* 0x1A GATT Daily Service Add Error */
	GATT_RES_SERVICE,				/* 0x1B GATT Response Service Add Error */
	GATT_RES_UUID,					/* 0x1C GATT Response Service UUID Error */
	GATT_BAT_SERVICE,				/* 0x1D GATT Battery Service Add Error */
	GATT_BAT_UUID,					/* 0x1E GATT Battery Service UUID Error */
	GATT_PAIR_SERVICE,				/* 0x1F GATT Pairing Service Add Error */
	GATT_PAIR_UUID,					/* 0x20 GATT Pairing Service UUID Error */
	GATT_UUID_ADD_ERROR,			/* 0x21 GATT UUID Error */
	GATT_CHAR_ADD_ERROR,			/* 0x22 GATT Characteristic Add Error */
	GATT_CHAR_SET_ERROR,			/* 0x23 GATT Characteristic Set Error */
	GAP_PARAM_UPDATE_ERROR,			/* 0x24 GAP Parameter Update Error */
	GAP_BLE_DISCONNECT_ERROR,		/* 0x25 GAP BLE Disconnect Error */
	GAP_PARAM_INIT_ERROR,			/* 0x26 GAP BLE Parameter Init Error */
	GAP_TX_POWER_SET_ERROR,			/* 0x27 GAP BLE Tx Power Set Error */
	GAP_SECS_PARAME_ERROR,			/* 0x28 GAP BLE Sec Parameter Error */
	GAP_EVT_PHY_UPDATE_REQ_ERROR,	/* 0x29 GAP BLE Phy Update Request Error */
	ADC_INIT,						/* 0x2A ADC Init Error */
	ADC_START,						/* 0x2B ADC Start Error */
	SLEEP_ERROR,					/* 0x2C Sleep Error */
	FORCE_SLEEP_ERROR,				/* 0x2D Force Sleep Error */
	FLASH_INIT_ERROR,				/* 0x2E Flash Init Error */
	FLASH_UNINIT_ERROR,				/* 0x2F Flash Uninit Error */
	FLASH_READ_ERROR,				/* 0x30 Flash Read Error */
	FLASH_WRITE_ERROR,				/* 0x31 Flash Write Error */
	FLASH_ERASE_ERROR,				/* 0x32 Flash Erase Error */
	SYSTEM_OFF_ERROR,				/* 0x33 System OFF Error */
	GAP_COMMON_OPT_CONN_EVT_EXT,	/* 0x34 BLE COMMON OPT CONN EVT EXT Set Error */
	GATT_MTU_SIZE_UPD_ERROR,		/* 0x35 MTU Size Update Error */
	ANGLE_FLASH_INIT_ERR,			/* 0x36 Angle Adjust Flash Init Error */
	ANGLE_FLASH_READ_ERR,			/* 0x37 Angle Adjust Flash Read Error */
	ANGLE_FLASH_WRITE_ERR,			/* 0x37 Angle Adjust Flash Write Error */
	ANGLE_FLASH_ERASE_ERR,			/* 0x37 Angle Adjust Flash Erase Error */
	ANGLE_FLASH_UNINIT_ERR,			/* 0x38 Angle Adjust Flash Uninit Error */
} ERR_PLACE;


/* BLE response err_code */
typedef enum
{
	UTC_BLE_SUCCESS = 0x00,
	UTC_BLE_TIME_UPDATE_ERROR,		//0x01
	UTC_BLE_ULT_SEND_RESULT_ERROR,	//0x02
	UTC_BLE_INIT_CMD_ERROR,			//0x03
	UTC_BLE_ULT_SEAQUENCE_ERROR,	//0x04
	UTC_BLE_FW_INIT_ERROR,			//0x05
	UTC_BLE_INITIAL_ERROR,			//0x06
	UTC_BLE_FLASH_WRITE_ERROR,		//0x07
	UTC_BLE_CON_INTERVAL_ERROR,		//0x08
	UTC_BLE_ULT_TIME_ERROR,			//0x09
	UTC_BLE_SPI_ERROR,				//0x0A
	UTC_BLE_SPI_INIT_ERROR,			//0x0B
	UTC_BLE_FW_RESPONSE_ERROR,		//0x0C
	UTC_BLE_I2C_ERROR,				//0x0D
	UTC_BLE_I2C_INIT_ERROR,			//0x0E
	UTC_BLE_AUTHORIZE_ERROR,		//0x0F
	UTC_BLE_GET_BAT_ERROR,
	UTC_BLE_PIN_CODE_ERROR,
} UTC_BLE_ERR_CODE;

typedef enum
{
	UTC_SUCCESS = 0x00,
	UTC_ERROR,
	UTC_I2C_INIT_ERROR,
	UTC_I2C_ERROR,
	UTC_SPI_INIT_ERROR,
	UTC_SPI_ERROR,
	UTC_THROUGH_ERROR
} UTC_SHOES_RET_CODE;

typedef enum
{
	SPI_ACC_RESET = 0x00,
	SPI_ACC_SET_UP,
	SPI_ACC_DEEP_SLEEP
} SPI_ACC_TRACE_ID;

/* Trace ID */
typedef enum
{
	TR_SOFTDEVICE_INIT_CMPL = 0x100,
	TR_SOFTDEVICE_BLE_INIT_CMPL,
	TR_I2C_BUS_RECOVERY_HAPPEN,
	TR_I2C_BUS_RECOVERY,
	TR_RAM_SIGN_ERROR,
	TR_RTC_TIME_SET_FROM_RAM,
	TR_RTC_TIME_SET_FROM_FLASH,
	TR_WDT_START,
	TR_ACC_RESET_CMPL,
	TR_ACC_RESET_FAIL,
	TR_ACC_READ_INT_INIT_ERROR,
	TR_ACC_READ_INT_ERROR,
	TR_RTC_RESET_CMPL,
	TR_RTC_RESET_FAIL,
	TR_ACC_INIT_CMPL,
	TR_ACC_INIT_FAIL,
	TR_RTC_INIT_CMPL,
	TR_RTC_INIT_FAIL,
	TR_ACC_3G_INIT_CMPL,
	TR_ACC_3G_INIT_FAIL,
	TR_ACC_3G_EVT_HANDLE_SPI_INIT_FAIL,
	TR_ADV_INIT_CMPL,
	TR_ADV_START,
	TR_ADV_FAILED,
	TR_ADV_TIMEOUT,
	TR_CONNECT_CMPL,
	TR_DISCONNECT_CMPL,
	TR_DEEP_SLEEP_ENTER,
	TR_BLE_INIT_CMD_CMPL,
	TR_BLE_SET_TIME_CMPL,
	TR_BLE_SET_PLAYER_INFO_CMPL,
	TR_BLE_DEVICE_NAME_SET_CMPL,
	TR_BLE_PLAYER_INOF_SERVICE_CREATE_CMPL,
	TR_BLE_MODE_SERVICE_CREATE_CMPL,
	TR_BLE_DAILY_SERVICE_CREATE_CMPL,
	TR_BLE_RESPONSE_SERVICE_CREATE_CMPL,
	TR_BLE_BATTERY_SERVICE_CREATE_CMPL,
	TR_BLE_PAIRING_CODE_SERVICE_CREATE_CMPL,
	TR_FLASH_OPERATION_MODE_START,
	TR_FLASH_OPERATION_ERASE_FAILED,
	TR_FLASH_OPERATION_WRITE_FAILED,
	TR_EVENT_FIFO_ERROR,
	TR_ACC_FIFO_ERROR,
	TR_WAIT_DISONNECT_STATE_RESET,
	TR_FORCR_DISCONNECT_GENERATE,
	TR_CONNECTION_PARAM_UPDATE_API_FAILED,
	TR_CONNECTION_PARAM_UPDATE_FAILED_SLAVE_LATENCY_ZERO,
	TR_CONNECTION_PARAM_UPDATE_FAILED_SLAVE_LATENCY_ONE,
	TR_DAILY_LOG_READ_CONTINUE_SEND_ERROR,
	TR_DAILY_LOG_READ_CONTINUE_SEND_END_ERROR,
	TR_DAILY_LOG_READ_SINGLE_SEND_ERROR,
	TR_DAILY_LOG_READ_SINGLE_SEND_END_ERROR,
	TR_SPI_ERROR,
	TR_SPI_INIT_ERROR,
	TR_I2C_ERROR,
	TR_I2C_INIT_ERROR,
	TR_I2C_GET_TIME_ERROR,
	TR_I2C_EVT_INT_INIT_ERROR,
	TR_RTC_OSC_FLAG_ERROR,
	TR_TIMER_ERROR,
	TR_GET_BAT_ERROR,
	TR_FLASH_OP_ERROR,
	TR_RTC_SET_TIME_ERROR,
	TR_RTC_CNT_INT_SET_TIME_ERROR,
	TR_RTC_CNT_SET_TIME_ERROR,
	TR_RTC_CNT_INIT_SET_TIME_ERROR,
	TR_NOTIFY_ERROR,
	TR_AUTHRIZE_ERROR,
	TR_SET_DAILY_LOG_WRITE_ERROR, 
	
	/* UICR */
	TR_UICR_WAIT_ERROR,
	TR_UICR_SENSOR_VALID_ERROR,
	TR_OFFSET_P_TO_P_ERROR,
	/* 2020.11.10 Add ACCモード設定時のエラー ++ */
	TR_ACC_SET_MODE_FAILED,
	TR_ACC_GET_MODE_FAILED,
	/* 2020.11.10 Add ACCモード設定時のエラー -- */
	
	/* 2022.01.11 Add RTC Default設定時のエラー ++ */
	TR_RTC_SET_DEFTIME_ERROR,
	/* 2022.01.11 Add RTC Default設定時のエラー -- */
	/* 2022.01.26 Add RTC Disable Interrupt 設定時のエラー ++ */
	TR_RTC_ENABLE_INT_ERROR,
	TR_RTC_DISABLE_INT_ERROR,
	/* 2022.01.26 Add RTC Disable Interrupt 設定時のエラー -- */
	/* 2020.12.23 Add RSSI取得テスト ++ */
	TR_BLE_RSSI_NOTIFY_COMP,
	TR_BLE_RSSI_NOTIFY_FAILED,
	/* 2020.12.23 Add RSSI取得テスト -- */

	TR_OTHER_ERROR_HEADER = 0xE000,
	
	TR_HARD_FAULT = 0xF000,
	
	TR_DUMMY_ID = 0xFFFF
}TRACE_LOG_ID;


#ifdef __cplusplus
}
#endif

#endif
