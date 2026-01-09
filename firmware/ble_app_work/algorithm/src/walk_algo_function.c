/**
  ******************************************************************************************
  * @file    walk_algo_function.c
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/25
  * @brief   Walk Algorithm Function
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/25       k.tashiro         create new
  ******************************************************************************************
*/

/* Includes --------------------------------------------------------------*/
#include <math.h>
#include "walk_algo.h"
#include "lib_common.h"

/**
 * @brief Dec Sort
 * @param Acc acc data array
 * @param SortAcc acc data array
 * @param num array size
 * @retval None
 */
static void dec_sort(short Acc[], short SortAcc[], short num);

/**
 * @brief Jump mode spike peak reject
 * @param wpoint walk peak
 * @param prewpoint pre walk peak
 * @param lengthpeak Length Peak
 * @param lengthprepeak Length PrePeak
 * @param peaktime Peak Time
 * @retval j
 */
short WalkCompareSpike(WALKPEAK wpoint[], PREWALKPEAK prewpoint[], short lengthpeak, short lengthprepeak, short peaktime)
{
	short i;
	short j = 0;
	short p_m_peakTime = peaktime;
	if(lengthpeak > 1){
		for(i = 0; i < (lengthpeak - 1); i++){
			if((uint16_t)(((int)wpoint[i].minus_seq - (int)wpoint[i+1].plus_seq)%MODULO) < p_m_peakTime && (/*(uint16_t)*/((int)wpoint[i].minus_seq - (int)wpoint[i+1].plus_seq) % MODULO) > 0){
				wpoint[i+1].plus_seq = 0;
				wpoint[i+1].plus_value = 0;
				wpoint[i+1].plus_spec = 0;
				wpoint[i+1].minus_seq = 0;
				wpoint[i+1].minus_value = 0;
				wpoint[i+1].minus_spec = 0;
				wpoint[i+1].ptop_value = 0;
				wpoint[i+1].ptop_time = 0;
				wpoint[i+1].interval = 0;
			}
		} 
	}

	if(lengthprepeak > 0){
		for(i = 0; i < lengthpeak; i++){
			for(j = 0; j < lengthprepeak; j++){
				if(((uint16_t)(((int)(prewpoint[j].minus_seq) - (int)(wpoint[i].plus_seq)) % MODULO) < p_m_peakTime) && (/*(uint16_t)*/(((int)prewpoint[j].minus_seq - (int)wpoint[i].plus_seq)%MODULO) > 0)){
					wpoint[i].plus_seq = 0;
					wpoint[i].plus_value = 0;
					wpoint[i].plus_spec = 0;
					wpoint[i].minus_seq = 0;
					wpoint[i].minus_value = 0;
					wpoint[i].minus_spec = 0;
					wpoint[i].ptop_value = 0;
					wpoint[i].ptop_time = 0;
					wpoint[i].interval = 0;	
				}
			}
		}
	}
	j = 0;
	for(i = 0; i < lengthpeak; i++){
		if(wpoint[i].plus_spec != 0){
			wpoint[j].plus_seq = wpoint[i].plus_seq;
			wpoint[j].plus_value = wpoint[i].plus_value;
			wpoint[j].plus_spec = wpoint[i].plus_spec;
			wpoint[j].minus_seq = wpoint[i].minus_seq;
			wpoint[j].minus_value = wpoint[i].minus_value;
			wpoint[j].minus_spec = wpoint[i].minus_spec;
			wpoint[j].ptop_value = wpoint[i].ptop_value;
			wpoint[j].ptop_time = wpoint[i].ptop_time;
			wpoint[j].interval = wpoint[i].interval;
			j++;
		}
	}
	
	if (lengthpeak > j) {
		memset(&wpoint[j],0x00,(sizeof(WALKPEAK) * (lengthpeak - j)));
	}
	return j;
}
/**
 * @brief Jump mode spike peak reject
 * @param wpoint walk peak
 * @param prewpoint pre walk peak
 * @param lengthpeak Length Peak
 * @param lengthprepeak Length PrePeak
 * @param peaktime Peak Time
 * @retval j
 */
short ModWalkCompareSpike(WALKPEAK wpoint[], PREWALKPEAK prewpoint[], short lengthpeak, short lengthprepeak, short peaktime)
{
	short i;
	short j = 0;
	short p_m_peakTime = peaktime;

	if(lengthpeak == 0){
		return j;
	}
	
	if(lengthpeak > 1){
		for(i = 0; i < (lengthpeak - 1); i++){
			if((uint16_t)abs((((int)wpoint[i + 1].plus_seq - (int)wpoint[i].minus_seq ) % MODULO)) < p_m_peakTime){
				wpoint[i+1].plus_seq = 0;
				wpoint[i+1].plus_value = 0;
				wpoint[i+1].plus_spec = 0;
				wpoint[i+1].minus_seq = 0;
				wpoint[i+1].minus_value = 0;
				wpoint[i+1].minus_spec = 0;
				wpoint[i+1].ptop_value = 0;
				wpoint[i+1].ptop_time = 0;
				wpoint[i+1].interval = 0;
			}
		} 
	}
	
	if(lengthprepeak > 0){
		for(i = 0; i < lengthpeak; i++){
			for(j = 0; j < lengthprepeak; j++){
				if(((uint16_t)abs((((int)(prewpoint[j].minus_seq) - (int)(wpoint[i].plus_seq)) % MODULO)) < p_m_peakTime)){
					wpoint[i].plus_seq = 0;
					wpoint[i].plus_value = 0;
					wpoint[i].plus_spec = 0;
					wpoint[i].minus_seq = 0;
					wpoint[i].minus_value = 0;
					wpoint[i].minus_spec = 0;
					wpoint[i].ptop_value = 0;
					wpoint[i].ptop_time = 0;
					wpoint[i].interval = 0;	
				}
			}
		}
	}
	j = 0;
	
	for(i = 0; i < lengthpeak; i++){
		if(wpoint[i].plus_spec != 0){
			wpoint[j].plus_seq = wpoint[i].plus_seq;
			wpoint[j].plus_value = wpoint[i].plus_value;
			wpoint[j].plus_spec = wpoint[i].plus_spec;
			wpoint[j].minus_seq = wpoint[i].minus_seq;
			wpoint[j].minus_value = wpoint[i].minus_value;
			wpoint[j].minus_spec = wpoint[i].minus_spec;
			wpoint[j].ptop_value = wpoint[i].ptop_value;
			wpoint[j].ptop_time = wpoint[i].ptop_time;
			wpoint[j].interval = wpoint[i].interval;
			j++;
		}
	}
	
	if (lengthpeak > j) {
		memset(&wpoint[j],0x00,(sizeof(WALKPEAK) * (lengthpeak - j)));
	}
	return j;
}

/**
 * @brief Walk Compare
 * @param wpoint walk peak
 * @param prewpoint pre walk peak
 * @param lengthpeak Length Peak
 * @param lengthprepeak Length PrePeak
 * @retval n
 */
short WalkCompare(WALKPEAK wpoint[], PREWALKPEAK prewpoint[], short lengthpeak, short lengthprepeak)
{
	short n = 0;
	short i;
	short j;
	
	if (lengthpeak == 0) {
		return 0;
	}
	
	if (lengthprepeak == 0) {
		return lengthpeak;
	}

	for(i = 0; i < lengthpeak; i++) {
		if(wpoint[i].minus_spec == -1) {
			for(j = 0; j < lengthprepeak; j++) {
				if ((wpoint[i].minus_seq == prewpoint[j].minus_seq) || (wpoint[i].plus_seq == prewpoint[j].plus_seq)) {
					wpoint[i].plus_seq = 0;
					wpoint[i].plus_value = 0;
					wpoint[i].plus_spec = 0;
					wpoint[i].minus_seq = 0;
					wpoint[i].minus_value = 0;
					wpoint[i].minus_spec = 0;
					wpoint[i].ptop_value = 0;
					wpoint[i].ptop_time = 0;
					wpoint[i].interval = 0;
				}
			}
		}
	}

	for (i = 0; i < lengthpeak; i++) {
		if (wpoint[i].plus_spec != 0) {
			wpoint[n].interval = wpoint[i].interval;
			wpoint[n].plus_seq = wpoint[i].plus_seq;
			wpoint[n].plus_value = wpoint[i].plus_value;
			wpoint[n].plus_spec = wpoint[i].plus_spec;
			wpoint[n].minus_seq = wpoint[i].minus_seq;
			wpoint[n].minus_value = wpoint[i].minus_value;
			wpoint[n].minus_spec = wpoint[i].minus_spec;
			wpoint[n].ptop_value = wpoint[i].ptop_value;
			wpoint[n].ptop_time = wpoint[i].ptop_time;
			n++;
		}
	}
	if (lengthpeak > n) {
		memset(&wpoint[n],0x00,(sizeof(WALKPEAK) * (lengthpeak - n)));
	}
	return n;
}

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
short PtopCheck(PEAK peak[], WALKPEAK outwalkpoint[], PRELASTWALKPEAK predata, float ptop_Threshold, short peakcount, short prelastpeak, short pm)
{
	PEAK pluspeak;
	PEAK minuspeak;
	PEAK prepluspeak;
	float ptop_value;
	unsigned short ptop_time;
	unsigned short num = 0;
	short tmpMinusFlag = -1;
	short tmpPlusFlag = -1;
	unsigned short tmpPlus_i;
	unsigned short flag = 1;
	
	if (peakcount == 0) {
		return 0;
	}

	for (short i = 0; i < peakcount; i++) {
		switch(flag) {
		case 1:
/*******************New Algo********************/
			if (peak[i].dir < 0 && tmpMinusFlag != -1){
				if(peak[i].peak <= minuspeak.peak){
					minuspeak.peak = peak[i].peak;
					minuspeak.indexnum = peak[i].indexnum;
					minuspeak.dir = peak[i].dir;
					tmpMinusFlag = 1;
				}
				if((uint16_t)abs((((int)tmpPlus_i - (int)peak[i].indexnum) % MODULO)) > pm){
					minuspeak.peak = peak[i].peak;
					minuspeak.indexnum = peak[i].indexnum;
					minuspeak.dir = peak[i].dir;
					tmpMinusFlag = 1;
				}
			}
/**********************************************************/
			if (peak[i].dir > 0) {
				flag = 2;
				pluspeak.indexnum = peak[i].indexnum;
				pluspeak.peak = peak[i].peak;
				pluspeak.dir = peak[i].dir;
				tmpPlusFlag = 1;
				tmpPlus_i = peak[i].indexnum;
			}
			break;
		case 2:
/*******************New Algo Add********************/
			if(peak[i].dir > 0 && tmpPlusFlag != -1){
				if(peak[i].peak >= pluspeak.peak){
					pluspeak.peak = peak[i].peak;
					pluspeak.dir = peak[i].dir;
					pluspeak.indexnum = peak[i].indexnum;
					tmpPlusFlag = 1;
					tmpPlus_i = peak[i].indexnum;
				}
				if((uint16_t)abs(((((int)peak[i].indexnum - (int)tmpPlus_i)) % MODULO)) > pm){
					pluspeak.indexnum = peak[i].indexnum;
					pluspeak.peak = peak[i].peak;
					pluspeak.dir = peak[i].dir;
					tmpPlusFlag = 1;
					tmpPlus_i = peak[i].indexnum;
				}
			}
/**********************************************************/	
				
			if (peak[i].dir < 0) {
				minuspeak.indexnum = peak[i].indexnum;
				minuspeak.peak = peak[i].peak;
				minuspeak.dir = peak[i].dir;
				tmpMinusFlag = 1;
				ptop_value = fabs(pluspeak.peak - minuspeak.peak);
				if (ptop_value > ptop_Threshold) {
					if(i != peakcount){
						if(peak[i+1].dir < 0){
							if((peak[i+1].peak <= minuspeak.peak) && ((uint16_t)(((int)peak[i + 1].indexnum - (int)minuspeak.indexnum) % MODULO)) < pm){
								minuspeak.indexnum = peak[i + 1].indexnum;
								minuspeak.peak = peak[i + 1].peak;
								minuspeak.dir = peak[i + 1].dir;
								tmpMinusFlag = 1;
							}
						}
					}
					ptop_time = (uint16_t)abs((((int)minuspeak.indexnum - (int)pluspeak.indexnum) % MODULO)); /// 100.0;
						
					outwalkpoint[num].plus_seq = pluspeak.indexnum;
					outwalkpoint[num].plus_value = pluspeak.peak;
					outwalkpoint[num].plus_spec = pluspeak.dir;
					outwalkpoint[num].minus_seq = minuspeak.indexnum;
					outwalkpoint[num].minus_value = minuspeak.peak;
					outwalkpoint[num].minus_spec = minuspeak.dir;
					outwalkpoint[num].ptop_value = ptop_value;
					outwalkpoint[num].ptop_time = ptop_time;
					outwalkpoint[num].interval = 0;
					flag = 1;
					num++;
				}
			}
			break;
		}
	}

	float fInterval;
	uint16_t tmpInterval;
	
	if (1 < num) {
		prepluspeak.indexnum = outwalkpoint[0].plus_seq;
		prepluspeak.dir = outwalkpoint[0].plus_spec;
		for (short i = 1; i < num; i++) {
			/*20201106 walk interval change from 100Hz -> 104Hz*/
			tmpInterval =  (uint16_t)(((int)outwalkpoint[i].plus_seq - (int)prepluspeak.indexnum) % MODULO);
			fInterval = (float)tmpInterval / SENSOR_ODR;	//		
			outwalkpoint[i].interval = (uint16_t)(fInterval * 1000.0f);
			
			/**/
			//outwalkpoint[i].interval = (uint16_t)(((int)outwalkpoint[i].plus_seq - (int)prepluspeak.indexnum) % MODULO);
			prepluspeak.indexnum = outwalkpoint[i].plus_seq;
			prepluspeak.dir = outwalkpoint[i].plus_spec;
		}
	}
	if ((num == 1) && (prelastpeak == 1)) {
		/*20201106 walk interval change from 100Hz -> 104Hz*/
		tmpInterval =  (uint16_t)(((int)outwalkpoint[0].plus_seq - (int)predata.plus_seq) % MODULO);
		fInterval = (float)tmpInterval / SENSOR_ODR;	//		
		outwalkpoint[0].interval = (uint16_t)(fInterval * 1000.0f);
		
		/**/
		//outwalkpoint[0].interval = (uint16_t)(((int)outwalkpoint[0].plus_seq - (int)predata.plus_seq) % MODULO);
	}
	return num;
 }

/**
 * @brief Average
 * @param Data data array
 * @param num data array size
 * @retval average
 */
float Average(float Data[], short num) {

	float total = 0;
	float average = 0;
	short i;
	
	for (i = 0; i < num; i++) {
		total += Data[i];
	}
	average = total / (float)num;

	return average;
}

/**
 * @brief Add MAvr SG11 Samples
 * @param MAvr_Table AVRAXES3 Struct
 * @param AccData ACC Data
 * @param SID SID
 * @param index index
 * @retval index
 */
short AddMAvrSG11Samples(AVRAXES3 MAvr_Table[], short AccData, unsigned short SID, short index) {
	MAvr_Table[index].sAccData = AccData;
	MAvr_Table[index].sid = SID;
	index++;
	return index;
}

/**
 * @brief Avr SG Filter
 * @param MAvr_Table AVRAXES3 Struct
 * @param fMAvr FAVRAXES3 Data
 * @retval None
 */
void AvrSGFilter(AVRAXES3 MAvr_Table[], FAVRAXES3 *fMAvr) {
	int fTemp;
	short i;
	
	fTemp = 0;
	
	for (i = 0; i < AVRDIM; i++) {
		fTemp +=  (int)(MAvr_Table[i].sAccData);
	}
	fMAvr->fAccData = (float)fTemp / (float)AVRDIM;//Normalize;
	//fMAvr->fAcAxisZV = (float)fTemp[1] / (float)AVRDIM;//Normalize;
	fMAvr->sid = MAvr_Table[(AVRDIM-1)].sid;
}

/**
 * @brief Add Median 7 Samples
 * @param MMed_Table AVRAXES3 Struct
 * @param AccData ACC Data
 * @param SID SID
 * @param index index
 * @retval index
 */
short AddMedian7Sample(AVRAXES3 MMed_Table[], short AccData, unsigned short SID, short index) {
	MMed_Table[index].sAccData = AccData;
	MMed_Table[index].sid = SID;
	index++;
	return index;
}

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
void JumpFlightTime(WALKPEAK walkpoint[], unsigned short count, STORAGE gXStrage[], float high_th, float minus_thre, SKYJUMPCOUNT *outputCount) {
	unsigned short i;
	unsigned short tmp_i;
	unsigned short startpoint;
	unsigned short endpoint;
	short start_flag = 0;
	short end_flag = 0;
	short jumpcountflag = 0;
	short j;
	short k;
	float tmpMinusValue;
	unsigned short tmpMinusSid;
	short tmp = -10000;
	static short debug_trap_flag = 0;
	
	DEBUG_LOG(LOG_DEBUG,"sky cal");
	if(debug_trap_flag == 1)
	{
		DEBUG_LOG(LOG_DEBUG,"no discovery. next cal. jp count %u",count);
		debug_trap_flag = 0;
	}
	for (i = 0; i < count; i++) {
		if ((walkpoint[i].minus_value < JUMP_MINUS_PEAK) && (JUMP_PTOP_VALUE < (walkpoint[i].ptop_value)) &&(500.0f < walkpoint[i].plus_value)){
			jumpcountflag = 1;  
			if(tmp < walkpoint[i].ptop_time){
				tmp = walkpoint[i].ptop_time;
				startpoint = walkpoint[i].plus_seq;
				endpoint   = walkpoint[i].minus_seq;
				//20180813 new end point search////////////////
				tmpMinusValue = walkpoint[i].minus_value;
				tmp_i = i;
				for(k = tmp_i; k < STRAGE2SEC; k++)
				{
					DEBUG_LOG(LOG_DEBUG,"new end point search");
					DEBUG_LOG(LOG_DEBUG,"walkpiont minus value %d",(int)tmpMinusValue);
					if(gXStrage[k].AccData < tmpMinusValue)
					{
						if((gXStrage[k].index - startpoint) < 100)
						{
							tmpMinusValue = gXStrage[k].AccData;
							tmpMinusSid   = gXStrage[k].index;
							DEBUG_LOG(LOG_INFO,"skyjump endpoint update before %u, after %u",endpoint,tmpMinusSid);
							endpoint = tmpMinusSid;
						}
					}
				}
				///////////////////////////////////////
			}
		}
	}
	if(jumpcountflag == 1){
		for(i = 0; i < STRAGE2SEC; i++){
			if(startpoint == gXStrage[i].index){
				startpoint = i;
				start_flag = 1;
			}
			if(endpoint == gXStrage[i].index){
				endpoint = i;
				end_flag = 1;
			}
			if(start_flag == 1 && end_flag == 1){
				break;
			}
		}
		//start point search
		for(j = startpoint; j >= 0; j--){
			if(j == 0){
				startpoint = gXStrage[j].index;
				break;
			}
			if((gXStrage[j].AccData - gXStrage[j - 1].AccData) < 100 && gXStrage[j].AccData < high_th){
				startpoint = gXStrage[j].index;
				break;
			}
		}
		DEBUG_LOG(LOG_INFO,"algo start point %u",startpoint);
		//end point search
		for(i = endpoint; i < STRAGE2SEC; i++){
			if(i == (STRAGE2SEC - 1)){
				endpoint = gXStrage[i].index;
				//break;
				DEBUG_LOG(LOG_INFO,"end point. no discovery. end point %u",endpoint);
				SkyjumpDataReset();
				debug_trap_flag = 1;
				outputCount->alt = 0;
				outputCount->sid = 0;
				return;
			}
			if(((gXStrage[i + 1].AccData - gXStrage[i].AccData) < 100) && (minus_thre < gXStrage[i].AccData)){
				endpoint = gXStrage[i].index;
				break;
			}
		}
		DEBUG_LOG(LOG_INFO,"algo end point %u",endpoint);
		
		outputCount->alt = (short)((((int)endpoint - (int)startpoint) % MODULO) * 10);
		outputCount->sid = startpoint;
	}
}

/**
 * @brief Tap mode count check.
 * @param walkpoint walk peak
 * @param count count
 * @param outputCount output count
 * @retval None
 */
void TapCountCheck(WALKPEAK walkpoint[], unsigned short count, ALTCOUNT *outputCount)
{
	short i;
	for(i = 0; i < count; i++){
		if(walkpoint[i].minus_value < TAP_MINUS_PEAK && TAP_PTOP < walkpoint[i].ptop_value){
			DEBUG_LOG(LOG_INFO,"tap count peak %u",walkpoint[i].plus_seq);
			outputCount->alt++;
		}
	}
	DEBUG_LOG(LOG_INFO,"cal tap count %u",outputCount->alt);
	return;
}

/**
 * @brief Radder count check
 * @param walkpoint walk peak
 * @param count count
 * @param outputCount output count
 * @retval None
 */
void RADDER_Check(WALKPEAK walkpoint[], unsigned short count, ALTCOUNT *outputCount)
{
	short i;
	for(i = 0; i < count; i++){
		if(RADDER_PTOP < walkpoint[i].ptop_value){
			outputCount->alt++;
		}
	}
	return;
}

/**
 * @brief TELEPORTATION count
 * @param walkpoint walk peak
 * @param count count
 * @param outputCount output count
 * @retval None
 */
void TeleportationCountCheck(WALKPEAK walkpoint[],unsigned short count, ALTCOUNT *outputCount)
{
	short i;
	for(i = 0; i < count; i++){
		if(walkpoint[i].minus_value < UL_MINUS_PEAK && UL_PTOP_PEAK < walkpoint[i].ptop_value){
			outputCount->alt++;
			DEBUG_LOG(LOG_INFO,"telep p_seq %u",walkpoint[i].plus_seq);
		}
	}
	DEBUG_LOG(LOG_INFO,"telep count %u",outputCount->alt);
}

/**
 * @brief Side Agility function
 * @param walkpoint walk peak
 * @param count count
 * @param ave average
 * @param outputCount output count
 * @retval None
 */
void SideAgilityCountCheck(WALKPEAK walkpoint[],unsigned short count, float ave, ALTCOUNT *outputCount)
{
	unsigned short i;
	short interval;
	short re_cal;
	short re_calflag = 0;
	
	for(i = 0; i < count; i++){
		interval = walkpoint[i].interval;
		if(walkpoint[i].minus_value < (650 + ave) && SIDE_PTOP_PEAK < walkpoint[i].ptop_value){
			if(re_calflag == 1){
				interval = walkpoint[i].plus_seq - re_cal;
				re_calflag = 0;
			}
			if((walkpoint[i].ptop_time < 50) && (20 <= interval || interval == 0)){
				re_cal = walkpoint[i].plus_seq;
				re_calflag = 1;
				outputCount->alt++;
				DEBUG_LOG(LOG_INFO,"side p_seq %u",walkpoint[i].plus_seq);
			}
		}
	}
	DEBUG_LOG(LOG_INFO,"side count %u",outputCount->alt);
}

/**
 * @brief Old mode. speed reaction judge function
 * @param walkpoint walk peak
 * @param count count
 * @param storage storage data
 * @param high_th high Threshold
 * @param outputCount output count
 * @retval None
 */
void SpeedReacCheckMod(WALKPEAK walkpoint[], unsigned short count, STORAGE storage[], float high_th, ALTCOUNT *outputCount) {
	unsigned short i;
	unsigned short startpoint;
	short reacflag = 0;
	short j;
	float tmp = -10000;
	
	for(i = 0; i < count; i++) {
		if ((walkpoint[i].minus_value < REAC_JUMP_MINUS_PEAK) && ((walkpoint[i].ptop_value) > REAC_JUMP_PTOP_VALUE) &&(walkpoint[i].plus_value > 500)){
			reacflag = 1;
			if(tmp < walkpoint[i].ptop_value){
				tmp = walkpoint[i].ptop_value;
				startpoint = walkpoint[i].plus_seq;
			}
		}
	}
	if(reacflag == 1){
		for(i = 0; i < STRAGE2SEC; i++){
			if(startpoint == storage[i].index){
				startpoint = i;
				break;
			}
		}
		for(j = (short)startpoint; j >= 0; j--){
			if(j == 0){
				outputCount->alt = storage[j].index;
				break;
			}
			if(((storage[j].AccData - storage[j-1].AccData) < 100) && storage[j].AccData < high_th){
				outputCount->alt = storage[j].index;
				break;
			}
		}
		if((AVRDIM + MEDIAN_NUM - 2) <= outputCount->alt){
			outputCount->alt -= (AVRDIM + MEDIAN_NUM - 2);
		}
	}
}

/**
 * @brief Walk Run Dash judge
 * @param walkpoint walk peak
 * @param count count
 * @param run_up_limits run up limit
 * @param dash_up_limits dash up limit
 * @param walkcount walk count
 * @retval None
 */
void WalkOrRun(WALKPEAK walkpoint[], unsigned short count,unsigned short run_up_limits, unsigned short dash_up_limits, DAILYCOUNT *walkcount) {
	unsigned short i;
	
	DEBUG_LOG(LOG_DEBUG,"run th %u, dash th %u",run_up_limits,dash_up_limits);
	
	if(count == 0){
		return;
	}
	
	if(count == 1){
		if(walkpoint[0].interval != 0){
			if(run_up_limits < walkpoint[0].interval && walkpoint[0].interval < WALK_LIMIT){
				walkcount->walk++;  
			}else if(walkpoint[0].interval <= run_up_limits && dash_up_limits < walkpoint[0].interval){
				walkcount->run++;
				
			}else if(walkpoint[0].interval <= dash_up_limits && DASH_LIMIT < walkpoint[0].interval){
				walkcount->dash++;
			}				
		}else if(walkpoint[0].interval == 0){
			if(WALK_LOW_PTOP_THRESHOLD <= walkpoint[0].ptop_value && walkpoint[0].ptop_value < WALK_PTOP_THRESHOLD ){
				walkcount->walk++;
			}else if(WALK_PTOP_THRESHOLD <= walkpoint[0].ptop_value){
				walkcount->run++;
			}else{
			}
		}
	}else{
		if(walkpoint[0].interval == 0){
			for(i = 1; i < count; i++){
				if(run_up_limits < walkpoint[i].interval && walkpoint[i].interval < WALK_LIMIT){
					walkcount->walk++;
					if(i == 1){
						walkcount->walk++;
					}
				}else if(walkpoint[i].interval <= run_up_limits && dash_up_limits < walkpoint[i].interval){
					walkcount->run++;
					if(i == 1){
						walkcount->run++;
					}
				}else if(walkpoint[i].interval <= dash_up_limits && DASH_LIMIT < walkpoint[i].interval){
					walkcount->dash++;
					if(i == 1){
						walkcount->dash++;
					}
				}else{
				}
			}
		}else{
			for(i = 0; i < count; i++){
				if(run_up_limits < walkpoint[i].interval && walkpoint[i].interval < WALK_LIMIT){
					walkcount->walk++;
				}else if(walkpoint[i].interval <= run_up_limits && dash_up_limits < walkpoint[i].interval){
					walkcount->run++;
				}else if(walkpoint[i].interval <= dash_up_limits && DASH_LIMIT < walkpoint[i].interval){
					walkcount->dash++;
				}else{
				}
			}
		}
	}
}

/**
 * @brief 1Axis Peak Detect
 * @param gXStrage storage data
 * @param peak Peak
 * @param flag flag
 * @param threshold Threshold
 * @param samplecount sample count
 * @retval peakcount
 */
short PeakPointDetect1Axis(STORAGE gXStrage[], PEAK peak[], short flag, THRESHOLD threshold, short samplecount)
{
	float maxvalue = -100000.0f;
	float minvalue = 100000.0f;
	unsigned short maxseq = 0;
	unsigned short minseq = 0;
	short pOK = 0;
	short nOK = 0;
	short ppeakDM     = 0;
	short npeakDM     = 0;
	short peak_search = 1;
	short peakcount = 0;
	
	if(flag == PEAK_DETECT_X_AXIS){
		for (short i = 1; i < (samplecount - 1); i++) {
			if (peak_search == 1) {
				if (ppeakDM == 0) {
					maxseq = 0;
					maxvalue = -100000.0f;
					ppeakDM = 1;
					pOK = 0;
				}

				if (ppeakDM == 1) {
					if (threshold.xp_high < gXStrage[i].AccData) {
						if (((gXStrage[i].AccData - gXStrage[i-1].AccData) >= 0) && ((gXStrage[i].AccData - gXStrage[i+1].AccData) > 0)) {
							if (maxvalue < gXStrage[i].AccData) {
								maxvalue = gXStrage[i].AccData;
								maxseq = gXStrage[i].index;
								pOK = 1;
							}
						}
					}
				}
				if ((threshold.xp_low > gXStrage[i].AccData) && (pOK == 1)) {
					if (ppeakDM == 1) {
						peak[peakcount].indexnum = maxseq;
						peak[peakcount].peak = maxvalue;
						peak[peakcount].dir = 1;
						peakcount++;
						ppeakDM = 0;
						peak_search = 2;
						pOK = 0;
					}
				}
				if (threshold.xn_low > gXStrage[i].AccData) {
					peak_search = 2;
					npeakDM = 1;
					minseq = gXStrage[i].index;
					minvalue = gXStrage[i].AccData;
					maxseq = 0;
					maxvalue = -100000.0f;
					ppeakDM = 0;
					pOK = 0;
				}
			}
			
			if (peak_search == 2) {
				if (npeakDM == 0) {
					minseq = 0;
					minvalue = 100000.0f;
					npeakDM = 1;
					nOK = 0;
				}
				if (npeakDM == 1) {
					if (threshold.xn_low > gXStrage[i].AccData) {
						if (((gXStrage[i].AccData - gXStrage[i-1].AccData) <= 0) && ((gXStrage[i].AccData - gXStrage[i+1].AccData) < 0)) {
							if (minvalue > gXStrage[i].AccData) {
								minvalue = gXStrage[i].AccData;
								minseq = gXStrage[i].index;
								nOK = 1;
							}
						}
					}
				}
				if ((threshold.xn_high < gXStrage[i].AccData) && (nOK == 1)) {
					if (npeakDM == 1) {
						peak[peakcount].indexnum = minseq;
						peak[peakcount].peak = minvalue;
						peak[peakcount].dir = -1;
						peakcount++;
						npeakDM = 0;
						peak_search = 1;
						nOK = 0;
					}
				}
				if (threshold.xp_high < gXStrage[i].AccData) {
					peak_search = 1;
					ppeakDM = 1;
					maxseq = gXStrage[i].index;
					maxvalue = gXStrage[i].AccData;
					npeakDM = 0;
					minseq = 0;
					minvalue = 100000.0f;
					nOK = 0;
				}
			}
		}
	//Z-Axis mode
	}else if(flag == PEAK_DETECT_Z_AXIS){
		for (short i = 1; i < (samplecount - 1); i++) {
			if (peak_search == 1) {
				if (ppeakDM == 0) {
					maxseq = 0;
					maxvalue = -100000.0f;
					ppeakDM = 1;
					pOK = 0;
				}

				if (ppeakDM == 1) {
					if (threshold.zp_high < gXStrage[i].AccData) {
						if (((gXStrage[i].AccData - gXStrage[i-1].AccData) >= 0) && ((gXStrage[i].AccData - gXStrage[i+1].AccData) > 0)) {
							if (maxvalue < gXStrage[i].AccData) {
								maxvalue = gXStrage[i].AccData;
								maxseq = gXStrage[i].index;
								pOK = 1;
							}
						}
					}
				}
				if ((threshold.zp_low > gXStrage[i].AccData) && (pOK == 1)) {
					if (ppeakDM == 1) {
						peak[peakcount].indexnum = maxseq;
						peak[peakcount].peak = maxvalue;
						peak[peakcount].dir = 1;
						peakcount++;
						ppeakDM = 0;
						peak_search = 2;
						pOK = 0;
					}
				}
				if (threshold.zn_low > gXStrage[i].AccData) {
					peak_search = 2;
					npeakDM = 1;
					minseq = gXStrage[i].index;
					minvalue = gXStrage[i].AccData;
					ppeakDM = 0;
					maxseq = 0;
					maxvalue = -100000.0f;
					pOK = 0;
				}
			}
			if (peak_search == 2) {
				if (npeakDM == 0) {
					minseq = 0;
					minvalue = 100000.0f;
					npeakDM = 1;
					nOK = 0;
				}
				if (npeakDM == 1) {
					if (threshold.zn_low > gXStrage[i].AccData) {
						if (((gXStrage[i].AccData - gXStrage[i-1].AccData) <= 0) && ((gXStrage[i].AccData - gXStrage[i+1].AccData) < 0)) {
							if (minvalue > gXStrage[i].AccData) {
								minvalue = gXStrage[i].AccData;
								minseq = gXStrage[i].index;
								nOK = 1;
							}
						}
					}
				}
				if ((threshold.zn_high < gXStrage[i].AccData) && (nOK == 1)) {
					if (npeakDM == 1) {
						peak[peakcount].indexnum = minseq;
						peak[peakcount].peak = minvalue;
						peak[peakcount].dir = -1;
						peakcount++;
						npeakDM = 0;
						peak_search = 1;
						nOK = 0;
					}
				}
				if (threshold.zp_high < gXStrage[i].AccData) {
					peak_search = 1;
					ppeakDM = 1;
					maxseq = gXStrage[i].index;
					maxvalue = gXStrage[i].AccData;
					npeakDM = 0;
					nOK = 0;
					minseq = 0;
					minvalue = 100000.0f;
				}
			}
		}
	}

	return peakcount;
}

/**
 * @brief Add 200 Samples
 * @param Strage storage data
 * @param fAccData acc data
 * @param index index
 * @param threshold Threshold
 * @param samplecount sample count
 * @retval samplecount
 */
short iAdd200Samples(STORAGE Strage[], float fAccData, unsigned short index, short samplecount) {
	Strage[samplecount].AccData = fAccData;
	Strage[samplecount].index = index;
	samplecount++;
	return samplecount;
}

/**
 * @brief Sort Dec
 * @param a src value
 * @param b comp value
 * @retval 0 a = b
 * @retval 1 a < b
 * @retval -1 a > b
 */
int SortFuncDec(const void *a, const void *b){
	if(*(float *)a < *(float *)b){
		return 1;
	}
	else if(*(float *)a == *(float *)b){
		return 0;
	}
	return -1;
}

/**
 * @brief Median Filter
 * @param fMedi_Table AVRAXES3 data table
 * @param fMedi midian data
 * @retval None
 */
void MedianFilter(AVRAXES3 fMedi_Table[], AVRAXES3 *fMedi){
	short AccData[MEDIAN_NUM];
	short SortAccData[MEDIAN_NUM];

	for (short i = 0; i < MEDIAN_NUM; i++) {
		AccData[i] = fMedi_Table[i].sAccData;
	}

	dec_sort(&AccData[0], &SortAccData[0],MEDIAN_NUM);
	fMedi->sAccData = SortAccData[(MEDIAN_NUM - 1)/2];
	fMedi->sid = fMedi_Table[(MEDIAN_NUM-1)].sid;
}

/**
 * @brief Dec Sort
 * @param Acc acc data array
 * @param SortAcc acc data array
 * @param num array size
 * @retval None
 */
static void dec_sort(short Acc[], short SortAcc[], short num)
{
	short i,j;
	short tmp;
	for (i = 0; i < num; i++) {
		SortAcc[i] = Acc[i];
	}
	for (i = 0; i < num; i++) {
		for (j = i + 1; j < num; j++) {
			if (SortAcc[i] > SortAcc[j]) {
				tmp = SortAcc[i];
				SortAcc[i] = SortAcc[j];
				SortAcc[j] = tmp;
			}
		}
	}
}

/**
 * @brief walk peak detect check
 * @param walkpoint walk peak
 * @param count count
 * @retval outcount
 */
short WalkPointCheck(WALKPEAK walkpoint[], short count)
{
	short outcount = 0;
	short i;
	
	if (count == 0) {
		return 0;
	}
	
	for (i = 0; i < count; i++) {
		if (walkpoint[i].plus_value != -100000.0f) {
			walkpoint[outcount].interval = walkpoint[i].interval;
			walkpoint[outcount].minus_seq = walkpoint[i].minus_seq;
			walkpoint[outcount].minus_spec = walkpoint[i].minus_spec;
			walkpoint[outcount].minus_value = walkpoint[i].minus_value;
			walkpoint[outcount].plus_seq = walkpoint[i].plus_seq;
			walkpoint[outcount].plus_spec = walkpoint[i].plus_spec;
			walkpoint[outcount].plus_value = walkpoint[i].plus_value;
			walkpoint[outcount].ptop_time = walkpoint[i].ptop_time;
			walkpoint[outcount].ptop_value = walkpoint[i].ptop_value;
			outcount++;
		}
	}
	if (outcount < count) {
		memset(&walkpoint[outcount],0x00,(sizeof(WALKPEAK) * (count - outcount)));
	}
	
	return outcount;
}

/**
 * @brief StartReaction Judge function
 * @param walkpoint walk peak
 * @param walkcount daily count
 * @param ystorage storage data
 * @param high_th high Threshold
 * @retval start_time
 */
short StartReacCheck(WALKPEAK walkpoint[],DAILYCOUNT walkcount,STORAGE ystorage[], float high_th)
{
	short totalCount = 0;
	short j;
	unsigned short i;
	unsigned short y_start;
	short start_time = 0;
	//short old_start_time = 0;
	float start_reac_pTh = 0;
	
	totalCount = (walkcount.walk + walkcount.run + walkcount.dash);
	
	if(0 < totalCount) {
		for(i = 0; i < STRAGE2SEC; i++){
			if(walkpoint[0].plus_seq == ystorage[i].index){
				y_start = i;
				break;
			}
		}
		
		//20180820 new algo kono. plus threshold value. re_caluculation///.
		start_reac_pTh = walkpoint[0].plus_value * 0.80f;
		//////////////////////////////////////////////////////////////////
		//new start point search
		for(j = (short)y_start; j >= 0; j--){
			DEBUG_LOG(LOG_DEBUG,"s_p serach");
			if(j == 0){
				start_time = ystorage[j].index;
				DEBUG_LOG(LOG_INFO,"new start reac point %u",start_time);
				break;
			}
			//if(((ystorage[j].AccData - ystorage[j-1].AccData) < 100) && (ystorage[j].AccData < high_th)){
			if(((ystorage[j].AccData - ystorage[j-1].AccData) < 10.0f) && (ystorage[j].AccData < start_reac_pTh)){
				start_time = ystorage[j].index;
				break;
			}
		}
		
		//DEBUG_LOG(LOG_INFO,"new b reac point %u\r\n old b reac point %u",start_time, old_start_time);
		DEBUG_LOG(LOG_INFO,"new b reac point %u",start_time);
		DEBUG_LOG(LOG_INFO,"old th %d", (int)high_th);
		DEBUG_LOG(LOG_INFO,"new th %d", (int)start_reac_pTh);

		if((AVRDIM + MEDIAN_NUM - 2) <= start_time){
			start_time -= (AVRDIM + MEDIAN_NUM - 2);
			DEBUG_LOG(LOG_INFO,"new reac point %u",start_time);
		}
	}
	return start_time;
}

/**
 * @brief Radder Peak point detect
 * @param gXStrage storage data
 * @param peak peak data
 * @param flag flag
 * @param threshold Threshold
 * @param samplecount sample count
 * @retval peakcount
 */
short RadderPeakPointDetect(STORAGE gXStrage[], PEAK peak[],short flag, THRESHOLD threshold, short samplecount)
{
	float maxvalue = -100000.0f;
	float minvalue = 100000.0f;
	unsigned short maxseq = 0;
	unsigned short minseq = 0;
	short pOK = 0;
	short nOK = 0;
	short ppeakDM     = 0;
	short npeakDM     = 0;
	short peak_search = 1;
	short peakcount = 0;

	for (short i = 1; i < (samplecount - 1); i++) {
		if (peak_search == 1) {
			if (ppeakDM == 0) {
				maxseq = 0;
				maxvalue = -100000.0f;
				ppeakDM = 1;
				pOK = 0;
			}

			if (ppeakDM == 1) {
				if (threshold.xp_high < gXStrage[i].AccData) {
					if ((0 <= ( gXStrage[i].AccData - gXStrage[i-1].AccData)) && (0 < (gXStrage[i].AccData - gXStrage[i+1].AccData))) {
						if (maxvalue < gXStrage[i].AccData) {
							maxvalue = gXStrage[i].AccData;
							maxseq = gXStrage[i].index;
							pOK = 1;
						}
					}
				}
			}
			if ((gXStrage[i].AccData < threshold.xp_low) && (pOK == 1)) {
				if (ppeakDM == 1) {
					peak[peakcount].indexnum = maxseq;
					peak[peakcount].peak = maxvalue;
					peak[peakcount].dir = 1;
					peakcount++;
					ppeakDM = 0;
					peak_search = 2;
					pOK = 0;
				}
			}
			if (gXStrage[i].AccData < threshold.xn_low) {
				peak_search = 2;
				npeakDM = 0;
				minseq = 0;
				minvalue = 100000.0f;
				ppeakDM = 0;
				maxseq = 0;
				maxvalue = -100000.0f;
				pOK = 0;
			}
		}
			
		if (peak_search == 2) {
			if (npeakDM == 0) {
				minseq = 0;
				minvalue = 100000.0f;
				npeakDM = 1;
				nOK = 0;
			}
			if (npeakDM == 1) {
				if (threshold.xn_low > gXStrage[i].AccData) {
					if (((gXStrage[i].AccData - gXStrage[i-1].AccData) <= 0) && ((gXStrage[i].AccData - gXStrage[i+1].AccData) < 0)) {
						if (minvalue > gXStrage[i].AccData) {
							minvalue = gXStrage[i].AccData;
							minseq = gXStrage[i].index;
							nOK = 1;
						}
					}
				}
			}
				
			if ((threshold.xn_high < gXStrage[i].AccData) && (nOK == 1)) {
				if (npeakDM == 1) {
					peak[peakcount].indexnum = minseq;
					peak[peakcount].peak = minvalue;
					peak[peakcount].dir = -1;
					peakcount++;
					npeakDM = 0;
					peak_search = 1;
					nOK = 0;
				}
			}
				
			if (threshold.xp_high < gXStrage[i].AccData) {
				peak_search = 1;
				ppeakDM = 0;
				maxseq = 0;
				maxvalue = -100000.0f;
				npeakDM = 0;
				minseq = 0;
				minvalue = 100000.0f;
				nOK = 0;
			}
		}
	}
	return peakcount;
}

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
short RadderForwardBackPtopCheck(WALKPEAK walkpoint[],PEAK Data[], PRELASTWALKPEAK PreData, float threshold, short count, short precount)
{
	PEAK prePlusPeak;
	PEAK pluspeak;
	PEAK minuspeak;
	float ptop_value;
	short tmpMinusFlag = -1;
	short tmpPlusFlag = -1;
	short flag = 1;
	short time_th = 20;
	short radder_count = 0;
	short outcount = 0;
	unsigned short ptop_time;
	unsigned short tmpPlus_i;
	
	for (short i = 0; i < count; i++) {
		switch(flag) {
		case 1:
			if(0 < Data[i].dir) {
				flag = 2;
				pluspeak.indexnum = Data[i].indexnum;
				pluspeak.peak = Data[i].peak;
				pluspeak.dir = Data[i].dir;
				tmpPlus_i = Data[i].indexnum;
				tmpPlusFlag = 1;
			}
			if (Data[i].dir < 0) {
				flag = 3;
				minuspeak.indexnum = Data[i].indexnum;
				minuspeak.peak = Data[i].peak;
				minuspeak.dir = Data[i].dir;
				tmpMinusFlag = 1;
			}
			break;
// minus peak search
		case 2:
			if((0 < Data[i].dir) && (tmpPlusFlag != -1)){
				if(pluspeak.peak <= Data[i].peak) {
					pluspeak.indexnum = Data[i].indexnum;
					pluspeak.peak = Data[i].peak;
					pluspeak.dir = Data[i].dir;
					tmpPlus_i = Data[i].indexnum;
					tmpPlusFlag = 1;
				}
				if(time_th < (uint16_t)abs((((int)tmpPlus_i - (int)Data[i].indexnum) % MODULO))){
					pluspeak.indexnum = Data[i].indexnum;
					pluspeak.peak = Data[i].peak;
					pluspeak.dir = Data[i].dir;
					tmpPlus_i = Data[i].indexnum;
					tmpPlusFlag = 1;
				}
			}
			
			if(Data[i].dir < 0) {
				minuspeak.indexnum = Data[i].indexnum;
				minuspeak.peak = Data[i].peak;
				minuspeak.dir = Data[i].dir;
				ptop_value = fabs((pluspeak.peak - minuspeak.peak));
				if(ptop_value > threshold) {
					if (i != count) {
						if(Data[i + 1].dir < 0) {
							if((Data[i+1].peak <= minuspeak.peak) && (((uint16_t)abs(((int)Data[i+1].indexnum - (int)minuspeak.indexnum) % MODULO)) < 10)) {
								minuspeak.indexnum = Data[i + 1].indexnum;
								minuspeak.peak = Data[i + 1].peak;
								minuspeak.dir = Data[i + 1].dir;
								//i++;
							}
						}
					}
					ptop_time = (uint16_t)abs((((int)pluspeak.indexnum - (int)minuspeak.indexnum) % MODULO));
					walkpoint[radder_count].plus_seq = pluspeak.indexnum;
					walkpoint[radder_count].plus_spec = pluspeak.dir;
					walkpoint[radder_count].plus_value = pluspeak.peak;
					walkpoint[radder_count].minus_seq = minuspeak.indexnum;
					walkpoint[radder_count].minus_spec = minuspeak.dir;
					walkpoint[radder_count].minus_value = minuspeak.peak;
					walkpoint[radder_count].ptop_value = ptop_value;
					walkpoint[radder_count].ptop_time = ptop_time;
					walkpoint[radder_count].interval = 0;
					radder_count++;
					flag = 1;
				}
			}
			break;
//plus peak search
		case 3:
			if((Data[i].dir < 0) && (tmpMinusFlag != -1)) {
				if(Data[i].peak <= minuspeak.peak) {
					minuspeak.indexnum = Data[i].indexnum;
					minuspeak.peak = Data[i].peak;
					minuspeak.dir = Data[i].dir;
					tmpMinusFlag = 1;
				}
				if( 50 < ((uint16_t)abs(((int)tmpPlus_i - (int)Data[i].indexnum)%MODULO))) {
					minuspeak.indexnum = Data[i].indexnum;
					minuspeak.peak = Data[i].peak;
					minuspeak.dir = Data[i].dir;
					tmpMinusFlag = 1;
				}
			}
			if(0 < Data[i].dir) {
				pluspeak.indexnum = Data[i].indexnum;
				pluspeak.peak = Data[i].peak;
				pluspeak.dir = Data[i].dir;
				ptop_value = fabs(pluspeak.peak - minuspeak.peak);
				if(ptop_value > threshold) {
					if(i != count) {
						if(0 < Data[i+1].dir) {
							if((Data[i+1].peak >= pluspeak.peak) && ((uint16_t)abs((((int)Data[i+1].indexnum - (int)pluspeak.indexnum) % MODULO))) < 10) {
								pluspeak.indexnum = Data[i + 1].indexnum;
								pluspeak.peak = Data[i + 1].peak;
								pluspeak.dir = Data[i + 1].dir;
							}						
						}	
					}
					ptop_time = (uint16_t)(abs(((int)pluspeak.indexnum - (int)minuspeak.indexnum) % MODULO));
					walkpoint[radder_count].plus_seq = pluspeak.indexnum;
					walkpoint[radder_count].plus_spec = pluspeak.dir;
					walkpoint[radder_count].plus_value = pluspeak.peak;
					walkpoint[radder_count].minus_seq = minuspeak.indexnum;
					walkpoint[radder_count].minus_spec = minuspeak.dir;
					walkpoint[radder_count].minus_value = minuspeak.peak;
					walkpoint[radder_count].ptop_value = ptop_value;
					walkpoint[radder_count].ptop_time = ptop_time;
					walkpoint[radder_count].interval = 0;
					radder_count++;
					flag = 1;
				}
			}
			break;
		}
	}

	for(short i = 0; i < radder_count; i++) {
		if(walkpoint[i].plus_spec != 0) {
			walkpoint[outcount].plus_seq = walkpoint[i].plus_seq ;
			walkpoint[outcount].plus_spec = walkpoint[i].plus_spec ;
			walkpoint[outcount].plus_value = walkpoint[i].plus_value;
			walkpoint[outcount].minus_seq = walkpoint[i].minus_seq;
			walkpoint[outcount].minus_spec = walkpoint[i].minus_spec;
			walkpoint[outcount].minus_value = walkpoint[i].minus_value ;
			walkpoint[outcount].ptop_value = walkpoint[i].ptop_value;
			walkpoint[outcount].ptop_time = walkpoint[i].ptop_time;
			outcount++;
		}
	}
	
	if(outcount < radder_count) {
		memset(&walkpoint[outcount],0x00,(sizeof(WALKPEAK) * (radder_count - outcount)));
	}
	
	if (1 < outcount){
		prePlusPeak.indexnum  = walkpoint[0].plus_seq;
		prePlusPeak.dir = walkpoint[0].plus_spec;
		for(short j = 1; j < outcount; j++) {
			walkpoint[j].interval = (uint16_t)abs(((int)walkpoint[j].plus_seq - (int)prePlusPeak.indexnum) % MODULO);
			prePlusPeak.indexnum = walkpoint[j].plus_seq;
			prePlusPeak.dir = walkpoint[j].plus_spec;
		}
	}
	
	if((1 <= outcount) && (precount == 1)) {
		walkpoint[0].interval = (uint16_t)abs((((int)walkpoint[0].plus_seq - (int)PreData.plus_seq) % MODULO));
	}
	return outcount;
}

