/**
  ******************************************************************************************
  * @file    walk_algo_daliy.c
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/25
  * @brief   Walk Algorithm Daliy Process
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/25       k.tashiro         create new
  ******************************************************************************************
*/

/* Includes --------------------------------------------------------------*/
#include "walk_algo.h"
#include "lib_common.h"
#include "lib_combsort.h"

/* Definition ------------------------------------------------------------*/
#define POINTBOX			30

#define DAILY				0			//Acc Y data
#define TAP					1			//not use
#define RADDER				2			//not use
#define START_REACTION		3			//Acc Y data
#define JUMP				4			//not use
#define SKYJUMP				5			//Acc Z data
#define SPEED_RAC			6			//not use
#define TELEPORTATION		7			//Acc Y data
#define SIDEAGILITY			8			//Acc Z data
#define DASH10				9			//not use

/* Private variables -----------------------------------------------------*/
static STORAGE g_storage[STRAGE2SEC];
static PEAK g_xfdpeak[POINTBOX];
static PEAK g_zfdpeak[POINTBOX];
static WALKPEAK g_x_walkpoint[POINTBOX];
static WALKPEAK g_z_walkpoint[POINTBOX];
static PREWALKPEAK g_pre_x_walkpoint[POINTBOX];
static PREWALKPEAK g_pre_z_walkpoint[POINTBOX];
static PRELASTWALKPEAK g_x_prelastwalkpoint;
static PRELASTWALKPEAK g_z_prelastwalkpoint;

static AVRAXES3 g_MedianTable[MEDIAN_NUM];  
static AVRAXES3 g_MedianResult;
static AVRAXES3 g_MavrTable[AVRDIM];
static FAVRAXES3 g_fMavrTable;
static float g_acc[STRAGE2SEC];
static short g_iNoMeSamples = 0;
static short g_iNoSamples = 0;
static short g_iMedianSampleflag = 0;
static short g_iMAvrSGSampleflag = 0;
static short g_iStrage2secSampleflag = 0;
static short g_istNoSamples = 0;
static short g_pre_last_xwalk_check = 0;
static short g_pre_last_zwalk_check = 0;
static short g_pre_mod_x_walkcount = 0;
static short g_pre_mod_z_walkcount = 0;
static short g_PreRuncount = 0;
static short g_PreDashcount = 0;

short lw = 1;
short lj = 1;
short ls = 1;
short lh = 1;
short lg = 1;

unsigned short g_prewalkdetect_plus = 0;
volatile unsigned short g_set_dash_up_limits = UNDER_EIGHT_DASH_LIMIT;
volatile unsigned short g_set_run_up_limits = UNDER_EIGHT_RUN_LIMIT;
const unsigned short g_age_dash_uplimit[2] = {UNDER_EIGHT_DASH_LIMIT,OVER_EIGHT_DASH_LIMIT};
const unsigned short g_age_run_uplimit[2]  = {UNDER_EIGHT_RUN_LIMIT, OVER_EIGHT_RUN_LIMIT};

/*debug display*/
volatile uint8_t gDaily_display = 0xff;

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
int8_t GetWalkResult(short XData, short ZData, unsigned short SID,short Mode, short *currentsample,void* pm)
{
	short xpeakcount = 0;
	short zpeakcount = 0;
	float pPeakAve;
	float nPeakAve;
	float groundAve;
	float low_coeff;
	float high_coeff;
	float zPtoP_thre = 500;
	THRESHOLD Threshold;
	float tmp_thre;
	short x_walkpeakcount;
	short z_walkpeakcount;
	short mod_x_walkcount;
	short	mod_z_walkcount;
	short mod_x_walkpoint_checkcount; 
	short	mod_z_walkpoint_checkcount;
	short spike_reject_mod_checkCount;
	short peaktime = 3;
	short ptom = 20;
	short sort_num;
	short tmpAccdata;
	
	/*2020 1106 change 104Hz Var*/
	float fInterval;
	uint16_t tmpInterval;
	
/*Median filter*/

	memset(&g_xfdpeak[0],0x00,(sizeof(PEAK)) * POINTBOX);
	memset(&g_zfdpeak[0],0x00,(sizeof(PEAK)) * POINTBOX);
	
	if((Mode == DAILY) || (Mode == START_REACTION) || (Mode == TELEPORTATION))
	{
		tmpAccdata = XData;
	}
	else
	{
		tmpAccdata = ZData;
	}
	
	if(g_iMedianSampleflag == 0){
		g_iNoMeSamples = AddMedian7Sample(&g_MedianTable[0], tmpAccdata, SID, g_iNoMeSamples);
		if(g_iNoMeSamples == MEDIAN_NUM){
			MedianFilter(&g_MedianTable[0], &g_MedianResult);
			g_iMedianSampleflag = 1;
		}else{
			return ALGO_DATACHARGE;
		}
	}else{
		memcpy(&g_MedianTable[0],&g_MedianTable[1],sizeof(AVRAXES3)*(MEDIAN_NUM - 1));
		g_MedianTable[MEDIAN_NUM - 1].sAccData = tmpAccdata;
		g_MedianTable[MEDIAN_NUM - 1].sid = SID;
		MedianFilter(&g_MedianTable[0], &g_MedianResult);
	}

/*Moving average filter*/
	if(g_iMAvrSGSampleflag == 0){
		g_iNoSamples = AddMAvrSG11Samples(&g_MavrTable[0], g_MedianResult.sAccData,g_MedianResult.sid,g_iNoSamples);
		
		if(g_iNoSamples == AVRDIM){
			AvrSGFilter(&g_MavrTable[0],&g_fMavrTable);
			g_iNoSamples = 0;
			g_iMAvrSGSampleflag = 1;
		}else{
			return ALGO_DATACHARGE;
		}
	}else{
		memcpy(&g_MavrTable[0], &g_MavrTable[1], (sizeof(AVRAXES3) * (AVRDIM - 1)));
		g_MavrTable[AVRDIM-1].sAccData = g_MedianResult.sAccData;
		g_MavrTable[AVRDIM-1].sid = g_MedianResult.sid;
		AvrSGFilter(&g_MavrTable[0],&g_fMavrTable);

	}

	//Daily, Jump, TELEPO, ULT, START_REACTION Mode (1 sec, storage)
	if((Mode == DAILY) || (Mode == JUMP) || (Mode == SIDEAGILITY) || (Mode == TELEPORTATION) || (Mode == START_REACTION) ||(Mode == SPEED_RAC) ||(Mode == SKYJUMP) || (Mode == DASH10)){
		if(g_iStrage2secSampleflag == 0){
			if(Mode == TELEPORTATION)
			{
				DEBUG_LOG(LOG_INFO,"telep %u, afacc %d, acc %d",g_fMavrTable.sid,(int)g_fMavrTable.fAccData, tmpAccdata);
			}
			else if(Mode == SIDEAGILITY)
			{
				DEBUG_LOG(LOG_INFO,"side  %u, afacc %d, acc %d",g_fMavrTable.sid,(int)g_fMavrTable.fAccData, tmpAccdata);
			}
			/*debug*/
			else if(Mode == DAILY)
			{
#ifdef DIALY_ACC_LOG_ON
				/*debug*/
				if(gDaily_display == (uint8_t)0)
				{
					DEBUG_LOG(LOG_INFO,"daily  %u, afacc %d, acc %d",g_fMavrTable.sid,(int)g_fMavrTable.fAccData, tmpAccdata);
				}
#endif
			}
			else if(Mode != DAILY)
			{
				DEBUG_LOG(LOG_INFO,"mode %u, sid %u, afacc %d, acc %d",Mode,g_fMavrTable.sid,(int)g_fMavrTable.fAccData, tmpAccdata);
			}
			
			g_istNoSamples = iAdd200Samples(&g_storage[0],g_fMavrTable.fAccData, g_fMavrTable.sid, g_istNoSamples);
			*currentsample = g_istNoSamples;

			if(g_istNoSamples == STRAGE2SEC){
				g_iStrage2secSampleflag = 1;
			}else{
				return ALGO_DATACHARGE;
			}
		}	
		if((g_istNoSamples < STRAGE2SEC) && (g_iStrage2secSampleflag == 1)){	
			if(Mode == TELEPORTATION)
			{
				DEBUG_LOG(LOG_INFO,"telep %u, afacc %d, acc %d",g_fMavrTable.sid,(int)g_fMavrTable.fAccData, tmpAccdata);
			}
			else if(Mode == SIDEAGILITY)
			{
				DEBUG_LOG(LOG_INFO,"side  %u, afacc %d, acc %d",g_fMavrTable.sid,(int)g_fMavrTable.fAccData, tmpAccdata);
			}
			/*debug*/
			else if(Mode == DAILY)
			{
#ifdef DIALY_ACC_LOG_ON
				/*debug*/
				if(gDaily_display == (uint8_t)0)
				{
					DEBUG_LOG(LOG_INFO,"daily  %u, afacc %d, acc %d",g_fMavrTable.sid,(int)g_fMavrTable.fAccData, tmpAccdata);
				}
#endif
			}

			else if(Mode != DAILY)
			{
				DEBUG_LOG(LOG_INFO,"mode %u, sid %u, afacc %d, acc %d",Mode,g_fMavrTable.sid,(int)g_fMavrTable.fAccData, tmpAccdata);
			}
			
			g_istNoSamples = iAdd200Samples(&g_storage[0],g_fMavrTable.fAccData, g_fMavrTable.sid, g_istNoSamples);
			*currentsample = g_istNoSamples;

		}
		if(g_istNoSamples == STRAGE2SEC){
			if((Mode == DAILY) || (Mode == SPEED_RAC) || (Mode == DASH10) || (Mode == SIDEAGILITY)){
				sort_num = DAILY_SORT_NUM;
			}else{
				sort_num = CHALLENGE_SORT_NUM_2SEC;
			}
			if((Mode == DAILY) || (Mode == START_REACTION) || (Mode == TELEPORTATION) ||(Mode == DASH10)){	
				//X-Axis threshold caluclation
				for (short i = 0; i < STRAGE2SEC; i++) {
					g_acc[i] = g_storage[i].AccData;
				}
				groundAve = Average(&g_acc[0], STRAGE2SEC);
				CombSort(g_acc,STRAGE2SEC,COMB_SORT_ELEVEN);
				pPeakAve = Average(&g_acc[0],sort_num);
				memcpy(&g_acc[0], &g_acc[STRAGE2SEC - sort_num], sizeof(float) * sort_num);
				nPeakAve = Average(&g_acc[0],sort_num);
				
//			Mode == DASH10,Daily coeff
				if((Mode == START_REACTION) || (Mode == TELEPORTATION) ){
					low_coeff = LOW_COEFF_STARTREAC;			//0.2
					high_coeff = HIGH_COEFF_STARTREAC;			//0.3
				}else{
					//HIGH_COEFF_LOW 0.6, HIGH_COEFF_HIGH 0.4
					low_coeff = HIGH_COEFF_LOW;							//0.6
					high_coeff = HIGH_COEFF_HIGH;						//0.4
				}
				
				Threshold.xp_high = groundAve + (pPeakAve - groundAve) * high_coeff;
				Threshold.xp_low = groundAve + (pPeakAve - groundAve) * low_coeff;
				Threshold.xn_low = groundAve - (groundAve - nPeakAve) * high_coeff;
				Threshold.xn_high = groundAve - (groundAve - nPeakAve) * low_coeff;
					
				if(Threshold.xp_low > Threshold.xp_high){
					tmp_thre = Threshold.xp_high;
					Threshold.xp_high = Threshold.xp_low;
					Threshold.xp_low = tmp_thre;
				}
				if(Threshold.xn_low > Threshold.xn_high){
					tmp_thre = Threshold.xn_high;
					Threshold.xn_high = Threshold.xn_low;
					Threshold.xn_low = tmp_thre;
				}
				
				//2018 log teleportation th
				if(Mode == TELEPORTATION)
				{
					DEBUG_LOG(LOG_INFO,"gAve %d, pPeakAve %d, nPeakAve %d",(int)groundAve,(int)pPeakAve,(int)nPeakAve);
					DEBUG_LOG(LOG_INFO,"pHigh th %d, pLow th %d, nHigh th %d, nLow th %d",(int)Threshold.xp_high, (int)Threshold.xp_low,(int)Threshold.xn_high,(int)Threshold.xn_low);
				}
				
			}

			if(Mode == JUMP || Mode == SKYJUMP || Mode == SIDEAGILITY || Mode == SPEED_RAC){
				//Z-Axis threshold caluclation
				for (short i = 0; i < STRAGE2SEC; i++) {
					g_acc[i] = g_storage[i].AccData;
				}
				groundAve = Average(&g_acc[0], STRAGE2SEC);
				CombSort(g_acc,STRAGE2SEC,COMB_SORT_ELEVEN);
				pPeakAve = Average(&g_acc[0],sort_num);
				memcpy(&g_acc[0],&g_acc[STRAGE2SEC - sort_num],sizeof(float) * sort_num);
				nPeakAve = Average(&g_acc[0],sort_num);
				
				//ULT_Z_AXIS coeff
				if((Mode == JUMP)|| (Mode == SPEED_RAC) || (Mode == SKYJUMP)) {
					low_coeff = HIGH_COEFF_LOW;
					high_coeff = HIGH_COEFF_HIGH;
				} else if(Mode == SIDEAGILITY) {
					low_coeff = SIDE_COEFF_LOW;
					high_coeff = SIDE_COEFF_HIGH;
				}
				
				Threshold.zp_high = groundAve + (pPeakAve - groundAve) * high_coeff;
				Threshold.zp_low  = groundAve + (pPeakAve - groundAve) * low_coeff;
				Threshold.zn_low  = groundAve - (groundAve - nPeakAve) * high_coeff;
				Threshold.zn_high = groundAve - (groundAve - nPeakAve) * low_coeff;
				
				if(Threshold.zp_low > Threshold.zp_high){
					tmp_thre = Threshold.zp_high;
					Threshold.zp_high = Threshold.zp_low;
					Threshold.zp_low = tmp_thre;
				}
				if(Threshold.zn_low > Threshold.zn_high){
					tmp_thre = Threshold.zn_high;
					Threshold.zn_high = Threshold.zn_low;
					Threshold.zn_low = tmp_thre;
				}
				
				//2018 log side agility th
				if(Mode == SIDEAGILITY)
				{
					DEBUG_LOG(LOG_INFO,"gAve %d, pPeakAve %d, nPeakAve %d",(int)groundAve,(int)pPeakAve,(int)nPeakAve);
					DEBUG_LOG(LOG_INFO,"pHigh th %d, pLow th %d, nHigh th %d, nLow th %d",(int)Threshold.zp_high, (int)Threshold.zp_low,(int)Threshold.zn_high,(int)Threshold.zn_low);
				}
				
			}
			if(Mode == DAILY || Mode == DASH10){
				DAILYCOUNT OutDaily = {0};
				memset(&g_xfdpeak[0], 0x00, (sizeof(PEAK) * POINTBOX));
				xpeakcount = PeakPointDetect1Axis(&g_storage[0], &g_xfdpeak[0], PEAK_DETECT_X_AXIS, Threshold, STRAGE2SEC);

				memset(&g_x_walkpoint[0], 0x00, (sizeof(WALKPEAK) * POINTBOX));
				//Daily Algo
				x_walkpeakcount = PtopCheck(&g_xfdpeak[0],&g_x_walkpoint[0],g_x_prelastwalkpoint,WALK_LOW_PTOP_THRESHOLD,xpeakcount,g_pre_last_xwalk_check,ptom);
										
				if(x_walkpeakcount > 0){
					g_x_prelastwalkpoint.plus_seq = g_x_walkpoint[x_walkpeakcount - 1].plus_seq;
					g_pre_last_xwalk_check = 1;
					lw = 1;
				} else {
					if (lw > 3) {
						g_x_prelastwalkpoint.plus_seq = 0x00;
						g_pre_last_xwalk_check = 0;
						lw = 1;
					} else {
						lw++;
					}
				}
				mod_x_walkcount = WalkCompare(&g_x_walkpoint[0], &g_pre_x_walkpoint[0], x_walkpeakcount, g_pre_mod_x_walkcount);

				mod_x_walkpoint_checkcount = WalkPointCheck(&g_x_walkpoint[0], mod_x_walkcount);
	
				if((mod_x_walkpoint_checkcount > 0) && (g_pre_mod_x_walkcount > 0)){
					
					/*20201106 change 104Hz*/
					tmpInterval = (uint16_t)(((int)g_x_walkpoint[0].plus_seq - (int)g_pre_x_walkpoint[g_pre_mod_x_walkcount - 1].plus_seq) % MODULO);
					fInterval = tmpInterval / SENSOR_ODR;
					g_x_walkpoint[0].interval = (uint16_t)(fInterval * 1000.0f);
					
					//g_x_walkpoint[0].interval = (uint16_t)(((int)g_x_walkpoint[0].plus_seq - (int)g_pre_x_walkpoint[g_pre_mod_x_walkcount - 1].plus_seq) % MODULO);	
				}
				if (mod_x_walkpoint_checkcount > 0) {
					for(short i = 0; i < POINTBOX; i++) {
						g_pre_x_walkpoint[i].plus_seq = g_x_walkpoint[i].plus_seq;
						g_pre_x_walkpoint[i].plus_value = g_x_walkpoint[i].plus_value;
						g_pre_x_walkpoint[i].minus_seq = g_x_walkpoint[i].minus_seq;
					}
					g_pre_mod_x_walkcount = mod_x_walkpoint_checkcount;
					ls = 1;
				} else {
					if (ls > 3) {
						memset(&g_pre_x_walkpoint[0],0x00,(sizeof(PREWALKPEAK) * (POINTBOX)));
						g_pre_mod_x_walkcount = 0;
						ls = 1;
					} else {
						ls++;
					}
				}
				
				if(Mode == DAILY){
					WalkOrRun(&g_x_walkpoint[0], mod_x_walkpoint_checkcount, g_set_run_up_limits, g_set_dash_up_limits, &OutDaily);
	
// Run,Dash 1shot detect Reject		
					if(((OutDaily.walk == 0) && (OutDaily.run == 1) && (OutDaily.dash == 0))||((OutDaily.walk == 0) && (OutDaily.run == 0) && (OutDaily.dash == 1))) {
						if((g_PreRuncount > 0) || (g_PreDashcount > 0)) {
							OutDaily.run = OutDaily.run + g_PreRuncount;
							OutDaily.dash = OutDaily.dash + g_PreDashcount;
							g_PreRuncount = 0;
							g_PreDashcount = 0;
							}else{
								g_PreRuncount = OutDaily.run;
								g_PreDashcount = OutDaily.dash;
								OutDaily.run = 0;
								OutDaily.dash = 0;
							}
					}else{
						if(OutDaily.run > 0 || OutDaily.dash > 0){
							if(g_PreRuncount > 0 || g_PreDashcount > 0){
								OutDaily.run += g_PreRuncount;
								OutDaily.dash += g_PreDashcount;
							}
						}
						g_PreRuncount = 0;
						g_PreDashcount = 0;
					}
												
					g_prewalkdetect_plus = g_x_walkpoint[mod_x_walkpoint_checkcount - 1].plus_seq;

					memcpy(pm,&OutDaily,sizeof(DAILYCOUNT));
					memcpy(&g_storage[0],&g_storage[STRAGE1SEC],(sizeof(STORAGE) * STRAGE1SEC));
					g_istNoSamples = STRAGE1SEC;
					return ALGO_SUCCESS;
				}//Mode Daily end
			}else if((Mode == JUMP) || (Mode == SIDEAGILITY) || (Mode == SPEED_RAC) || (Mode == SKYJUMP)){
				zpeakcount = PeakPointDetect1Axis(&g_storage[0], &g_zfdpeak[0], PEAK_DETECT_Z_AXIS, Threshold, STRAGE2SEC);

				memset(&g_z_walkpoint[0],0x00,(sizeof(WALKPEAK) * POINTBOX));
				if(Mode == JUMP || Mode == SKYJUMP || Mode == SPEED_RAC){
					zPtoP_thre = ZPTOP_VALUE_HIGH;
				}else{
					zPtoP_thre = ZPTOP_VALUE_LOW;
					ptom = 10;
				}
				z_walkpeakcount = PtopCheck(&g_zfdpeak[0],&g_z_walkpoint[0],g_z_prelastwalkpoint,zPtoP_thre,zpeakcount,g_pre_last_zwalk_check,ptom);
	
				memset(&g_x_walkpoint[0],0x00,(sizeof(WALKPEAK) * POINTBOX));
				if (z_walkpeakcount > 0) {
					g_z_prelastwalkpoint.plus_seq = g_z_walkpoint[z_walkpeakcount - 1].plus_seq;
					g_pre_last_zwalk_check = 1;
					lj = 1;
				} else {
					if(lj > 3){
						g_z_prelastwalkpoint.plus_seq = 0x00;
						g_pre_last_zwalk_check = 0;
						lj = 1;
					}else{
						lj++;
					}
				}
				mod_z_walkcount = WalkCompare(&g_z_walkpoint[0],&g_pre_z_walkpoint[0],z_walkpeakcount,g_pre_mod_z_walkcount);
	
				if(mod_z_walkcount > 0){
					for(short i = 0; i < POINTBOX; i++) {
						g_pre_z_walkpoint[i].plus_seq = g_z_walkpoint[i].plus_seq;
						g_pre_z_walkpoint[i].plus_value = g_z_walkpoint[i].plus_value;
						g_pre_z_walkpoint[i].minus_seq = g_z_walkpoint[i].minus_seq;
					}
					g_pre_mod_z_walkcount = mod_z_walkcount;
					lh = 1;
				} else {
					if(lh > 3){
							memset(&g_pre_z_walkpoint[0],0x00,(sizeof(PREWALKPEAK)*(POINTBOX)));
							lh = 1;
							g_pre_mod_z_walkcount = 0;
					}else{
						lh++;
					}
				}
				mod_z_walkpoint_checkcount = WalkPointCheck(&g_z_walkpoint[0], mod_z_walkcount);
				
				if(Mode == SIDEAGILITY){
					peaktime = 5;
				}else if(Mode == SKYJUMP){
					peaktime = 30;
				}
				if(Mode == JUMP || Mode == SKYJUMP || Mode == SIDEAGILITY){
					spike_reject_mod_checkCount = ModWalkCompareSpike(&g_z_walkpoint[0],&g_pre_x_walkpoint[0], mod_z_walkpoint_checkcount,g_pre_mod_x_walkcount,peaktime);
				}else{
					spike_reject_mod_checkCount = WalkCompareSpike(&g_z_walkpoint[0],&g_pre_x_walkpoint[0], mod_z_walkpoint_checkcount,g_pre_mod_x_walkcount,peaktime);
				}

				if((spike_reject_mod_checkCount > 0) && (g_pre_mod_x_walkcount > 0)){
					/*20201106 change 104Hz*/
					tmpInterval = (uint16_t)(((int)g_z_walkpoint[0].plus_seq - (int)g_pre_x_walkpoint[g_pre_mod_x_walkcount-1].plus_seq) % MODULO);
					fInterval = tmpInterval / SENSOR_ODR;
					g_z_walkpoint[0].interval = (uint16_t)(fInterval * 1000.0f);					
					
					//g_z_walkpoint[0].interval = (uint16_t)(((int)g_z_walkpoint[0].plus_seq - (int)g_pre_x_walkpoint[g_pre_mod_x_walkcount-1].plus_seq) % MODULO);
				}
				if(spike_reject_mod_checkCount > 0) {
					for(short i = 0; i < POINTBOX; i++) {
						g_pre_x_walkpoint[i].plus_seq = g_z_walkpoint[i].plus_seq;
						g_pre_x_walkpoint[i].plus_value = g_z_walkpoint[i].plus_value;
						g_pre_x_walkpoint[i].minus_seq = g_z_walkpoint[i].minus_seq;
					}
					g_pre_mod_x_walkcount = spike_reject_mod_checkCount;
					lh = 1;
				} else {
					if(lh > 3){
						memset(&g_pre_x_walkpoint[0],0x00,(sizeof(PREWALKPEAK) * POINTBOX));
						lh = 1;
						g_pre_mod_x_walkcount = 0;
					}else{
						lh++;
					}
				}
				ALTCOUNT jumpcount = {0};
				
				if((Mode == JUMP)||(Mode == SKYJUMP)){
					SKYJUMPCOUNT skyjumpcount ={0};
					// n_high -> groundAve
					JumpFlightTime(&g_z_walkpoint[0], spike_reject_mod_checkCount, &g_storage[0], Threshold.zp_high, groundAve, &skyjumpcount);
					memcpy(pm,&skyjumpcount,sizeof(SKYJUMPCOUNT));
					memcpy(&g_storage[0],&g_storage[STRAGE1SEC],(sizeof(STORAGE) * STRAGE1SEC));
					g_istNoSamples = STRAGE1SEC;
					return ALGO_SUCCESS;					
				}else if(Mode == SIDEAGILITY){//side Agility
					SideAgilityCountCheck(&g_z_walkpoint[0],spike_reject_mod_checkCount,nPeakAve,&jumpcount);
					
				} else if(Mode == SPEED_RAC) {
					SpeedReacCheckMod(&g_z_walkpoint[0],spike_reject_mod_checkCount, &g_storage[0], Threshold.zp_high, &jumpcount);
				}
				memcpy(pm,&jumpcount,sizeof(ALTCOUNT));
				memcpy(&g_storage[0],&g_storage[STRAGE1SEC],(sizeof(STORAGE) * STRAGE1SEC));
				g_istNoSamples = STRAGE1SEC;
				return ALGO_SUCCESS;

			}else if(Mode == START_REACTION || Mode == TELEPORTATION){
				memset(&g_xfdpeak[0],0x00,(sizeof(PEAK) * POINTBOX));
				if(Mode == START_REACTION){
					xpeakcount = PeakPointDetect1Axis(&g_storage[0], &g_xfdpeak[0], PEAK_DETECT_X_AXIS, Threshold,STRAGE2SEC);
				}else if(Mode == TELEPORTATION){
					xpeakcount = RadderPeakPointDetect(&g_storage[0], &g_xfdpeak[0], PEAK_DETECT_X_AXIS, Threshold, STRAGE2SEC);
				}

				memset(&g_x_walkpoint[0],0x00,(sizeof(WALKPEAK) * POINTBOX));
				if(Mode == START_REACTION){
					x_walkpeakcount = PtopCheck(&g_xfdpeak[0],&g_x_walkpoint[0],g_x_prelastwalkpoint,PTOP_VALUE_LOW,xpeakcount,g_pre_last_xwalk_check,ptom);
				}else{
					x_walkpeakcount =	RadderForwardBackPtopCheck(&g_x_walkpoint[0],&g_xfdpeak[0],g_x_prelastwalkpoint,UL_Y_PTOP_PEAK,xpeakcount,g_pre_last_xwalk_check);
				}
							
				if(x_walkpeakcount > 0){
					g_x_prelastwalkpoint.plus_seq = g_x_walkpoint[x_walkpeakcount - 1].plus_seq;
					g_pre_last_xwalk_check = 1;
					lw = 1;
				} else {
					if(lw > 3){
						g_x_prelastwalkpoint.plus_seq = 0x00;
						g_pre_last_xwalk_check = 0;
						lw = 1;
					}else{
						lw++;
					}
				}
				mod_x_walkcount = WalkCompare(&g_x_walkpoint[0],&g_pre_x_walkpoint[0],x_walkpeakcount,g_pre_mod_x_walkcount);
				mod_x_walkpoint_checkcount = WalkPointCheck(&g_x_walkpoint[0], mod_x_walkcount);

				if((mod_x_walkpoint_checkcount > 0) && (g_pre_mod_x_walkcount > 0)){
					/*20201106 change 104Hz*/
					tmpInterval = (uint16_t)(((int)g_x_walkpoint[0].plus_seq - (int)g_pre_x_walkpoint[g_pre_mod_x_walkcount - 1].plus_seq) % MODULO);
					fInterval = tmpInterval / SENSOR_ODR;
					g_x_walkpoint[0].interval = (uint16_t)(fInterval * 1000.0f);						
					
					//g_x_walkpoint[0].interval = (uint16_t)(((int)g_x_walkpoint[0].plus_seq - (int)g_pre_x_walkpoint[g_pre_mod_x_walkcount - 1].plus_seq) % MODULO);
				}
				if (mod_x_walkpoint_checkcount > 0) {
					for(short i = 0; i < POINTBOX; i++) {
						g_pre_x_walkpoint[i].plus_seq = g_x_walkpoint[i].plus_seq;
						g_pre_x_walkpoint[i].minus_seq = g_x_walkpoint[i].minus_seq;
						g_pre_x_walkpoint[i].plus_value = g_x_walkpoint[i].plus_value;
					}
					g_pre_mod_x_walkcount = mod_x_walkpoint_checkcount;
					ls = 1;
				} else {
					if (ls > 3) {
						memset(&g_pre_x_walkpoint[0],0x00,(sizeof(PREWALKPEAK) * POINTBOX));
						g_pre_mod_x_walkcount = 0;
						ls = 1;
					} else {
						ls++;
					}
				}
				if(Mode == START_REACTION){
					DAILYCOUNT walkcount = {0};
					ALTCOUNT start_sid = {0};
					WalkOrRun(&g_x_walkpoint[0], mod_x_walkcount,g_set_run_up_limits, g_set_dash_up_limits, &walkcount);
					start_sid.alt = StartReacCheck(&g_x_walkpoint[0],walkcount, &g_storage[0],Threshold.xp_high);
					memcpy(pm,&start_sid,sizeof(ALTCOUNT));		
					memcpy(&g_storage[0],&g_storage[STRAGE1SEC],(sizeof(STORAGE) * STRAGE1SEC));
					g_istNoSamples = STRAGE1SEC;
					return ALGO_SUCCESS;			
				}else if(Mode == TELEPORTATION){
					ALTCOUNT jumpcount = {0};
					TeleportationCountCheck(&g_x_walkpoint[0],mod_x_walkpoint_checkcount,&jumpcount);	
					memcpy(pm,&jumpcount,sizeof(ALTCOUNT));
					memcpy(&g_storage[0],&g_storage[STRAGE1SEC],(sizeof(STORAGE) * STRAGE1SEC));
					g_istNoSamples = STRAGE1SEC;
					return ALGO_SUCCESS;
				}
			}
		} else {
			return ALGO_DATACHARGE;
		}
	// 100 strage 1 sec
	}else if((Mode == TAP) || (Mode == RADDER)){
		if(g_iStrage2secSampleflag == 0){
			DEBUG_LOG(LOG_INFO,"tap sid %u, afacc %d, acc %d",g_fMavrTable.sid,(int)g_fMavrTable.fAccData, tmpAccdata);
			g_istNoSamples = iAdd200Samples(&g_storage[0],g_fMavrTable.fAccData, g_fMavrTable.sid,g_istNoSamples);
			*currentsample = g_istNoSamples;	
			if(g_istNoSamples == STRAGE1SEC){
				g_iStrage2secSampleflag = 1;
			}else{
				return ALGO_DATACHARGE;
			}
		}
		if((g_istNoSamples < STRAGE1SEC)&&(g_iStrage2secSampleflag == 1)){
			DEBUG_LOG(LOG_INFO,"tap sid %u, afacc %d, acc %d",g_fMavrTable.sid,(int)g_fMavrTable.fAccData, tmpAccdata);
			g_istNoSamples = iAdd200Samples(&g_storage[0],g_fMavrTable.fAccData, g_fMavrTable.sid, g_istNoSamples);
			*currentsample = g_istNoSamples;
		}
		if(g_istNoSamples == STRAGE1SEC){
			if(Mode == TAP){
				//Z-Axis threshold caluclation
				for (short i = 0; i < STRAGE1SEC; i++) {
					g_acc[i] = g_storage[i].AccData;
				}
				groundAve = Average(&g_acc[0], STRAGE1SEC);
				CombSort(g_acc,STRAGE2SEC,COMB_SORT_ELEVEN);
				pPeakAve = Average(&g_acc[0],20);
				memcpy(&g_acc[0],&g_acc[STRAGE1SEC - 20],(sizeof(float) * 20));
				nPeakAve = Average(&g_acc[0],20);
				
				Threshold.zp_high = groundAve + (pPeakAve - groundAve) * HIGH_COEFF_HIGH;
				Threshold.zp_low = groundAve + (pPeakAve - groundAve) * HIGH_COEFF_LOW;
				Threshold.zn_low = groundAve - (groundAve - nPeakAve) * HIGH_COEFF_HIGH;
				Threshold.zn_high = groundAve - (groundAve - nPeakAve) * HIGH_COEFF_LOW;
				if(Threshold.zp_low > Threshold.zp_high){
					tmp_thre = Threshold.zp_high;
					Threshold.zp_high = Threshold.zp_low;
					Threshold.zp_low = tmp_thre;
				}
				if(Threshold.zn_low > Threshold.zn_high){
					tmp_thre = Threshold.zn_high;
					Threshold.zn_high = Threshold.zn_low;
					Threshold.zn_low = tmp_thre;
				}
			}
						
			if(Mode == TAP){
				zpeakcount = PeakPointDetect1Axis(&g_storage[0], &g_zfdpeak[0], PEAK_DETECT_Z_AXIS, Threshold, STRAGE1SEC);
				memset(&g_z_walkpoint[0], 0x00, sizeof(WALKPEAK) * POINTBOX);
				z_walkpeakcount = PtopCheck(&g_zfdpeak[0],&g_z_walkpoint[0],g_z_prelastwalkpoint,ZPTOP_VALUE_LOW, zpeakcount, g_pre_last_zwalk_check,ptom);
				if (z_walkpeakcount > 0) {
					g_z_prelastwalkpoint.plus_seq = g_z_walkpoint[z_walkpeakcount - 1].plus_seq;
					g_pre_last_zwalk_check = 1;
					lj = 1;
				} else {
					if(lj > 3) {
						g_z_prelastwalkpoint.plus_seq = 0x00;
						g_pre_last_zwalk_check = 0;
						lj = 1;
					} else {
						lj++;
					}
				}
				mod_z_walkcount = WalkCompare(&g_z_walkpoint[0],&g_pre_z_walkpoint[0],z_walkpeakcount,g_pre_mod_z_walkcount);
				mod_z_walkpoint_checkcount = WalkPointCheck(&g_z_walkpoint[0], mod_z_walkcount);
				if((mod_z_walkpoint_checkcount > 0) && (g_pre_mod_z_walkcount > 0)){
					/*20201106 change 104Hz*/
					tmpInterval = (uint16_t)(((int)g_z_walkpoint[0].plus_seq - (int)g_pre_z_walkpoint[g_pre_mod_z_walkcount - 1].plus_seq) % MODULO);
					fInterval = tmpInterval / SENSOR_ODR;
					g_z_walkpoint[0].interval = (uint16_t)(fInterval * 1000.0f);
					
					//g_z_walkpoint[0].interval = (uint16_t)(((int)g_z_walkpoint[0].plus_seq - (int)g_pre_z_walkpoint[g_pre_mod_z_walkcount - 1].plus_seq) % MODULO);
				}
				if (mod_z_walkpoint_checkcount > 0) {
					for(short i = 0; i < POINTBOX; i++) {
						g_pre_z_walkpoint[i].plus_seq = g_z_walkpoint[i].plus_seq;
						g_pre_z_walkpoint[i].minus_seq = g_z_walkpoint[i].minus_seq;
						g_pre_z_walkpoint[i].plus_value = g_z_walkpoint[i].plus_value;
					}
					g_pre_mod_z_walkcount = mod_z_walkpoint_checkcount;
					lh = 1;
				} else {
					if (lh > 3) {
						memset(&g_pre_z_walkpoint[0],0x00,(sizeof(PREWALKPEAK) * POINTBOX));
						g_pre_mod_z_walkcount = 0;
						lh = 1;
					} else {
						lh++;
					}
				}
				ALTCOUNT tapcount = {0};
				TapCountCheck(&g_z_walkpoint[0], mod_z_walkpoint_checkcount, &tapcount);
				memcpy(pm,&tapcount,sizeof(ALTCOUNT));
				g_istNoSamples = (STRAGE1SEC / 2);
				memcpy(&g_storage[0],&g_storage[STRAGE1SEC / 2],(sizeof(STORAGE) * (STRAGE1SEC / 2)));
				return ALGO_SUCCESS;
				
			} 
			g_istNoSamples = STRAGE1SEC / 2;
			memcpy(&g_storage[0],&g_storage[STRAGE1SEC / 2],(sizeof(STORAGE) * (STRAGE1SEC / 2)));
			return ALGO_ERROR;
		} else {
			return ALGO_DATACHARGE;
		}
	}
	return ALGO_ERROR;
}


/**
 * @brief storage reset
 * @param None
 * @retval None
 */
void StorageReset(void)
{
	g_iNoMeSamples = 0;
	g_iNoSamples = 0;
	g_iMedianSampleflag = 0;
	g_iMAvrSGSampleflag = 0;
	g_iStrage2secSampleflag = 0;
	g_istNoSamples = 0;
	g_pre_last_xwalk_check = 0;
	g_pre_last_zwalk_check = 0;	
	g_pre_mod_x_walkcount = 0;
	g_pre_mod_z_walkcount = 0;
	g_prewalkdetect_plus = 0;
	memset(&g_storage[0], 0x00, (sizeof(STORAGE) * STRAGE2SEC));
	memset(&g_xfdpeak[0], 0x00, (sizeof(PEAK) * POINTBOX));
	memset(&g_zfdpeak[0], 0x00, (sizeof(PEAK) * POINTBOX));
	memset(&g_x_walkpoint[0], 0x00, (sizeof(WALKPEAK) * POINTBOX));	
	memset(&g_z_walkpoint[0], 0x00, (sizeof(WALKPEAK) * POINTBOX));
	memset(&g_pre_x_walkpoint[0], 0x00, (sizeof(PREWALKPEAK) * POINTBOX));
	memset(&g_pre_z_walkpoint[0], 0x00, (sizeof(PREWALKPEAK) * POINTBOX));
	memset(&g_MedianTable[0], 0x00, (sizeof(AVRAXES3) * MEDIAN_NUM));
	memset(&g_MavrTable[0], 0x00, (sizeof(AVRAXES3) * AVRDIM));
}

/**
 * @brief dash coeff select
 * @param age age
 * @retval 1 success
 */
int8_t DashCoeffSelect(unsigned short age)
{
	if(age <= CHANGE_AGE){
		g_set_dash_up_limits = g_age_dash_uplimit[0];
		g_set_run_up_limits = g_age_run_uplimit[0];
	}else{
		g_set_dash_up_limits = g_age_dash_uplimit[1];
		g_set_run_up_limits = g_age_run_uplimit[1];
	}
	DEBUG_LOG(LOG_INFO,"algo set age %u, run th %u, dash th %u",age, g_set_run_up_limits, g_set_dash_up_limits);
	return 1;
}

/**
 * @brief Sky Jump Data Reset
 * @param None
 * @retval None
 */
void SkyjumpDataReset(void)
{
	g_pre_last_xwalk_check = 0;
	g_pre_last_zwalk_check = 0;	
	g_pre_mod_x_walkcount = 0;
	g_pre_mod_z_walkcount = 0;
	g_prewalkdetect_plus = 0;
	memset(g_z_walkpoint,0x00,sizeof(g_z_walkpoint));
	memset(g_pre_z_walkpoint,0x00,sizeof(g_pre_z_walkpoint));
	memset(&g_z_prelastwalkpoint,0x00,sizeof(g_z_prelastwalkpoint));
	
}
