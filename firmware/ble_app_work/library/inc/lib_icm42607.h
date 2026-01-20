/**
  ******************************************************************************************
  * @file    lib_icm42607.h
  * @author  k.tashiro
  * @version 1.0
  * @date    2022/03/04
  * @brief   ICM42607 Control ACC/Gyro
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2022/03/04       k.tashiro         create new
  ******************************************************************************************
*/

#ifndef LIB_ICM42607_H_
#define LIB_ICM42607_H_

/* Includes --------------------------------------------------------------*/
#include "nrf_gpio.h"
#include "nrfx_gpiote.h"

//#include "state_control.h"
#include "lib_spi_function.h"
//#include "definition.h"
#include "lib_common.h"

/* Definition ------------------------------------------------------------*/
#define ICM42607_DEVICE_ID					(0x68)			/* LSM6DSOW Device ID */

/* Register Address */
#define ICM42607_MCLK_RDY					(0x00)			/* MCLK_RDY (OTP load is completed) */
#define ICM42607_DEVICE_CONFIG				(0x01)			/* SPI Mode Setting */
#define ICM42607_SIGNAL_PATH_RESET			(0x02)			/* Signal Path Reset(Software Reset and FIFO Flush) */
#define ICM42607_DRIVE_CONFIG1				(0x03)			/* I3C Slew Rate (Unused) */
#define ICM42607_DRIVE_CONFIG2				(0x04)			/* I2C Slew Rate (Unused) */
#define ICM42607_DRIVE_CONFIG3				(0x05)			/* SPI Slew Rate */

#define ICM42607_INT_CONFIG					(0x06)			/* Interrupt Config */

#define ICM42607_TEMP_DATA1					(0x09)			/* Temp Data 1 (上位8bit) */
#define ICM42607_TEMP_DATA0					(0x0A)			/* Temp Data 0 (下位8bit) */

#define ICM42607_ACCEL_DATA_X1				(0x0B)			/* Upper byte of Accel X-axis data */
#define ICM42607_ACCEL_DATA_X0				(0x0C)			/* Lower byte of Accel X-axis data */
#define ICM42607_ACCEL_DATA_Y1				(0x0D)			/* Upper byte of Accel Y-axis data */
#define ICM42607_ACCEL_DATA_Y0				(0x0E)			/* Lower byte of Accel Y-axis data */
#define ICM42607_ACCEL_DATA_Z1				(0x0F)			/* Upper byte of Accel Z-axis data */
#define ICM42607_ACCEL_DATA_Z0				(0x10)			/* Lower byte of Accel Z-axis data */
#define ICM42607_GYRO_DATA_X1				(0x11)			/* Upper byte of Gyro X-axis data */
#define ICM42607_GYRO_DATA_X0				(0x12)			/* Lower byte of Gyro X-axis data */
#define ICM42607_GYRO_DATA_Y1				(0x13)			/* Upper byte of Gyro Y-axis data */
#define ICM42607_GYRO_DATA_Y0				(0x14)			/* Lower byte of Gyro Y-axis data */
#define ICM42607_GYRO_DATA_Z1				(0x15)			/* Upper byte of Gyro Z-axis data */
#define ICM42607_GYRO_DATA_Z0				(0x16)			/* Lower byte of Gyro Z-axis data */

#define ICM42607_TMST_FSYNCH				(0x17)			/* Stores the upper byte of the time delta (Fsync) */
#define ICM42607_TMST_FSYNCL				(0x18)			/* Stores the lower byte of the time delta (Fsync) */

#define ICM42607_PWR_MGMT0					(0x1F)			/* Power Management */
#define ICM42607_GYRO_CONFIG0				(0x20)			/* Gyroscope Config 0 */
#define ICM42607_ACC_CONFIG0				(0x21)			/* Accelerometer Config */
#define ICM42607_TEMP_CONFIG0				(0x22)			/* Temperature Config */

#define ICM42607_GYRO_CONFIG1				(0x23)			/* Gyroscope Config 1. Selects GYRO UI low pass filter bandwidth */
#define ICM42607_ACC_CONFIG1				(0x24)			/* Accelerometer Config */

#define ICM42607_APEX_CONFIG1				(0x26)			/* APEX Config */
#define ICM42607_WOM_CONFIG					(0x27)			/* Wake on Motion */
#define ICM42607_FIFO_CONFIG1				(0x28)			/* FIFO Config */
#define ICM42607_FIFO_CONFIG2				(0x29)			/* Lower bits of FIFO watermark */
#define ICM42607_FIFO_CONFIG3				(0x2A)			/* Upper bits of FIFO watermark */

#define ICM42607_INT_SOURCE0				(0x2B)			/* Interrupt Source 0 */
#define ICM42607_INT_SOURCE1				(0x2C)			/* Interrupt Source 1 */
#define ICM42607_INT_SOURCE3				(0x2D)			/* Interrupt Source 3 */
#define ICM42607_INT_SOURCE4				(0x2E)			/* Interrupt Source 4 */

#define ICM42607_FIFO_LOST_PKT0				(0x2F)			/* Low byte, number of packets lost in the FIFO */
#define ICM42607_FIFO_LOST_PKT1				(0x30)			/* High byte, number of packets lost in the FIFO */

#define ICM42607_INTF_CONFIG0				(0x35)			/* FIFO Interface packet format */
#define ICM42607_INT_STATUS_DRDY			(0x39)			/* Data Ready */

#define ICM42607_INT_STATUS					(0x3A)			/* Interrupt Status Register */
#define ICM42607_INT_STATUS2				(0x3B)			/* Interrupt Status Register 2 (WOM) */
#define ICM42607_INT_STATUS3				(0x3C)			/* Interrupt Status Register 3 */

#define ICM42607_FIFO_COUNTH				(0x3D)			/* High Bits, count indicates the number of records or bytes available in FIFO */
#define ICM42607_FIFO_COUNTL				(0x3E)			/* Low Bits, count indicates the number of records or bytes available in FIFO */
#define ICM42607_FIFO_DATA					(0x3F)			/* FIFO data port */

#define ICM42607_WHO_AM_I					(0x75)			/* Who am i */
/* Bank Control Register */
#define ICM42607_BLK_SEL_W					(0x79)			/* Block address for accessing MREG1 or MREG2 register space */
#define ICM42607_MADDR_W					(0x7A)			/* To write to a register in MREG1 or MREG2 space */
#define ICM42607_M_W						(0x7B)			/* value must be written to M_W */
#define ICM42607_BLK_SEL_R					(0x7C)			/* Block address for accessing MREG1 or MREG2 register space for register read operation */
#define ICM42607_MADDR_R					(0x7D)			/* set this register field to the address of the register in MREG1 or MREG2 space */
#define ICM42607_M_R						(0x7E)			/* that value is accessed from M_R. */

#define ICM42607_MREG1_TMST_CONFIG1			(0x00)			/* Timestamp Config (MREG1) */
#define ICM42607_MREG1_FIFO_CONFIG5			(0x01)			/* FIFO Config (MREG1) */
#define ICM42607_MREG1_INT_CONFIG0			(0x04)			/* FIFO Interrupt Clear Setting (MREG1) */
#define ICM42607_MREG1_SENSOR_CONFIG3		(0x06)			/* FIFO Interrupt Clear Setting (MREG1) */
#define ICM42607_MREG1_OTP_CONFIG			(0x2B)			/* OTP Config (MREG1) */
#define ICM42607_MREG1_ACCEL_WOM_X_THR		(0x4B)			/* Threshold value for the Wake on Motion Interrupt for X-axis accelerometer(MREG1) */
#define ICM42607_MREG1_ACCEL_WOM_Y_THR		(0x4C)			/* Threshold value for the Wake on Motion Interrupt for Y-axis accelerometer(MREG1) */
#define ICM42607_MREG1_ACCEL_WOM_Z_THR		(0x4D)			/* Threshold value for the Wake on Motion Interrupt for Z-axis accelerometer(MREG1) */

#define ICM42607_MREG2_OTP_CTRL7			(0x06)			/* OTP_CTRL7 (MREG2) */

#define ICM42607_MREG1						(0x00)			/* MREG1 */
#define ICM42607_MREG2						(0x28)			/* MREG2 */
#define ICM42607_MREG3						(0x50)			/* MREG3 */

#define MCLK_RDY_ENABLE						(0x08)			/* Indicates internal clock is currently */
#define MCLK_RDY_ON							(1)				/* Indicates internal clock is currently running */
#define MCLK_RDY_OFF						(0)				/* Indicates internal clock is currently not running */

#define ACC_GYRO_ODR_MASK					(0x0F)			/* ODR Setting Mask */
#define ACC_GYRO_ODR_POS					(4)				/* ODR Setting Position */

#define ACC_GYRO_FSS_MASK					(0xC0)			/* full-scale selection Setting Mask */
#define ACC_GYRO_FSS_POS					(5)				/* full-scale selection Setting Position */

#define ACC_GYRO_ODR_FSS_MASK				(0xFC)			/* ODR and full-scale selection Setting Mask */

#define ACC_GYRO_INT_MASK					(0xFF)			/* Interrupt Register Mask */

#define ACC_GYRO_FIFO_CONFIG2_WTM_MASK		(0x00FF)		/* FIFO_CONFIG2 Reg Watermark Mask */
#define ACC_GYRO_FIFO_CONFIG3_WTM_MASK		(0x0F00)		/* FIFO_CONFIG3 Reg Watermark Mask */

#define ACC_GYRO_FIFO_FORMAT_MASK			(0x70)			/* INTF_CONFIG0: Endian format setting */
#define ACC_GYRO_FIFO_FORMAT_VAL			(0x00)			/* INTF_CONFIG0: Little Endian */
#define ACC_GYRO_FIFO_COUNT_FORMAT			(0x00)			/* INTF_CONFIG0: FIFO Count is reported in byte */

#define ACC_GYRO_WAKEUP_THS_MASK			(0x7F)			/* WAKE_UP_THS Threshold Mask */
#define ACC_GYRO_WAKEUP_DUR_MASK			(0x60)			/* WAKE_UP_DUR Duration Mask */
#define ACC_GYRO_WAKEUP_DUR_POS				(5)				/* WAKE_UP_DUR Duration Position */

#define ACC_FS_16G							(2048.0f)		/* full-scale 16g gain */
#define GYRO_FS_2000DPS						(16.4)
#define ACC_MG_UNIT							(1000.0f)		/* mg変換用 */
#define GYRO_UNIT							(10.0)			/* 送信データ作成用 */

#define ACC_MODE_MASK						(0x10)			/* ACC High-performance Enable/Disable */
#define ACC_HM_MODE_POS						(4)				/* ACC High-performance Bit Position  */
#define GYRO_MODE_MASK						(0x80)			/* Gyro High-performance Enable/Disable */
#define GYRO_HM_MODE_POS					(7)				/* Gyro High-performance Bit Position  */

#define ACC_USR_OFF_W_MASK					(0x08)			/* Weight of XL user offset bits of registers */
#define ACC_USR_OFF_W_POS					(3)				/* Weight of XL user offset bits Position  */
#define ACC_USR_OFF_W_ENABLE				(0x01)			/* Weight of XL user offset bits Enable */

#define ENABLE_BASIC_INTERRUTS_MASK			(0x80)			/* Enable basic interrupts */
#define INACT_EN_LOWPOWER					(0x60)			/* Accelerometer ODR to 12.5 Hz gyro power down */

#define ACC_GYRO_FIFO_PACKET_SIZE			(16)			/* FIFO Packet Size */
#define ACC_GYRO_WTM_COUNT					ACC_GYRO_FIFO_PACKET_SIZE * 4				/* FIFO watermark threshold */

#define ACC_GYRO_SETEUP_ERROR				(0xFF)			/* Acc/Gyro Setup Error */

#define WOM_THRESHOLD_MASK					(0xFF)			/* Wake On Motion Threshold Mask */
#define WOM_X_AXIS_THR						(0xFF)			/* X-AxisのWake On Motionしきい値(996.1mg) Resolution(1g/256) [255 / 256 = 0.99609] */
#define WOM_Y_AXIS_THR						(0xFF)			/* Y-AxisのWake On Motionしきい値(996.1mg) Resolution(1g/256) [255 / 256 = 0.99609] */
#define WOM_Z_AXIS_THR						(0xFF)			/* Z-AxisのWake On Motionしきい値(996.1mg) Resolution(1g/256) [255 / 256 = 0.99609] */

#define ACC_GYRO_TAG_MASK					(0xF8)			/* Output TAG Data Mask */

#define ACC_GYRO_WOM_X_INT					(0x04)			/* WOM X-Axis Detect */
#define ACC_GYRO_WOM_Y_INT					(0x02)			/* WOM Y-Axis Detect */
#define ACC_GYRO_WOM_Z_INT					(0x01)			/* WOM Z-Axis Detect */

#define ACC_GYRO_MODE_MASK					(0x0F)			/* Power Mode Mask */
#define ACC_GYRO_IDLE_MASK					(0x10)			/* Power Mode IDLE Mask */

#define ACC_GYRO_OFFSET_MASK				(0xFF)			/* OFS_USR Mask */
#define ACC_X_AXIS_OFFSET_VAL				(0x00)			/* Accelerometer X-axis user offset value */
#define ACC_Y_AXIS_OFFSET_VAL				(0xA0)			/* Accelerometer Y-axis user offset value (2022.01.20 Add) */
#define ACC_Z_AXIS_OFFSET_VAL				(0xC0)			/* Accelerometer Z-axis user offset value */

#define WAKEUP_X_AXIS						(1)				/* Wake Up X-Axis */
#define WAKEUP_Y_AXIS						(1)				/* Wake Up Y-Axis */
#define WAKEUP_Z_AXIS						(1)				/* Wake Up Z-Axis */
#define NON_WAKEUP							(0)				/* Wake Up Src None */

#define ACC_DATA_COUNT						(1)				/* ACC Only Mode時は ACCデータのみ */
#define GYRO_DATA_COUNT						(1)				/* Gyro/Temp Mode時は Gyroデータのみ */
#define ACC_GYRO_DATA_COUNT					(2)				/* ACC/Gyro/Temp Mode時は ACC/Gyroデータ */
#define TEMP_DISABLE						(0)				/* 温度データなし */
#define TEMP_ENABLE							(1)				/* 温度データあり */

#define PWR_MGMT_ACC_MODE_POS				(0)				/* Accel Mode Position */
#define PWR_MGMT_GYRO_MODE_POS				(2)				/* Gyro Mode Position */

#define TMST_CONFIG1_MASK					(0x0F)			/* TMST_CONFIG1: 3;TMST_RES, 0:TMST_EN mask */
#define TMST_CONFIG1_RESOLUTION				(0x08)			/* Timestamp Resolution */
#define TMST_CONFIG1_RESOLUTION_1US			(0x00)			/* Timestamp Resolution (1us) */
#define TMST_CONFIG1_FSYNC_ENABLE			(0x02)			/* Timestamp Fsync Enable */
#define TMST_CONFIG1_ENABLE					(0x01)			/* Timestamp Enable */
#define TMST_CONFIG1_DISABLE				(0x00)			/* Timestamp Disable */

#define FIFO_CONFIG5_MASK					(0x07)			/* FIFO_CONFIG5 Mask(FIFO_TMST_FSYNC_EN:3 FIFO_GYRO_EN:1 FIFO_ACCEL_EN:0) */
#define FIFO_CONFIG5_TMST_FSYNC_POS			(2)				/* FIFO_TMST_FSYNC_EN Position */
#define FIFO_CONFIG5_GYRO_EN_POS			(1)				/* FIFO_GYRO_EN Position */
#define FIFO_CONFIG5_ACC_EN_POS				(0)				/* FIFO_ACCEL_EN Position */

#define FIFO_CONFIG1_BYPASS_MASK			(0x01)			/* FIFO Bypass Control */

#define INT1_CONF_MODE						(0x00)			/* INT1_MODE 1:Latched mode */
#define INT1_CONF_POLARITY					(0x01)			/* INT1_POLARITY 1:Active high */
#define INT2_CONF_MODE						(0x20)			/* INT2_MODE 1:Latched mode */
#define INT2_CONF_POLARITY					(0x08)			/* INT2_POLARITY 1:Active high */
//#define INT_CONFIG_VALUE					INT1_CONF_MODE | INT1_CONF_POLARITY | INT2_CONF_MODE | INT2_CONF_POLARITY
#define INT_CONFIG_VALUE					INT1_CONF_MODE | INT1_CONF_POLARITY | 0x02

#define FIFO_THS_INT_MASK					(0x0C)			/* FIFO Threshold Interrupt Clear Option (latched mode) */
#define FIFO_THS_INT_VAL					(0x03)			/* 11: Clear on Status Bit Read OR on FIFO data 1 byte read */
#define FIFO_THS_INT_POS					(2)				/* FIFO_THS_INT_CLEAR Position */

#define WOM_MODE_ABSOLUTE					(0x00)			/* WOM Absolute Mode */
#define WOM_MODE_DIFFERENTIAL				(0x02)			/* WOM Differential Mode */
#define WOM_MODE_ENABLE						(0x01)			/* WOM Mode Enable */
#define WOM_MODE_DISABLE					(0x00)			/* WOM Mode Disable */
#define WOM_CONFIG_MASK						(0x03)			/* WOM_MODE and WOM_EN */

#define ACCEL_CONFIOG_MASK					(0x77)
#define ACCEL_UI_AVG						(0x00)			/* ACCEL_UI_AVG[6:4] 000: 2x average */
#define ACCEL_UI_FILT_BW					(0x01)			/* ACCEL_UI_AVG[2:0] 000: Low pass filter bypassed */
#define ACCEL_CONFIOG_VAL					ACCEL_UI_AVG | ACCEL_UI_FILT_BW

#define ACC_GYRO_SW_RESET_VAL				(0x10)			/* Software reset Mask Default value: 0 */
#define ACC_GYRO_SW_RESET_MASK				ACC_GYRO_SW_RESET_VAL	/* Software Reset Mask */
#define ACC_GYRO_FIFO_FLUSH					(0x04)			/* FIFO Flush */
#define ACC_GYRO_FIFO_FLUSH_MASK			ACC_GYRO_FIFO_FLUSH

#define GYRO_INVALID_DATA					((int16_t)0x8000)		/* Gyro Invalid Data */

#define OTP_CONFIG_MASK						(0x0C)			/* OTP Config Mask */
#define OTP_CTRL7_MASK						(0x0A)			/* OTP CTRL7 Mask */

#define OTP_CONFIG_VALUE					(0x06)			/* OTP Config Value */
#define OTP_CTRL7_SETTING_VAL				(0x04)			/* OTP_CTRL7 Setting Value */
#define OTP_CTRL7_RESET_VAL					(0x0C)			/* OTP_CTRL7 Reset Value */

/* Enum ------------------------------------------------------------------*/
/* ACC/Gyro ODR List(ODR_XL[3:0]/ODR_G[3:0]) */
typedef enum
{
	ACC_ODR_NONE			=	0x00,			/* ACC ODR None */
	ACC_ODR_1_6KHZ			=	0x05,			/* 1.6kHz(LN mode) */
	ACC_ODR_800HZ			=	0x06,			/* 800Hz(LN mode) */
	ACC_ODR_400HZ			=	0x07,			/* 400Hz(LP or LN mode) */
	ACC_ODR_200HZ			=	0x08,			/* 200Hz(LP or LN mode) */
	ACC_ODR_100HZ			=	0x09,			/* 100Hz(LP or LN mode) */
	ACC_ODR_50HZ			=	0x0A,			/* 50Hz(LP or LN mode) */
	ACC_ODR_25HZ			=	0x0B,			/* 25Hz(LP or LN mode) */
	ACC_ODR_12_5HZ			=	0x0C,			/* 12.5Hz(LP or LN mode) */
	ACC_ODR_6_25HZ			=	0x0D,			/* 6.25Hz(LP mode) */
	ACC_ODR_3_125HZ			=	0x0E,			/* 3.125Hz(LP mode) */
	ACC_ODR_1_5625HZ		=	0x0F,			/* 1.5625Hz(LP mode) */
} ACC_ODR;

typedef enum
{
	GYRO_ODR_NONE			=	0x00,			/* Gyro ODR None */
	GYRO_ODR_1_6_KHZ		=	0x05,			/* 1.6KHz */
	GYRO_ODR_800HZ			=	0x06,			/* 800Hz */
	GYRO_ODR_400HZ			=	0x07,			/* 400Hz */
	GYRO_ODR_200HZ			=	0x08,			/* 200Hz */
	GYRO_ODR_100HZ			=	0x09,			/* 100Hz */
	GYRO_ODR_50HZ			=	0x0A,			/* 50Hz */
	GYRO_ODR_25HZ			=	0x0B,			/* 25Hz */
	GYRO_ODR_12_5HZ			=	0x0C,			/* 12.5Hz */
} GYRO_ODR;

/* Accelerometer full-scale selection(FS[1:0]_XL) */
typedef enum
{
	ACC_FS_SEL_2G	= 0x03,		/* 11: 2g */
	ACC_FS_SEL_4G	= 0x02,		/* 10: 4g */
	ACC_FS_SEL_8G	= 0x01,		/* 01: 8g */
	ACC_FS_SEL_16G	= 0x00,		/* 00: 16g */
} ACC_FSXL;

/* Gyroscope chain full-scale selection(FS[1:0]_G) */
typedef enum
{
	GYRO_FSS_250DPS		= 0x03,		/* 11: 250dps. */
	GYRO_FSS_500DPS		= 0x02,		/* 01: 500dps. */
	GYRO_FSS_1000DPS	= 0x01,		/* 10: 1000dps. */
	GYRO_FSS_2000DPS	= 0x00,		/* 00: 2000dps. */
} GYRO_CHAIN_FSG;

/* INT1 CTRL Setting */
typedef enum
{
	ST_INT1_EN			=	0x80,		/* Self-Test Done interrupt */
	FSYNC_INT1_EN		=	0x40,		/* FSYNC interrupt */
	PLL_RDY_INT1_EN		=	0x20,		/* PLL ready interrupt */
	RESET_DONE_INT1_EN	=	0x10,		/* Reset done interrupt */
	DRDY_INT1_EN		=	0x08,		/* Data Ready interrupt */
	FIFO_THS_INT1_EN	=	0x04,		/* FIFO threshold interrupt */
	FIFO_FULL_INT1_EN	=	0x02,		/* FIFO full interrupt */
	AGC_RDY_INT1_EN		=	0x01,		/* UI AGC ready interrupt */
	INT1_NONE			=	0x00,		/* Interrupt Setting None */
} INT1_CTRL_VAL;

/* INT2 CTRL Setting */
typedef enum
{
	ST_INT2_EN			=	0x80,		/* Self-Test Done interrupt */
	FSYNC_INT2_EN		=	0x40,		/* FSYNC interrupt */
	PLL_RDY_INT2_EN		=	0x20,		/* PLL ready interrupt */
	RESET_DONE_INT2_EN	=	0x10,		/* Reset done interrupt */
	DRDY_INT2_EN		=	0x08,		/* Data Ready interrupt */
	FIFO_THS_INT2_EN	=	0x04,		/* FIFO threshold interrupt */
	FIFO_FULL_INT2_EN	=	0x02,		/* FIFO full interrupt */
	AGC_RDY_INT2_EN		=	0x01,		/* UI AGC ready interrupt */
	INT2_NONE			=	0x00,		/* Interrupt Setting None */
} INT2_CTRL_VAL;

/* INT1 Wake on Motion */
typedef enum
{
	INT1_WOM_Z_EN		= 0x04,			/* Z-axis WOM interrupt routed */
	INT1_WOM_Y_EN		= 0x02,			/* Y-axis WOM interrupt routed */
	INT1_WOM_X_EN		= 0x01,			/* X-axis WOM interrupt routed */
} INT1_WOM_VAL;

/* INT2 Wake on Motion */
typedef enum
{
	INT2_WOM_Z_EN		= 0x04,			/* Z-axis WOM interrupt routed */
	INT2_WOM_Y_EN		= 0x02,			/* Y-axis WOM interrupt routed */
	INT2_WOM_X_EN		= 0x01,			/* X-axis WOM interrupt routed */
} INT2_WOM_VAL;

typedef enum
{
	HP_MODE		= 0,	/* Normal Mode Disable / High-performance Enable */
	NORMAL_MODE	= 1,	/* Normal Mode Enable  / High-performance Disable */
} ACC_GYRO_HM_MODE;

typedef enum
{
	FIFO_NOT_BYPASS		= 0x00,		/* FIFO is not bypassed */
	FIFO_BYPASS			= 0x01,		/* Bypass mode: FIFO disabled */
} FIFO_BYPASS_MODE;

typedef enum
{
	MODE_STREAM			= 0x00,		/* Stream-to-FIFO Mode */
	MODE_STOP_ON_FULL	= 0x01,		/* STOP-on-FULL Mode */
} FIFO_MODE;

/* FIFO_CTRL4 Temperature ODR Setting */
typedef enum
{
	ODR_T_NON_USE_FIFO	=	0x00,		/* Temperature not batched in FIFO (default) */
	ODR_T_1_6_HZ		=	0x01,		/* 1.6Hz */
	ODR_T_12_5HZ		=	0x02,		/* 12.5Hz */
	ODR_T_52HZ			=	0x03,		/* 52Hz */
} FIFO_TEMP_ODR;

/* FIFO Output Data Tag TAG_SENSOR_[4:0] */
typedef enum
{
	TAG_GYRO_NC		=	0x01,		/* Gyroscope NC */
	TAG_ACC_NC		=	0x02,		/* Accelerometer NC */
	TAG_TEMP		=	0x03,		/* Temperature */
} TAG_DATA_OUT;

/* 動作モード指定 */
typedef enum
{
	MODE_ACC_ONLY	= 0,		/* Acc Only */
	MODE_GYRO_TEMP,				/* Gyro/Temp */
	MODE_BOTH,					/* Acc/Gyro/Temp */
	MODE_ACC_ONLY_LP,			/* Acc Only Low Power */
} ACC_GYRO_EXEC_MODE;

typedef enum
{
	HEADER_ACC_DATA			= 0x01,		/* Acc Data */
	HEADER_GYRO_DATA		= 0x02,		/* Gyro Data */
	HEADER_ACC_GYRO_DATA	= 0x03,		/* Acc/Gyro Data */
	HEADER_ACC_LP_DATA		= 0x04,		/* Acc LP Mode Data */
	HEADER_ACC_GYRO			= 0x68,		/* Acc/Gyro Data */
	HEADER_FIFO_EMPTY		= 0x80,		/* Acc/Gyro Data FIFO Empty */
} ACC_GYRO_SEND_HEADER;

/* Power Management ACC Mode */
typedef enum
{
	PWR_ACC_TURN_OFF	= 0x00,		/* Turns accelerometer off */
	/* 0x01もTurn OFF Modeとなっているため定義しない */
	PWR_ACC_LP_MODE		= 0x02,		/* Places accelerometer in Low Power (LP) Mode */
	PWR_ACC_LN_MODE		= 0x03,		/* Places accelerometer in Low Noise (LN) Mode */
} PWR_ACC_MODE;

/* Power Management Gyro Mode */
typedef enum
{
	PWR_GYRO_TURN_OFF	= 0x00,		/* Turns gyroscope off */
	PWR_GYRO_STANDBY	= 0x01,		/* Places gyroscope in Standby Mode */
	PWR_GYRO_RESERVED	= 0x02,		/* Reserved */
	PWR_GYRO_LN_MODE	= 0x03,		/* Places gyroscope in Low Noise (LN) Mode */
} PWR_GYRO_MODE;

/* Timestamp Enable/Disable */
typedef enum
{
	TIMESTAMP_REG_DISABLE = 0x00,	/* Time Stamp register disable */
	TIMESTAMP_REG_ENABLE  = 0x01,	/* Time Stamp register enable */
} TMST_MODE;

/* FIFO Configuration FSYNC Timestamp */
typedef enum
{
	FIFO_TMST_FSYNC_DISABLE	= 0x00,		/* TMST in the FIFO cannot be replaced by the FSYNC timestamp */
	FIFO_TMST_FSYNC_ENABLE	= 0x01,		/* Allows the TMST in the FIFO to be replaced by the FSYNC timestamp */
} FIFO_TMST_FSYNC_MODE;

/* FIFO Configuration ACC Mode */
typedef enum
{
	FIFO_ACC_DISABLE	= 0x00,		/* Accel packets not enabled to go to FIFO */
	FIFO_ACC_ENABLE		= 0x01,		/* Enables Accel packets to go to FIFO */
} FIFO_CONFIG_ACC_MODE;

/* FIFO Configuration Gyro Mode */
typedef enum
{
	FIFO_GYRO_DISABLE	= 0x00,		/* Gyro packets not enabled to go to FIFO */
	FIFO_GYRO_ENABLE	= 0x01,		/* Enables Gyro packets to go to FIFO */
} FIFO_CONFIG_GYRO_MODE;

typedef enum
{
	PWR_MGMT_RC_OFF	= 0x00,		/* RC oscillator powered off */
	PWR_MGMT_RC_ON	= 0x10,		/* RC oscillator powered on */
} PWR_MGMT_POWER_RC;

/* Struct ----------------------------------------------------------------*/
typedef void ( *gpio_handler )( nrfx_gpiote_pin_t, nrf_gpiote_polarity_t );
typedef struct _gpio_pin_info
{
	uint32_t gpio_pin;
	nrf_gpio_pin_pull_t	pull_config;
    nrf_gpiote_polarity_t sense_config;
    gpio_handler handler;
} ACC_GYRO_GPIO_PIN_INFO;

typedef struct _gpio_pin_wakeup_info
{
	uint32_t gpio_pin;
	nrf_gpio_pin_pull_t	pull_config;
    nrf_gpio_pin_sense_t sense_config;
} ACC_GYRO_GPIO_WAKEUP_PIN_INFO;

/* ACC/Gyro Data Information */
typedef struct _acc_gyro_data_info
{
	int16_t acc_x_data;
	int16_t acc_y_data;
	int16_t acc_z_data;
	int16_t gyro_x_data;
	int16_t gyro_y_data;
	int16_t gyro_z_data;
	uint16_t timestamp;
	uint16_t sid;			/* 2020.12.07 Add */
	int8_t temperature;
	uint8_t header;
} ACC_GYRO_DATA_INFO;

/* Sensor FIFO Data Info */
typedef struct _sensor_fifo_data_info
{
	uint8_t fifo_count;
} SENSOR_FIFO_DATA_INFO;

/* 2022.06.09 Add 分解能追加 ++ */
typedef struct _acc_gyro_resolution
{
	uint16_t acc_resolution;
	uint16_t gyro_resolution;
	uint16_t reserve[12];
} ACC_GYRO_RESOLUTION;
/* 2022.06.09 Add 分解能追加 -- */

/* Function prototypes ---------------------------------------------------*/
/**
 * @brief ACC/Gyro Sensor Registor Reset
 * @param None
 * @retval UTC_SUCCESS Success
 * @retval UTC_SPI_ERROR Error
 */
uint32_t AccGyroRegReset( void );

/**
 * @brief ACC/Gyro Sensor Initialize
 * @param previous_err 一つ前の処理結果
 * @retval UTC_SUCCESS Success
 * @retval UTC_SPI_ERROR Error
 */
uint32_t AccGyroInitialize( uint32_t previous_err );

/**
 * @brief ACC/Gyro Sensor Wake Up Setting
 * @param previous_err 一つ前の処理結果
 * @retval UTC_SUCCESS Success
 * @retval UTC_SPI_ERROR Error
 */
uint32_t AccGyroWakeUpSetting( uint32_t previous_err );

/**
 * @brief ACC/Gyro Sensor Read WakeUp Interrupt Source
 * @param x_int X-Axis Wake Up
 * @param y_int Y-Axis Wake Up
 * @param z_int Z-Axis Wake Up
 * @retval UTC_SUCCESS Success
 * @retval UTC_SPI_ERROR Error
 */
uint32_t AccGyroReadIntSrc( uint8_t *x_int, uint8_t *y_int, uint8_t *z_int );

/**
 * @brief ACC/Gyro Sensor Reconfig Execution Setting
 * @param mode_number Mode id number
 * @retval UTC_SUCCESS Success
 * @retval UTC_SPI_ERROR Error
 */
uint32_t AccGyroReconfig( uint8_t mode_number );

/**
 * @brief ACC/Gyro Sensor X-Axis INT2 Enable
 * @param None
 * @retval UTC_SUCCESS Success
 * @retval UTC_SPI_ERROR Error
 */
uint32_t AccGyroEnableInterrupt( void );

/**
 * @brief ACC/Gyro Sensor Data Calc
 * @param acc_gyro_info ACC/Gyro Sensor Data
 * @retval UTC_SUCCESS Success
 * @retval UTC_SPI_ERROR Error
 */
void AccGyroCalcData( ACC_GYRO_DATA_INFO *acc_gyro_info );

/**
 * @brief ACC/Gyro Sensor INT GPIO Enable Interrupt
 * @param pin Interrupt Enable PIN Number
 * @retval None
 */
void AccGyroEnableGpioInt( uint32_t pin );

/**
 * @brief ACC/Gyro Sensor INT GPIO Disable Interrupt
 * @param pin Interrupt Enable PIN Number
 * @retval None
 */
void AccGyroDisableGpioInt( uint32_t pin );

/**
 * @brief ACC/Gyro SID Clear
 * @param pin Interrupt Enable PIN Number
 * @retval None
 */
void AccGyroSetSid( uint16_t sid );

/**
 * @brief ACC/Gyro Inc SID
 * @param None
 * @retval sid 現在のSIDを返す
 */
uint16_t AccGyroIncSid( void );

/**
 * @brief ACC/Gyro Clear FIFO Count
 * @param None
 * @retval None
 */
void AccGyroClearFifoCount( void );

/**
 * @brief ACC/Gyro Increment FIFO Count
 * @param None
 * @retval None
 */
void AccGyroIncFifoCount( void );

/**
 * @brief ACC/Gyro Sub FIFO Count
 * @param None
 * @retval fifo_count FIFO Count
 */
uint16_t AccGyroSubFifoCount( void );

/**
 * @brief ACC/Gyro Get FIFO Count
 * @param None
 * @retval fifo_count FIFO Count
 */
uint16_t AccGyroGetFifoCount( void );

/**
 * @brief validate and clear Acc/Gyro FIFO
 * @param None
 * @retval None
 */
void AccGyroValidateClearFifo( void );

/**
 * @brief Get Send Header
 * @param tag_data ACC/Gyro FIFO Tag Data
 * @retval header Send Header Data
 */
uint8_t AccGyroGetSendHeader( uint8_t tag_data );

void AccGyroSelfTest( void );

void AccGyroOneshotAcc( float* fax, float* fay, float* faz);

#endif

