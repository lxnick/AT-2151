/**
  ******************************************************************************************
  * @file    state_control.c
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/15
  * @brief   State COntrol
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/15       k.tashiro         create new
  ******************************************************************************************
*/

#include "stdint.h"
#include "nordic_common.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "app_fifo.h"
#include "lib_common.h"
#include "state_control.h"
#include "mode_manager.h"
#include "lib_ram_retain.h"
#include "lib_flash.h"
#include "ble_manager.h"
#include "lib_icm42607.h"
#include "algo_acc.h"
#include "flash_operation.h"
#include "ble_definition.h"

/*
 * イベントを受信した際の状態別関数定義テーブル
 * 状態とイベントにより呼び出す関数を決定する
 * フォーマット
 *  EVT_INIT_CMPL,				EVT_UPDATE_PARAM,					EVT_UPDATING_PARAM,				EVT_ADV_TO,					EVT_NO_WALK_TO,				EVT_CNT_CMPL, 				EVT_CNT_PARAM_UPDATE_CMPL,		EVT_CNT_PARAM_UPDATE_ERR, 	EVT_DISCNT_SUCCESS, 	EVT_FORCED_TO,
 *  EVT_DISCNT_API_CMPL,		EVT_UPDATE_TO,						EVT_UPDATE_ERR_TO,				EVT_FLASH_CNT_PARAM_CHANGE, EVT_FLASH_CNT_PARAM_RETURN,	EVT_NOTIFY_RETRY,			EVT_ACC_FIFO_INT, 				EVT_RTC_INT,				EVT_BLE_CMD_INIT,		BLE_CMD_PLAYER_INFO_SET,
 *  EVT_BLE_CMD_TIME_INFO_SET,	EVT_BLE_CMD_ACCEPT_PINCODE_CHECK,	EVT_BLE_CMD_PINCODE_CHECK,		EVT_BLE_CMD_PINCODE_ERASE,	EVT_BLE_CMD_READ_LOG,		EVT_BLE_CMD_READ_LOG_ONE,	EVT_BLE_CMD_GET_TIME,			EVT_BLE_CMD_GET_BAT, 		EVT_ADC_READ,			EVT_BLE_CMD_ERASE_LOG_ONE,
 *  EVT_BLE_CMD_MODE_CHANGE,	EVT_BLE_CMD_ULT_START_TRIG,			EVT_BLE_CMD_ULT_END_TRIG,		EVT_BLE_ACK_TO,				EVT_ULT_START_TRIG_TO,		EVT_ULT_STOP_TRIG_TO,		EVT_FLASH_DATA_FORMAT_CREATE,	EVT_FORCE_SLEEP,			EVT_BLE_CMD_ULT_TRIG,	EVT_FLASH_OP_STATUS_CHECK,
 *	EVT_INIT_CMD_START,			EVT_FLASH_DATA_WRITE_CMPL,  		EVT_FLASH_DATA_ERASE_CMPL,		EVT_RSSI_NOTIFY,			EVT_X_AXIS_ADV,				EVT_BLE_CMD_GUEST_MODE,		EVT_BLE_CMD_ANGLE_ADJUST
 */

/* Initialize */
const static STATE_ST gSt_init = {
	{	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,		NonAction,		NonAction,	NonAction,
		NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	RunAlgoAdv,		IntRtcNonBle,	NonAction,	NonAction,
		NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,		NonAction,		NonAction,	NonAction,
		NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,		ForceSleep,		NonAction,	NonAction,
		NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction
	},
	ST_INIT
};

/* Advertise */
const static STATE_ST gSt_adv = {
	{	BleAdvertisngStart,	NonAction,		NonAction,		NonAction,	EnterDeepSleep,	NonAction,	NonAction,		NonAction,		BleAdvertisngStart,	NonAction,
		NonAction,			NonAction,		NonAction,		NonAction,	NonAction,		NonAction,	RunAlgoAdv,		RepushEvent,	NonAction,			NonAction,
		NonAction,			NonAction,		NonAction,		NonAction,	NonAction,		NonAction,	NonAction,		NonAction,		BleWrapperFunc,		NonAction,
		NonAction,			NonAction,		NonAction,		NonAction,	NonAction,		NonAction,	NonAction,		ForceSleep,		NonAction,			NonAction,
		NonAction,			BleWrapperFunc,	BleWrapperFunc,	NonAction,	NonAction,		NonAction,	NonAction
	},
	ST_ADV
}; 

/* Pre DeepSlepp */
const static STATE_ST gSt_pre_deep_sleep = {
	/* 2022.03.18 Modify AccGyroReinitXAxisInt -> AdvTimeoutClearIntへ変更 */
	{	NonAction,	NonAction,	NonAction,	AdvTimeoutClearInt,		NonAction,	NonAction,	NonAction,				NonAction,		NonAction,	NonAction,
		NonAction,	NonAction,	NonAction,	NonAction,				NonAction,	NonAction,	RunAlgoPreDeepSleep,	IntRtcNonBle,	NonAction,	NonAction,
		NonAction,	NonAction,	NonAction,	NonAction,				NonAction,	NonAction,	NonAction,				NonAction,		NonAction,	NonAction,
		NonAction,	NonAction,	NonAction,	NonAction,				NonAction,	NonAction,	NonAction,				ForceSleep,		NonAction,	NonAction,
		NonAction,	NonAction,	NonAction,	NonAction,				YAxisAdv,	NonAction,	NonAction
	},
	ST_PRE_DEEP_SLEEP
}; 

/* DeepSlepp */
const static STATE_ST gSt_deep_sleep = {
	{	NonAction,	NonAction,	NonAction,	NonAction,	EnterDeepSleep,	NonAction,	NonAction	,			NonAction,		NonAction,	NonAction,
		NonAction,	NonAction,	NonAction,	NonAction,	NonAction,		NonAction,	RunAlgoPreDeepSleep,	RepushEvent,	NonAction,	NonAction,
		NonAction,	NonAction,	NonAction,	NonAction,	NonAction,		NonAction,	NonAction,				NonAction,		NonAction,	NonAction,
		NonAction,	NonAction,	NonAction,	NonAction,	NonAction,		NonAction,	NonAction,				ForceSleep,		NonAction,	NonAction,
		NonAction,	NonAction,	NonAction,	NonAction,	NonAction,		NonAction,	NonAction
	},
	ST_DEEP_SLEEP
};

/* Connect Parameter Update */
const static STATE_ST gSt_cnt_param_update = {
	{	NonAction,		NonAction,		UpdateParamCheck,	NonAction,		NonAction,					NonAction,		NonAction,		NonAction,		NonAction,		NonAction,
		NonAction,		NonAction,		NonAction,			RepushEvent,	BleSlaveLatencyZeroChange,	RepushEvent,	RunAlgo,		RepushEvent,	NonAction,		NonAction,
		NonAction,		RepushEvent,	RepushEvent,		RepushEvent,	RepushEvent,				RepushEvent,	RepushEvent,	RepushEvent,	RepushEvent,	NonAction,
		RepushEvent,	NonAction,		NonAction,			NonAction,		NonAction,					NonAction,		NonAction,		ForceSleep,		NonAction,		RepushEvent,
		RepushEvent,	RepushEvent,	RepushEvent,		RepushEvent,	NonAction,					RepushEvent,	RepushEvent
	},
	ST_CNT_PARAM_UPDATE
};

/* Connect Parameter Update Error */
const static STATE_ST gSt_cnt_param_update_err = {
	{	NonAction,	NonAction,		NonAction,		NonAction,		NonAction,	NonAction,		NonAction,		UpdateErrorFailed,	NonAction,		NonAction,
		NonAction,	NonAction,		NonAction,		NonAction,		NonAction,	RepushEvent,	RunAlgo,		RepushEvent,		NonAction,		NonAction,
		NonAction,	NonAction,		NonAction,		NonAction,		NonAction,	NonAction,		BleWrapperFunc,	BleWrapperFunc,		BleWrapperFunc,	NonAction,
		NonAction,	BleWrapperFunc,	NonAction,		NonAction,		NonAction,	NonAction,		NonAction,		ForceSleep,			BleWrapperFunc,	NonAction,
		NonAction,	BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,	NonAction,	NonAction,		NonAction
	},
	ST_CNT_PARAM_UPDATE_ERR
};

/* Connect */
const static STATE_ST gSt_cnt = {
	{	NonAction,		NonAction,		NonAction,		NonAction,		NonAction,		NonAction,		FlashOpSlaveLatencyZeroCheck,	NonAction,			NonAction,		NonAction,
		NonAction,		NonAction,		NonAction,		NonAction,		NonAction,		RetryNotify,	RunAlgo,						BleWrapperFunc,		BleWrapperFunc,	BleWrapperFunc,
		BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,					BleWrapperFunc,		BleWrapperFunc,	BleWrapperFunc,
		BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,	NonAction,						ForceSleep,			BleWrapperFunc,	BleWrapperFunc,
		BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,	NonAction,		BleWrapperFunc,	BleWrapperFunc
	},
	ST_CNT
};

/* Force Disconnect */
const static STATE_ST gSt_force_discnt = {
	{	NonAction,	NonAction,			NonAction,			NonAction,	NonAction,	NonAction,	NonAction,		NonAction,		NonAction,	BleForceDisconnect,
		NonAction,	BleForceDisconnect,	BleForceDisconnect,	NonAction,	NonAction,	NonAction,	RunAlgo,		RepushEvent,	NonAction,	NonAction,
		NonAction,	NonAction,			NonAction,			NonAction,	NonAction,	NonAction,	NonAction,		NonAction,		NonAction,	NonAction,
		NonAction,	NonAction,			NonAction,			NonAction,	NonAction,	NonAction,	NonAction,		ForceSleep,		NonAction,	NonAction,
		NonAction,	BleWrapperFunc,		BleWrapperFunc,		NonAction,	NonAction,	NonAction,	NonAction
	},
	ST_FORCE_DISCNT
};

/* Connect Parrameter Update slave latency 1 */
const static STATE_ST gSt_cnt_param_update_SlaveLatencyOne = {
	{	NonAction,		NonAction,		ChangeCntParamSlaveLetencyOne,	NonAction,					NonAction,		NonAction,		NonAction,		NonAction,		NonAction,		NonAction,
		NonAction,		NonAction,		NonAction,						BleSlaveLatencyOneChange,	RepushEvent,	RepushEvent,	RunAlgo,		RepushEvent,	NonAction,		NonAction,
		NonAction,		NonAction,		NonAction,						NonAction,					RepushEvent,	RepushEvent,	RepushEvent,	RepushEvent,	RepushEvent,	NonAction,
		RepushEvent,	NonAction,		NonAction,						NonAction,					NonAction,		NonAction,		NonAction,		ForceSleep,		NonAction,		RepushEvent,
		RepushEvent,	RepushEvent,	RepushEvent,					NonAction,					NonAction,		NonAction,		NonAction
	},
	ST_CNT_PARAM_UPDATE_SL1
};

/* Connect slave latency 1 */
const static STATE_ST gSt_cnt_slave_latency_one = {
	{	NonAction,		NonAction,		NonAction,		NonAction,		NonAction,		NonAction,		FlashOpSlaveLatencyZeroCheck,	NonAction,			NonAction,		NonAction,
		NonAction,		NonAction,		NonAction,		NonAction,		NonAction,		RetryNotify,	RunAlgo,						BleWrapperFunc,		BleWrapperFunc,	BleWrapperFunc,
		BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,					BleWrapperFunc,		BleWrapperFunc,	BleWrapperFunc,
		BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,	NonAction,						ForceSleep,			BleWrapperFunc,	BleWrapperFunc,
		BleWrapperFunc, BleWrapperFunc, BleWrapperFunc, BleWrapperFunc,	NonAction,		BleWrapperFunc,	BleWrapperFunc
	},
	ST_CNT_SLAVE_LATENCY_ONE
};

/* Pre Connect */
/* 2022.07.22 Add ゲストモード時のRTC割り込むは、通過させるように修正 RepushEvent -> ValidateRtcInterrupt */
const static STATE_ST gSt_pre_cnt = {
	{	NonAction,		NonAction,		UpdateParamCheckGuest,/*PreCntParamUpdateReject,*/	NonAction,		NonAction,		NonAction,		FlashOpSlaveLatencyZeroCheck,	NonAction,				NonAction,		NonAction,
		NonAction,		NonAction,		NonAction,					NonAction,		NonAction,		RepushEvent,	RunAlgo,						ValidateRtcInterrupt,	BleWrapperFunc,	BleWrapperFunc,
		BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,				NonAction,		BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,					BleWrapperFunc,			BleWrapperFunc,	RepushEvent,
		BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,				BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,	NonAction,						ForceSleep,				BleWrapperFunc,	BleWrapperFunc,
		BleWrapperFunc,	BleWrapperFunc,	BleWrapperFunc,				BleWrapperFunc,	NonAction,		BleWrapperFunc,	BleWrapperFunc
	},
	ST_PRE_CNT
};

/* Wait Disconnect */
const static STATE_ST gSt_wait_discnt = {
	{	NonAction,	NonAction,			NonAction,			NonAction,	NonAction,	NonAction,	NonAction,	NonAction,		BleForceDisconnect,	BleForceDisconnect,
		NonAction,	BleForceDisconnect,	BleForceDisconnect,	NonAction,	NonAction,	NonAction,	RunAlgo,	RepushEvent,	NonAction,			NonAction,
		NonAction,	NonAction,			NonAction,			NonAction,	NonAction,	NonAction,	NonAction,	NonAction,		NonAction,			NonAction,
		NonAction,	NonAction,			NonAction,			NonAction,	NonAction,	NonAction,	NonAction,	ForceSleep,		NonAction,			NonAction,
		NonAction,	BleWrapperFunc,		BleWrapperFunc,		NonAction,	NonAction,	NonAction,	NonAction
	},
	ST_WAIT_DISCNT
};

/*
 * 各状態で実行する関数のパラメータ一覧を定義したテーブル
 * [<STATE>]には、MAX_STATE_NUM以上のものは指定できない
 *  - テーブルのフォーマットは以下の用になっている
 *		EVT_INIT_CMPL, EVT_UPDATE_PARAM, 
 */
uint32_t static *const stateTable[MAX_STATE_NUM][TABLE_MAX_EVT_NUM] = {
	[ST_INIT] = {
		(uint32_t*)&gSt_adv,	(uint32_t*)&gSt_init,	(uint32_t*)&gSt_init,	(uint32_t*)&gSt_init,	(uint32_t*)&gSt_init,	(uint32_t*)&gSt_init,	(uint32_t*)&gSt_init,	(uint32_t*)&gSt_init,
		(uint32_t*)&gSt_init,	(uint32_t*)&gSt_init,	(uint32_t*)&gSt_init,	(uint32_t*)&gSt_init,	(uint32_t*)&gSt_init,	(uint32_t*)&gSt_init,	(uint32_t*)&gSt_init,	(uint32_t*)&gSt_init,
		(uint32_t*)&gSt_init,	(uint32_t*)&gSt_init,	(uint32_t*)&gSt_init,	(uint32_t*)&gSt_init,	(uint32_t*)&gSt_init,	(uint32_t*)&gSt_init,	(uint32_t*)&gSt_init,	(uint32_t*)&gSt_init,
	},
	[ST_ADV] = {
		(uint32_t*)&gSt_adv,	(uint32_t*)&gSt_pre_cnt,	(uint32_t*)&gSt_cnt_param_update,	(uint32_t*)&gSt_pre_deep_sleep,	(uint32_t*)&gSt_adv,	(uint32_t*)&gSt_cnt_param_update,	(uint32_t*)&gSt_adv,	(uint32_t*)&gSt_adv,
		(uint32_t*)&gSt_adv,	(uint32_t*)&gSt_adv,		(uint32_t*)&gSt_adv,				(uint32_t*)&gSt_adv,			(uint32_t*)&gSt_adv,	(uint32_t*)&gSt_adv,				(uint32_t*)&gSt_adv,	(uint32_t*)&gSt_adv,
		(uint32_t*)&gSt_adv,	(uint32_t*)&gSt_adv,		(uint32_t*)&gSt_adv,				(uint32_t*)&gSt_adv,			(uint32_t*)&gSt_adv,	(uint32_t*)&gSt_adv,				(uint32_t*)&gSt_adv,	(uint32_t*)&gSt_adv,
	},
	[ST_PRE_DEEP_SLEEP] = {
		(uint32_t*)&gSt_adv,			(uint32_t*)&gSt_pre_deep_sleep,	(uint32_t*)&gSt_pre_deep_sleep,	(uint32_t*)&gSt_pre_deep_sleep,	(uint32_t*)&gSt_deep_sleep,		(uint32_t*)&gSt_pre_deep_sleep,	(uint32_t*)&gSt_pre_deep_sleep,	(uint32_t*)&gSt_pre_deep_sleep,
		(uint32_t*)&gSt_pre_deep_sleep,	(uint32_t*)&gSt_pre_deep_sleep,	(uint32_t*)&gSt_pre_deep_sleep,	(uint32_t*)&gSt_pre_deep_sleep,	(uint32_t*)&gSt_pre_deep_sleep,	(uint32_t*)&gSt_pre_deep_sleep,	(uint32_t*)&gSt_pre_deep_sleep,	(uint32_t*)&gSt_pre_deep_sleep,
		(uint32_t*)&gSt_pre_deep_sleep,	(uint32_t*)&gSt_pre_deep_sleep,	(uint32_t*)&gSt_pre_deep_sleep,	(uint32_t*)&gSt_pre_deep_sleep,	(uint32_t*)&gSt_pre_deep_sleep,	(uint32_t*)&gSt_pre_deep_sleep,	(uint32_t*)&gSt_pre_deep_sleep,	(uint32_t*)&gSt_pre_deep_sleep, 
	},
	[ST_DEEP_SLEEP] = {
		(uint32_t*)&gSt_deep_sleep,	(uint32_t*)&gSt_deep_sleep,	(uint32_t*)&gSt_deep_sleep,	(uint32_t*)&gSt_deep_sleep,	(uint32_t*)&gSt_deep_sleep,	(uint32_t*)&gSt_deep_sleep,	(uint32_t*)&gSt_deep_sleep,	(uint32_t*)&gSt_deep_sleep,
		(uint32_t*)&gSt_deep_sleep,	(uint32_t*)&gSt_deep_sleep,	(uint32_t*)&gSt_deep_sleep,	(uint32_t*)&gSt_deep_sleep,	(uint32_t*)&gSt_deep_sleep,	(uint32_t*)&gSt_deep_sleep,	(uint32_t*)&gSt_deep_sleep,	(uint32_t*)&gSt_deep_sleep,
		(uint32_t*)&gSt_deep_sleep,	(uint32_t*)&gSt_deep_sleep,	(uint32_t*)&gSt_deep_sleep,	(uint32_t*)&gSt_deep_sleep,	(uint32_t*)&gSt_deep_sleep,	(uint32_t*)&gSt_deep_sleep,	(uint32_t*)&gSt_deep_sleep,	(uint32_t*)&gSt_deep_sleep,
	},
	[ST_CNT_PARAM_UPDATE] = {
		(uint32_t*)&gSt_cnt_param_update,	(uint32_t*)&gSt_cnt_param_update,	(uint32_t*)&gSt_cnt_param_update,	(uint32_t*)&gSt_cnt_param_update,	(uint32_t*)&gSt_cnt_param_update,	(uint32_t*)&gSt_cnt_param_update,	(uint32_t*)&gSt_cnt,				(uint32_t*)&gSt_cnt_param_update_err,
		(uint32_t*)&gSt_adv,				(uint32_t*)&gSt_cnt_param_update,	(uint32_t*)&gSt_cnt_param_update,	(uint32_t*)&gSt_force_discnt,		(uint32_t*)&gSt_force_discnt,		(uint32_t*)&gSt_cnt_param_update,	(uint32_t*)&gSt_cnt_param_update,	(uint32_t*)&gSt_cnt_param_update,
		(uint32_t*)&gSt_cnt_param_update,	(uint32_t*)&gSt_cnt_param_update,	(uint32_t*)&gSt_cnt_param_update,	(uint32_t*)&gSt_cnt_param_update,	(uint32_t*)&gSt_cnt_param_update,	(uint32_t*)&gSt_cnt_param_update,	(uint32_t*)&gSt_cnt_param_update,	(uint32_t*)&gSt_cnt_param_update,
	},
	[ST_CNT_PARAM_UPDATE_ERR] = {
		(uint32_t*)&gSt_cnt_param_update_err,	(uint32_t*)&gSt_cnt_param_update_err,	(uint32_t*)&gSt_cnt_param_update_err,	(uint32_t*)&gSt_cnt_param_update_err,	(uint32_t*)&gSt_cnt_param_update_err,	(uint32_t*)&gSt_cnt_param_update_err,	(uint32_t*)&gSt_cnt_param_update_err,	(uint32_t*)&gSt_cnt_param_update_err,
		(uint32_t*)&gSt_adv,					(uint32_t*)&gSt_cnt_param_update_err,	(uint32_t*)&gSt_cnt_param_update_err,	(uint32_t*)&gSt_cnt_param_update_err,	(uint32_t*)&gSt_force_discnt,			(uint32_t*)&gSt_cnt_param_update_err,	(uint32_t*)&gSt_cnt_param_update_err,	(uint32_t*)&gSt_cnt_param_update_err,
		(uint32_t*)&gSt_cnt_param_update_err,	(uint32_t*)&gSt_cnt_param_update_err,	(uint32_t*)&gSt_cnt_param_update_err,	(uint32_t*)&gSt_cnt_param_update_err,	(uint32_t*)&gSt_cnt_param_update_err,	(uint32_t*)&gSt_cnt_param_update_err,	(uint32_t*)&gSt_cnt_param_update_err,	(uint32_t*)&gSt_cnt_param_update_err,
	},
	[ST_CNT] = {
		(uint32_t*)&gSt_cnt,	(uint32_t*)&gSt_cnt,			(uint32_t*)&gSt_cnt_param_update,	(uint32_t*)&gSt_cnt,	(uint32_t*)&gSt_cnt,	(uint32_t*)&gSt_cnt,								(uint32_t*)&gSt_cnt,				(uint32_t*)&gSt_cnt,
		(uint32_t*)&gSt_adv,	(uint32_t*)&gSt_force_discnt,	(uint32_t*)&gSt_cnt,				(uint32_t*)&gSt_cnt,	(uint32_t*)&gSt_cnt,	(uint32_t*)&gSt_cnt_param_update_SlaveLatencyOne,	(uint32_t*)&gSt_cnt_param_update,	(uint32_t*)&gSt_cnt,
		(uint32_t*)&gSt_cnt,	(uint32_t*)&gSt_cnt,			(uint32_t*)&gSt_cnt,				(uint32_t*)&gSt_cnt,	(uint32_t*)&gSt_cnt,	(uint32_t*)&gSt_cnt,								(uint32_t*)&gSt_cnt,				(uint32_t*)&gSt_cnt
	},
	[ST_FORCE_DISCNT] = {
		(uint32_t*)&gSt_force_discnt,	(uint32_t*)&gSt_force_discnt,	(uint32_t*)&gSt_force_discnt,	(uint32_t*)&gSt_force_discnt,	(uint32_t*)&gSt_force_discnt,	(uint32_t*)&gSt_force_discnt,	(uint32_t*)&gSt_force_discnt,	(uint32_t*)&gSt_force_discnt,
		(uint32_t*)&gSt_adv,			(uint32_t*)&gSt_force_discnt,	(uint32_t*)&gSt_wait_discnt,	(uint32_t*)&gSt_force_discnt,	(uint32_t*)&gSt_force_discnt,	(uint32_t*)&gSt_force_discnt,	(uint32_t*)&gSt_force_discnt,	(uint32_t*)&gSt_force_discnt,
		(uint32_t*)&gSt_force_discnt,	(uint32_t*)&gSt_force_discnt,	(uint32_t*)&gSt_force_discnt,	(uint32_t*)&gSt_force_discnt,	(uint32_t*)&gSt_force_discnt,	(uint32_t*)&gSt_force_discnt,	(uint32_t*)&gSt_force_discnt,	(uint32_t*)&gSt_force_discnt,
	},
	[ST_CNT_PARAM_UPDATE_SL1] = {
		(uint32_t*)&gSt_cnt_param_update_SlaveLatencyOne,	(uint32_t*)&gSt_cnt_param_update_SlaveLatencyOne,	(uint32_t*)&gSt_cnt_param_update_SlaveLatencyOne,	(uint32_t*)&gSt_cnt_param_update_SlaveLatencyOne,	(uint32_t*)&gSt_cnt_param_update_SlaveLatencyOne,	(uint32_t*)&gSt_cnt_param_update_SlaveLatencyOne,	(uint32_t*)&gSt_cnt_slave_latency_one,				(uint32_t*)&gSt_cnt_param_update_err,
		(uint32_t*)&gSt_adv,								(uint32_t*)&gSt_force_discnt,						(uint32_t*)&gSt_cnt_param_update_SlaveLatencyOne,	(uint32_t*)&gSt_cnt_param_update_err,				(uint32_t*)&gSt_cnt_param_update_SlaveLatencyOne,	(uint32_t*)&gSt_cnt_param_update_SlaveLatencyOne,	(uint32_t*)&gSt_cnt_param_update,					(uint32_t*)&gSt_cnt_param_update_SlaveLatencyOne,
		(uint32_t*)&gSt_cnt_param_update_SlaveLatencyOne,	(uint32_t*)&gSt_cnt_param_update_SlaveLatencyOne,	(uint32_t*)&gSt_cnt_param_update_SlaveLatencyOne,	(uint32_t*)&gSt_cnt_param_update_SlaveLatencyOne,	(uint32_t*)&gSt_cnt_param_update_SlaveLatencyOne,	(uint32_t*)&gSt_cnt_param_update_SlaveLatencyOne,	(uint32_t*)&gSt_cnt_param_update_SlaveLatencyOne,	(uint32_t*)&gSt_cnt_param_update_SlaveLatencyOne,
	},
	[ST_CNT_SLAVE_LATENCY_ONE] = {
		(uint32_t*)&gSt_cnt_slave_latency_one,	(uint32_t*)&gSt_cnt_slave_latency_one,	(uint32_t*)&gSt_cnt_param_update_SlaveLatencyOne,	(uint32_t*)&gSt_cnt_slave_latency_one,	(uint32_t*)&gSt_cnt_slave_latency_one,	(uint32_t*)&gSt_cnt_slave_latency_one,				(uint32_t*)&gSt_cnt_slave_latency_one,	(uint32_t*)&gSt_cnt_slave_latency_one,
		(uint32_t*)&gSt_adv,					(uint32_t*)&gSt_force_discnt,			(uint32_t*)&gSt_cnt_slave_latency_one,				(uint32_t*)&gSt_cnt_slave_latency_one,	(uint32_t*)&gSt_cnt_slave_latency_one,	(uint32_t*)&gSt_cnt_param_update_SlaveLatencyOne,	(uint32_t*)&gSt_cnt_param_update,		(uint32_t*)&gSt_cnt_slave_latency_one,
		(uint32_t*)&gSt_cnt_slave_latency_one,	(uint32_t*)&gSt_cnt_slave_latency_one,	(uint32_t*)&gSt_cnt_slave_latency_one,				(uint32_t*)&gSt_cnt_slave_latency_one,	(uint32_t*)&gSt_cnt_slave_latency_one,	(uint32_t*)&gSt_cnt_slave_latency_one,				(uint32_t*)&gSt_cnt_slave_latency_one,	(uint32_t*)&gSt_cnt_slave_latency_one,
	},
	[ST_PRE_CNT] = {
		(uint32_t*)&gSt_pre_cnt,	(uint32_t*)&gSt_pre_cnt,		(uint32_t*)&gSt_pre_cnt,	(uint32_t*)&gSt_pre_cnt,	(uint32_t*)&gSt_pre_cnt,	(uint32_t*)&gSt_pre_cnt,							(uint32_t*)&gSt_cnt,				(uint32_t*)&gSt_cnt_param_update_err/*gSt_pre_cnt*/,
		(uint32_t*)&gSt_adv,		(uint32_t*)&gSt_force_discnt,	(uint32_t*)&gSt_pre_cnt,	(uint32_t*)&gSt_pre_cnt,	(uint32_t*)&gSt_pre_cnt,	(uint32_t*)&gSt_cnt_param_update_SlaveLatencyOne,	(uint32_t*)&gSt_cnt_param_update,	(uint32_t*)&gSt_pre_cnt,
		(uint32_t*)&gSt_pre_cnt,	(uint32_t*)&gSt_pre_cnt,		(uint32_t*)&gSt_pre_cnt,	(uint32_t*)&gSt_pre_cnt,	(uint32_t*)&gSt_pre_cnt,	(uint32_t*)&gSt_pre_cnt,							(uint32_t*)&gSt_pre_cnt,			(uint32_t*)&gSt_pre_cnt
	},
	[ST_WAIT_DISCNT] = {
		(uint32_t*)&gSt_wait_discnt,	(uint32_t*)&gSt_wait_discnt,	(uint32_t*)&gSt_wait_discnt,	(uint32_t*)&gSt_wait_discnt,	(uint32_t*)&gSt_wait_discnt,	(uint32_t*)&gSt_wait_discnt,	(uint32_t*)&gSt_wait_discnt,	(uint32_t*)&gSt_wait_discnt,
		(uint32_t*)&gSt_adv,			(uint32_t*)&gSt_wait_discnt,	(uint32_t*)&gSt_wait_discnt,	(uint32_t*)&gSt_wait_discnt,	(uint32_t*)&gSt_wait_discnt,	(uint32_t*)&gSt_wait_discnt,	(uint32_t*)&gSt_wait_discnt,	(uint32_t*)&gSt_wait_discnt,
		(uint32_t*)&gSt_wait_discnt,	(uint32_t*)&gSt_wait_discnt,	(uint32_t*)&gSt_wait_discnt,	(uint32_t*)&gSt_wait_discnt,	(uint32_t*)&gSt_wait_discnt,	(uint32_t*)&gSt_wait_discnt,	(uint32_t*)&gSt_wait_discnt,	(uint32_t*)&gSt_wait_discnt,
	},
};

/**
 * @brief Change State
 * @param pCurrState Current State Information
 * @param pCurrEvt Current Event Information
 * @retval None
 */
void ShoesChangeState(STATE_ST *pCurrState, PEVT_ST pCurrEvt)
{
	void *pData;
	if((pCurrState == NULL) || (pCurrEvt == NULL))
	{
		DEBUG_LOG(LOG_ERROR,"enter state pointer is NULL");
		sd_nvic_SystemReset();
	}
	
	if(pCurrEvt->evt_id < CHANGE_STATE_EVT)
	{
		DEBUG_LOG(LOG_DEBUG,"func change state 1 st 0x%x, evt 0x%x",pCurrState->st_id,pCurrEvt->evt_id);
		pData = (void*)stateTable[pCurrState->st_id][pCurrEvt->evt_id];
		memcpy(pCurrState, pData, sizeof(STATE_ST));
		DEBUG_LOG(LOG_DEBUG,"func change state 2 st 0x%x, evt 0x%x",pCurrState->st_id,pCurrEvt->evt_id);
	}

	if(pCurrState == NULL)
	{
		DEBUG_LOG(LOG_ERROR,"exit state pointer is NULL");
		sd_nvic_SystemReset();
	}
}

/**
 * @brief Initialize State
 * @param pCurrState Current State Information
 * @retval None
 */
void InitState(STATE_ST *pCurrState)
{
	//Initialize state
	memcpy(pCurrState,(STATE_ST*)&gSt_init,sizeof(STATE_ST));
}

/**
 * @brief Not Action Process
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t NonAction(PEVT_ST pEvent)
{
	UNUSED_PARAMETER(*pEvent);
	DEBUG_LOG(LOG_DEBUG,"not action 0x%x", pEvent->evt_id);
	return 0;
}

/**
 * @brief Force Sleep Process
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t ForceSleep(PEVT_ST pEvent)
{
	ret_code_t ret;
	
	//future change
	DEBUG_LOG(LOG_DEBUG,"force sleep");
	PreSleepUart();
	ret = sd_app_evt_wait();
	WakeUpUart();
	LIB_ERR_CHECK(ret, FORCE_SLEEP_ERROR, __LINE__);
	
	return 0;
}

/**
 * @brief Initialize State No Advertise
 * @param pCurrState Current State Information
 * @retval None
 */
void InitStateNoAdv(STATE_ST *pCurrState)
{
	//Initialize state
	memcpy(pCurrState,(STATE_ST*)&gSt_pre_deep_sleep,sizeof(STATE_ST));
}

