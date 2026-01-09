/**
  ******************************************************************************************
  * @file    walk_algo.h
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/25
  * @brief   Walk Algorithm
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/25       k.tashiro         create new
  ******************************************************************************************
*/

#ifndef WALK_ALGO_H_
#define WALK_ALGO_H_

/* Includes --------------------------------------------------------------*/
#include <stdint.h>
#include "lib_common.h"

/* Definition ------------------------------------------------------------*/
#define AVRDIM						8
#define MEDIAN_NUM					7
#define STRAGE2SEC					200
#define STRAGE1SEC					STRAGE2SEC / 2
#define	PEAK_DETECT_X_AXIS			0
#define PEAK_DETECT_Z_AXIS			1
#define WALK_PTOP_THRESHOLD			4500.0f 
#define WALK_LOW_PTOP_THRESHOLD		1000.0f	
//#define DASH_INTERVAL				60				//60 under six age limit, other age limit is 65.
//#define UNDER_SIX_DASH_LIMIT		60
//#define OVER_SIX_DASH_LIMIT			65

/* age threshold */
/*
#define OVER_EIGHT_RUN_LIMIT		85
#define OVER_EIGHT_DASH_LIMIT		65
#define UNDER_EIGHT_RUN_LIMIT		85
#define UNDER_EIGHT_DASH_LIMIT		60

#define CHANGE_AGE					8
#define WALK_LIMIT					300	
//#define WALK_INTERVAL				85
#define DASH_LIMIT					30				// Dash interval high speed limt.
*/

/*20201106 change 104Hz*/
/* age threshold */
#define OVER_EIGHT_RUN_LIMIT		850
#define OVER_EIGHT_DASH_LIMIT		650
#define UNDER_EIGHT_RUN_LIMIT		850
#define UNDER_EIGHT_DASH_LIMIT		600

#define CHANGE_AGE					8
#define WALK_LIMIT					3000	
//#define WALK_INTERVAL				85
#define DASH_LIMIT					300				// Dash interval high speed limt.
/*****************************************/

#define JUMP_MINUS_PEAK				500.0f
#define JUMP_PTOP_VALUE				1000.0f
#define REAC_JUMP_MINUS_PEAK		500.0f
#define	REAC_JUMP_PTOP_VALUE		300.0f
#define TAP_MINUS_PEAK				500.0f
#define TAP_PTOP					500.0f
#define RADDER_PTOP					300.0f			//after change radder ptop 
#define WALK_PTOP					500.0f
#define UL_MINUS_PEAK				550.0f 	
#define	UL_PTOP_PEAK				200.0f
#define UL_Y_PTOP_PEAK				500.0f

//MODULO coeff.
#define MODULO						65536

#define ALGO_SUCCESS				1
#define ALGO_DATACHARGE				2
#define ALGO_ERROR					0

#define HIGH_COEFF_HIGH				0.6f
#define HIGH_COEFF_LOW				0.4f

#define SIDE_COEFF_HIGH				0.3f
#define SIDE_COEFF_LOW				0.2f
#define SIDE_PTOP_PEAK				1000.0f

#define HIGH_COEFF_STARTREAC		0.3f
#define LOW_COEFF_STARTREAC			0.2f

#define PTOP_VALUE_HIGH				1000.0f
#define PTOP_VALUE_LOW				500.0f
#define ZPTOP_VALUE_HIGH			500.0f
#define ZPTOP_VALUE_LOW				300.0f

#define	DAILY_SORT_NUM				40
#define CHALLENGE_SORT_NUM_2SEC		30

/* 2022.03.18 Add アルゴリズム計算用ODR値追加 ++ */
#define SENSOR_ODR					100.0f
/* 2022.03.18 Add アルゴリズム計算用ODR値追加 -- */

/* Struct ----------------------------------------------------------------*/
typedef struct _storage
{
	float			AccData;		//4 byte
	unsigned short	index;			//Index Number
}STORAGE, *PSTORAGE;

typedef struct _peakdata {
	float		peak;			// peak value
	short		dir;			// peak direction    +1: plus peak  -1: minus peak
	uint16_t	indexnum; 		// index number.        // ---> original statement ... unsigned int   indexnum;   // Index Number
} PEAK, *PPEAK;

typedef struct _walkpeakdata {
	float		plus_value;			// + value.
	int			interval;
	float		minus_value;		// - value.
	float		ptop_value;			// + top value.
	uint16_t	plus_seq;			// + sequence num.         // ---> original statement ... uint32_t   plus_seq;
	uint16_t	minus_seq;			// - sequence num.         // ---> original statement ... uint32_t   minus_seq;
	short		plus_spec;			//
	short		minus_spec;			//   6 byte+36 byte = 42byte
	short		ptop_time;			//
} WALKPEAK, *PWALKPEAK;

typedef struct _threshold {
	 float xp_high;
	 float xp_low;
	 float xn_high;
	 float xn_low;
	 float zp_high;
	 float zp_low;
	 float zn_high;
	 float zn_low;
} THRESHOLD, *PTHRESHOLD;

typedef struct _prewalkpeakdata{
	uint16_t plus_seq;
	float  plus_value; 
	uint16_t minus_seq;
} PREWALKPEAK, *PPREWALKPEAK;

typedef struct _prelastwalkpeakdata {
	uint16_t plus_seq;
} PRELASTWALKPEAK, *PPRELASTWALKPEAK;

typedef struct _avr3axesxyzdata {
	short    sAccData;
	unsigned short   sid;
} AVRAXES3, *PAVRAXES3;


typedef struct _favr3axesxyzdata
{

	float    fAccData;
	unsigned short  sid;
} FAVRAXES3, *FPAVRAXES3;

// daily
typedef struct _dailycount {
	short walk;
	short run;
	short dash;
} DAILYCOUNT, *PDAILYCOUNT;

// altimet output
typedef struct _altcount {
	short alt;
}ALTCOUNT, *PALTCOUNT;

// skyjump
typedef struct _skyjumpcount {
	short alt;
	unsigned short sid;
}SKYJUMPCOUNT, *PSKYJUMPCOUNT;

/* Function prototypes ----------------------------------------------------*/
/**
 * @brief Jump mode spike peak reject
 * @param wpoint walk peak
 * @param prewpoint pre walk peak
 * @param lengthpeak Length Peak
 * @param lengthprepeak Length PrePeak
 * @param peaktime Peak Time
 * @retval j
 */
//short Walk_compare_spike(WALKPEAK wpoint[], PREWALKPEAK prewpoint[], short lengthpeak, short lengthprepeak, short peaktime);
short WalkCompareSpike(WALKPEAK wpoint[], PREWALKPEAK prewpoint[], short lengthpeak, short lengthprepeak, short peaktime);

/**
 * @brief Jump mode spike peak reject
 * @param wpoint walk peak
 * @param prewpoint pre walk peak
 * @param lengthpeak Length Peak
 * @param lengthprepeak Length PrePeak
 * @param peaktime Peak Time
 * @retval j
 */
//short mod_Walk_compare_spike(WALKPEAK wpoint[], PREWALKPEAK prewpoint[], short lengthpeak, short lengthprepeak, short peaktime);
short ModWalkCompareSpike(WALKPEAK wpoint[], PREWALKPEAK prewpoint[], short lengthpeak, short lengthprepeak, short peaktime);

/**
 * @brief Walk Compare
 * @param wpoint walk peak
 * @param prewpoint pre walk peak
 * @param lengthpeak Length Peak
 * @param lengthprepeak Length PrePeak
 * @retval n
 */
//short Walk_compare(WALKPEAK wpoint[], PREWALKPEAK prewpoint[], short lengthpeak, short lengthprepeak);
short WalkCompare(WALKPEAK wpoint[], PREWALKPEAK prewpoint[], short lengthpeak, short lengthprepeak);

/**
 * @brief Peak Top Check
 * @param peak peak
 * @param outwalkpoint walk peak
 * @param predata pre walk peak
 * @param ptop_Threshold Peak Top Threshold
 * @param peakcount Peak count
 * @param prelastpeak Last PrePeak
 * @param pm pm
 * @retval num
 */
//short ptopcheck(PEAK peak[], WALKPEAK outwalkpoint[], PRELASTWALKPEAK predata, float ptop_Threshold, short peakcount, short prelastpeak,short pm);
short PtopCheck(PEAK peak[], WALKPEAK outwalkpoint[], PRELASTWALKPEAK predata, float ptop_Threshold, short peakcount, short prelastpeak, short pm);

/**
 * @brief Average
 * @param Data data array
 * @param num data array size
 * @retval average
 */
float Average(float Data[], short num);

/**
 * @brief Add MAvr SG11 Samples
 * @param MAvr_Table AVRAXES3 Struct
 * @param AccData ACC Data
 * @param SID SID
 * @param index index
 * @retval index
 */
short AddMAvrSG11Samples(AVRAXES3 MAvr_Table[], short AccData, unsigned short SID, short index);

/**
 * @brief Avr SG Filter
 * @param MAvr_Table AVRAXES3 Struct
 * @param fMAvr FAVRAXES3 Data
 * @retval None
 */
void AvrSGFilter(AVRAXES3 MAvr_Table[], FAVRAXES3 *fMAvr);

/**
 * @brief Add Median 7 Samples
 * @param MMed_Table AVRAXES3 Struct
 * @param AccData ACC Data
 * @param SID SID
 * @param index index
 * @retval index
 */
short AddMedian7Sample(AVRAXES3 MMed_Table[], short AccData, unsigned short SID, short index);

/**
 * @brief SkyJump Mode count check and flight duration time. new algo_rhythm.
 * @param walkpoint walk peak
 * @param count count
 * @param gXStrage storage data
 * @param high_th higt Threshold
 * @param minus_thre Minus Threshold
 * @param outputCount output count
 * @retval None
 */
//void Jump_flightTime(WALKPEAK walkpoint[], unsigned short count, STORAGE gXStrage[], float high_th, float minus_thre, SKYJUMPCOUNT *outputCount);
void JumpFlightTime(WALKPEAK walkpoint[], unsigned short count, STORAGE gXStrage[], float high_th, float minus_thre, SKYJUMPCOUNT *outputCount);

/**
 * @brief Tap mode count check.
 * @param walkpoint walk peak
 * @param count count
 * @param outputCount output count
 * @retval None
 */
//void Tap_count_check(WALKPEAK walkpoint[], unsigned short count, ALTCOUNT *outputCount);
void TapCountCheck(WALKPEAK walkpoint[], unsigned short count, ALTCOUNT *outputCount);

/**
 * @brief Radder count check
 * @param walkpoint walk peak
 * @param count count
 * @param outputCount output count
 * @retval None
 */
void RADDER_Check(WALKPEAK walkpoint[], unsigned short count, ALTCOUNT *outputCount);

/**
 * @brief TELEPORTATION count
 * @param walkpoint walk peak
 * @param count count
 * @param outputCount output count
 * @retval None
 */
//void Teleportation_count_check(WALKPEAK walkpoint[],unsigned short count, ALTCOUNT *outputCount);
void TeleportationCountCheck(WALKPEAK walkpoint[],unsigned short count, ALTCOUNT *outputCount);

/**
 * @brief Side Agility function
 * @param walkpoint walk peak
 * @param count count
 * @param ave average
 * @param outputCount output count
 * @retval None
 */
//void SideAgility_count_check(WALKPEAK walkpoint[],unsigned short count, float ave, ALTCOUNT *outputCount);
void SideAgilityCountCheck(WALKPEAK walkpoint[],unsigned short count, float ave, ALTCOUNT *outputCount);

/**
 * @brief Old mode. speed reaction judge function
 * @param walkpoint walk peak
 * @param count count
 * @param storage storage data
 * @param high_th high Threshold
 * @param outputCount output count
 * @retval None
 */
//void SpeedReac_Check_mod(WALKPEAK walkpoint[], unsigned short count, STORAGE storage[], float high_th, ALTCOUNT *outputCount);
void SpeedReacCheckMod(WALKPEAK walkpoint[], unsigned short count, STORAGE storage[], float high_th, ALTCOUNT *outputCount);

/**
 * @brief Walk Run Dash judge
 * @param walkpoint walk peak
 * @param count count
 * @param run_up_limits run up limit
 * @param dash_up_limits dash up limit
 * @param walkcount walk count
 * @retval None
 */
void WalkOrRun(WALKPEAK walkpoint[], unsigned short count,unsigned short run_up_limits, unsigned short dash_up_limits, DAILYCOUNT *walkcount);

/**
 * @brief 1Axis Peak Detect
 * @param gXStrage storage data
 * @param peak Peak
 * @param flag flag
 * @param threshold Threshold
 * @param samplecount sample count
 * @retval peakcount
 */
//short peak_point_detect_1Axis(STORAGE gXStrage[], PEAK peak[],short flag, THRESHOLD threshold, short samplecount);
short PeakPointDetect1Axis(STORAGE gXStrage[], PEAK peak[], short flag, THRESHOLD threshold, short samplecount);

/**
 * @brief Add 200 Samples
 * @param Strage storage data
 * @param fAccData acc data
 * @param index index
 * @param threshold Threshold
 * @param samplecount sample count
 * @retval samplecount
 */
short iAdd200Samples(STORAGE Strage[], float fAccData, unsigned short index, short samplecount);

/**
 * @brief Sort Dec
 * @param a src value
 * @param b comp value
 * @retval 0 a = b
 * @retval 1 a < b
 * @retval -1 a > b
 */
//int sort_func_dec(const void *a, const void *b);
int SortFuncDec(const void *a, const void *b);

/**
 * @brief Median Filter
 * @param fMedi_Table AVRAXES3 data table
 * @param fMedi midian data
 * @retval None
 */
void MedianFilter(AVRAXES3 fMedi_Table[], AVRAXES3 *fMedi);

/**
 * @brief walk peak detect check
 * @param walkpoint walk peak
 * @param count count
 * @retval outcount
 */
//short walkPointCheck(WALKPEAK walkpoint[], short count);
short WalkPointCheck(WALKPEAK walkpoint[], short count);

/**
 * @brief StartReaction Judge function
 * @param walkpoint walk peak
 * @param walkcount daily count
 * @param ystorage storage data
 * @param high_th high Threshold
 * @retval start_time
 */
//short StartReac_check(WALKPEAK walkpoint[],DAILYCOUNT walkcount,STORAGE storage[], float high_th);
short StartReacCheck(WALKPEAK walkpoint[],DAILYCOUNT walkcount,STORAGE ystorage[], float high_th);

/**
 * @brief Radder Peak point detect
 * @param gXStrage storage data
 * @param peak peak data
 * @param flag flag
 * @param threshold Threshold
 * @param samplecount sample count
 * @retval peakcount
 */
//short radder_peak_point_detect(STORAGE gXStrage[], PEAK peak[],short flag, THRESHOLD threshold, short samplecount);
short RadderPeakPointDetect(STORAGE gXStrage[], PEAK peak[],short flag, THRESHOLD threshold, short samplecount);

/**
 * @brief Radder Forward Back Peak top Check
 * @param walkpoint walk peak
 * @param Data peak data
 * @param PreData pre last walk peak
 * @param threshold Threshold
 * @param count count
 * @param precount Pre Count
 * @retval outcount
 */
//short Radder_Forward_Back_ptopcheck(WALKPEAK walkpoint[],PEAK Data[], PRELASTWALKPEAK PreData, float threshold, short count, short precount);
short RadderForwardBackPtopCheck(WALKPEAK walkpoint[],PEAK Data[], PRELASTWALKPEAK PreData, float threshold, short count, short precount);

/**
 * @brief Get Walk Result
 * @param XData X Axis Data
 * @param ZData Z Axis Data
 * @param SID sid
 * @param Mode mode
 * @param currentsample current sample
 * @param pm pm
 * @retval ALGO_SUCCESS Success
 * @retval ALGO_DATACHARGE Data Charge
 * @retval ALGO_ERROR Error
 */
//int8_t GET_WALK_RESULT(short XData, short ZData, unsigned short SID,short Mode, short *currentsample,void* pm);
int8_t GetWalkResult(short XData, short ZData, unsigned short SID,short Mode, short *currentsample,void* pm);

/**
 * @brief storage reset
 * @param None
 * @retval None
 */
//void STORAGE_RESET(void);
void StorageReset(void);

/**
 * @brief dash coeff select
 * @param age age
 * @retval 1 success
 */
//int8_t dash_coeff_select(unsigned short age);
int8_t DashCoeffSelect(unsigned short age);

/**
 * @brief Sky Jump Data Reset
 * @param None
 * @retval None
 */
//void skyjump_data_reset(void);
void SkyjumpDataReset(void);

#endif
