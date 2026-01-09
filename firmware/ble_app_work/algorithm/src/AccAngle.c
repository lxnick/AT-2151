/**
  ******************************************************************************************
  * @file    AccAngle.c
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

/* Includes --------------------------------------------------------------*/
#include "AccAngle.h"
#include <string.h>

#include "mode_manager.h"
#include "lib_angle_flash.h"
#include "ble_manager.h"

/* Definition ------------------------------------------------------------*/
#define ACC_BUF_SIZE	200   /* 回転行列算出用加速度データバッファサイズ */

#define M_PI			3.141593		/* 円周率 */

/* Private variables -----------------------------------------------------*/
static ACC_BUF g_acc_buf[ACC_BUF_SIZE];
static uint16_t g_buf_count = 0;

static float g_rot[3][3];   /* 加速度データ補正用回転行列バッファ */

static ROM_ANGLE_INFO g_angle_info = {0};		/* ROMデータ */

/* Private function prototypes -------------------------------------------*/
static void matrix_product3x3(float pA[3][3], float pB[3][3], float pAns[3][3]);

/**
 * @brief Clear ACC Buffer
 * @param None
 * @retval None
 */
void clear_acc_buf(void)
{
	memset(g_acc_buf, 0, sizeof(g_acc_buf));
	g_buf_count = 0;
}

/**
 * @brief Clear rot Matrix
 * @param None
 * @retval None
 */
void clear_rot_matrix(void)
{
	memset(g_rot, 0 ,sizeof(g_rot));
	g_rot[0][0] = 1.0;
	g_rot[1][1] = 1.0;
	g_rot[2][2] = 1.0;
}

/**
 * @brief ACC Angle Reset
 * @param None
 * @retval None
 */
void acc_angle_reset(void)
{
	clear_acc_buf();
	clear_rot_matrix();
}

/**
 * @brief 姿勢角度単位変換関数 rad -> deg
 * @param rad デバイス姿勢角度[unit rad]
 * @param deg デバイス姿勢角度[unit deg]
 * @retval NRF_SUCCESS Success
 * @retval NRF_ERROR_INTERNAL Failed
 */
uint32_t angle_rad2deg( ACC_ANGLE *rad, ACC_ANGLE *deg )
{
	uint32_t ret = NRF_ERROR_INTERNAL;
	
	if ( ( rad != NULL ) && ( deg != NULL ) )
	{
		ret = NRF_SUCCESS;
		deg->roll	= rad->roll * (180.0 / M_PI);
		deg->pitch	= rad->pitch * (180.0 / M_PI);
		deg->yaw	= rad->yaw * (180.0 / M_PI);
		deg->cmpl	= rad->cmpl;
	}

	return ret;
}

/**
 * @brief 姿勢角度単位変換関数 deg -> rad
 * @param deg デバイス姿勢角度[unit deg]
 * @param red デバイス姿勢角度[unit rad]
 * @retval NRF_SUCCESS Success
 * @retval NRF_ERROR_INTERNAL Failed
 */
uint32_t angle_deg2rad( ACC_ANGLE *deg, ACC_ANGLE *rad )
{
	uint32_t ret = NRF_ERROR_INTERNAL;

	if ( ( deg != NULL ) && ( rad != NULL ) )
	{
		ret = NRF_SUCCESS;
		rad->roll	= deg->roll * (M_PI / 180.0);
		rad->pitch	= deg->pitch * (M_PI / 180.0);
		rad->yaw	= deg->yaw * (M_PI / 180.0);
		rad->cmpl	= deg->cmpl;
	}

	return ret;
}

/**
 * @brief デバイス姿勢角度計算関数
 * @param x ACC_X
 * @param y ACC_Y
 * @param z ACC_Z
 * @retval ret 姿勢角度[unit rad]
 *			cmpl: true 計算完了, false:計算中
 */
uint32_t clac_acc_angle( int16_t x, int16_t y, int16_t z, ACC_ANGLE *result_rad )
{
	uint16_t i;
	int32_t total_x = 0;
	int32_t total_y = 0;
	int32_t total_z = 0;

	if ( result_rad == NULL )
	{
		return NRF_ERROR_INVALID_PARAM;
	}
	/* 結果格納 */
	result_rad->cmpl = false;

	g_acc_buf[g_buf_count].x = x;
	g_acc_buf[g_buf_count].y = y;
	g_acc_buf[g_buf_count].z = z;
	g_buf_count++;

	if(g_buf_count < ACC_BUF_SIZE)
	{
		return NRF_ERROR_INVALID_DATA;
	}

	for(i = 0; i < ACC_BUF_SIZE; i++)
	{
		total_x += g_acc_buf[i].x;
		total_y += g_acc_buf[i].y;
		total_z += g_acc_buf[i].z;
	}

	total_x = total_x / ACC_BUF_SIZE;
	total_y = total_y / ACC_BUF_SIZE;
	total_z = total_z / ACC_BUF_SIZE;

	/* 角度計算 */

	/*roll*/
	double roll = atan2((double)(total_y),(double)(total_z));

	/*pitch*/
	double sq = (double)(total_y)*(double)(total_y) + (double)(total_z)*(double)(total_z);
	if(sq < 1e-12)
	{
		return NRF_ERROR_INVALID_DATA;
	}

	sq = sqrt(sq);
	double pitch = atan2((double)(-total_x), sq);

	result_rad->cmpl	= true;
	result_rad->roll	= (float)(roll);	/* unit: rad */
	result_rad->pitch	= (float)(pitch);	/* unit: rad */
	result_rad->yaw		= 0.0;				/* 加速度センサだけではYaw角度算出不可。そのため0固定とする */

	clac_rot_matrix( result_rad );

	return NRF_SUCCESS;
}

/**
 * @brief 補正用回転行列計算関数
 * @param angle_raw デバイス姿勢角度[unit: rad]
 * @retval None
 */
void clac_rot_matrix( ACC_ANGLE *angle_raw )
{
	float Z[3][3];
	float Y[3][3];
	float X[3][3];

	if ( angle_raw == NULL )
	{
		return ;
	}

	Z[0][0] = cos( angle_raw->yaw );
	Z[0][1] = -sin( angle_raw->yaw );
	Z[0][2] = 0.0;

	Z[1][0] = sin( angle_raw->yaw );
	Z[1][1] = cos( angle_raw->yaw );
	Z[1][2] = 0.0;

	Z[2][0] = 0.0;
	Z[2][1] = 0.0;
	Z[2][2] = 1.0;

	Y[0][0] = cos( angle_raw->pitch );
	Y[0][1] = 0.0;
	Y[0][2] = sin( angle_raw->pitch );

	Y[1][0] = 0.0;
	Y[1][1] = 1.0;
	Y[1][2] = 0.0;

	Y[2][0] = -sin( angle_raw->pitch );
	Y[2][1] = 0.0;
	Y[2][2] = cos( angle_raw->pitch );

	X[0][0] = 1.0;
	X[0][1] = 0.0;
	X[0][2] = 0.0;

	X[1][0] = 0.0;
	X[1][1] = cos( angle_raw->roll );
	X[1][2] = -sin( angle_raw->roll );

	X[2][0] = 0.0;
	X[2][1] = sin( angle_raw->roll );
	X[2][2] = cos( angle_raw->roll );

	float ans_1[3][3];

	/*
	 * 補正行列 A = Z * Y * X
	 * */

	/* tempA = Z * Y */
	matrix_product3x3( Z, Y, ans_1 );

	/* A = tempA * X */
	matrix_product3x3( ans_1, X, g_rot );  /*g_rotに補正用回転行列数値を格納*/
}

/* 2022.05.18 Add Flashからデータを読み出し設定する ++ */
/**
 * @brief Flashから角度情報を読み出し、補正用回転行列計算関数を実行
 * @param None
 * @retval None
 */
void SetupAngleAdjust( void )
{
	ACC_ANGLE angle_data = {0};
	uint32_t ret;
	uint8_t signiture[] = { ANGLE_ADJUST_SIG };
	
	ret = ReadAngleAdjust( &g_angle_info );
	if ( ret == NRF_SUCCESS )
	{
		if ( memcmp( &g_angle_info.signiture[0], signiture, ANGLE_ADJUST_SIG_SIZE ) == 0 )
		{
#if 1
			uint8_t buffer[128] = {0};
			sprintf( (char *)buffer, "state: %d roll: %f, pitch: %f", g_angle_info.state, g_angle_info.x_angle, g_angle_info.y_angle );
			DEBUG_LOG( LOG_INFO, "%s", buffer );
#endif
			/* signiture一致 */
			angle_data.cmpl		= false;
			angle_data.roll		= g_angle_info.x_angle;	/* unit: rad */
			angle_data.pitch	= g_angle_info.y_angle;	/* unit: rad */
			angle_data.yaw		= 0.0;					/* 加速度センサだけではYaw角度算出不可。そのため0固定とする */
			/* 補正値を計算 */
			clac_rot_matrix( &angle_data );
			/* 使用する情報の設定 */
			SetAngleAdjustInfo( &angle_data );
			/* BLEのReadデータを設定 */
			ChangeAngleAdjustState( g_angle_info.state );
		}
		else
		{
			/* BLEのReadデータを設定 */
			ChangeAngleAdjustState( ANGLE_ADJUST_DISABLE );
		}
	}
}

/**
 * @brief Flashへ角度情報を書き込む
 * @param None
 * @retval None
 */
void SaveAngleAdjust( void )
{
	ACC_ANGLE angle_data = {0};
	uint8_t state = 0;
	uint8_t signiture[] = { ANGLE_ADJUST_SIG };
	
	/* 補正値取得 */
	GetAngleAdjustInfo( &angle_data );
	if ( ( g_angle_info.x_angle != angle_data.roll ) || ( g_angle_info.y_angle != angle_data.pitch ) )
	{
		DEBUG_LOG( LOG_INFO, "!!! Save Angle Adjust !!!" );
		/* ステータス取得 */
		GetAngleAdjustState( &state );
		/* 補正値が更新されている場合書き込む */
		memcpy( &g_angle_info.signiture[0], signiture, ANGLE_ADJUST_SIG_SIZE );
		g_angle_info.state		= state;
		g_angle_info.x_angle	= angle_data.roll;
		g_angle_info.y_angle	= angle_data.pitch;
		/* ROMへ保存 */
		WriteAngleAdjust( &g_angle_info );
	}
}

/* 2022.05.18 Add Flashからデータを読み出し設定する -- */

/**
 * @brief デバイス姿勢角度加速度データ補正関数
 * @param x ACC_X
 * @param y ACC_Y
 * @param z ACC_Z
 * @param calib_data 姿勢補正後加速度データ
 * @retval None
 */
void calc_acc_rot( int16_t x, int16_t y, int16_t z, ACC_RESULT *calib_data )
{
	if ( calib_data != NULL )
	{
		calib_data->x = x * g_rot[0][0] + y * g_rot[0][1] + z * g_rot[0][2];
		calib_data->y = x * g_rot[1][0] + y * g_rot[1][1] + z * g_rot[1][2];
		calib_data->z = x * g_rot[2][0] + y * g_rot[2][1] + z * g_rot[2][2];
	}
}

/**
 * @brief 3x3行列 行列積計算関数
 * @remark この関数は3x3行列にしか対応していない事に注意
 * @param pA 3x3行列
 * @param pB 3x3行列
 * @param pAns pA x pBの行列積
 * @retval None
 */
static void matrix_product3x3(float pA[3][3], float pB[3][3], float pAns[3][3])
{
	uint16_t i,j,k;
	float term = 0.0;

	for(i = 0; i < 3; i++)
	{
		for(j = 0; j < 3; j++)
		{
			term = 0.0;
			for(k = 0; k < 3; k++)
			{
				term = term + pA[i][k] * pB[k][j];
			}
			pAns[i][j] = term;
		}
	}
}
