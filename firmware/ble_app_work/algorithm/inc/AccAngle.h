/**
  ******************************************************************************************
  * @file    AccAngle.h
  * @author  akiteru
  * @version 1.0
  * @date    2022/05/16
  * @brief   ACC Angle Calc
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2022/05/16       akiteru           create new
  ******************************************************************************************
*/

#ifndef ACCANGLE_H_
#define ACCANGLE_H_

/* Includes --------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "lib_common.h"

/* Definition ------------------------------------------------------------*/
#define ANGLE_ADJUST_START		0
#define ANGLE_ADJUST_ENABLE		1
#define ANGLE_ADJUST_DISABLE	2
#define ANGLE_ADJUST_STOP		3

/* Struct ----------------------------------------------------------------*/
typedef struct _acc_angle_var_
{
	bool  cmpl;		/* 角度調整完了/mode_managerでの角度補正を行うかの有無を保持 */
	float roll;
	float pitch;
	float yaw;
}ACC_ANGLE;

typedef struct _acc_buf_
{
	int16_t x;
	int16_t y;
	int16_t z;
}ACC_BUF, ACC_RESULT;

/* Function prototypes ---------------------------------------------------*/
/**
 * @brief Clear ACC Buffer
 * @param None
 * @retval None
 */
void clear_acc_buf(void);

/**
 * @brief Clear rot Matrix
 * @param None
 * @retval None
 */
void clear_rot_matrix(void);

/**
 * @brief ACC Angle Reset
 * @param None
 * @retval None
 */
void acc_angle_reset(void);

/**
 * @brief 姿勢角度単位変換関数 rad -> deg
 * @param rad デバイス姿勢角度[unit rad]
 * @param deg デバイス姿勢角度[unit deg]
 * @retval NRF_SUCCESS Success
 * @retval NRF_ERROR_INTERNAL Failed
 */
uint32_t angle_rad2deg( ACC_ANGLE *rad, ACC_ANGLE *deg );

/**
 * @brief 姿勢角度単位変換関数 deg -> rad
 * @param deg デバイス姿勢角度[unit deg]
 * @param red デバイス姿勢角度[unit rad]
 * @retval NRF_SUCCESS Success
 * @retval NRF_ERROR_INTERNAL Failed
 */
uint32_t angle_deg2rad( ACC_ANGLE *deg, ACC_ANGLE *rad );

/**
 * @brief デバイス姿勢角度計算関数
 * @param x ACC_X
 * @param y ACC_Y
 * @param z ACC_Z
 * @retval ret 姿勢角度[unit rad]
 *			cmpl: true 計算完了, false:計算中
 */
uint32_t clac_acc_angle( int16_t x, int16_t y, int16_t z, ACC_ANGLE *result_rad );

/**
 * @brief 補正用回転行列計算関数
 * @param angle_raw デバイス姿勢角度[unit: rad]
 * @retval None
 */
void clac_rot_matrix( ACC_ANGLE *angle_raw );

/**
 * @brief デバイス姿勢角度加速度データ補正関数
 * @param x ACC_X
 * @param y ACC_Y
 * @param z ACC_Z
 * @param calib_data 姿勢補正後加速度データ
 * @retval None
 */
void calc_acc_rot( int16_t x, int16_t y, int16_t z, ACC_RESULT *calib_data );

/* 2022.05.18 Add Flashからデータを読み出し設定する ++ */
/**
 * @brief Flashから角度情報を読み出し、補正用回転行列計算関数を実行
 * @param None
 * @retval None
 */
void SetupAngleAdjust( void );

/**
 * @brief Flashへ角度情報を書き込む
 * @param None
 * @retval None
 */
void SaveAngleAdjust( void );
/* 2022.05.18 Add Flashからデータを読み出し設定する -- */

#endif /* ACCANGLE_H_ */

