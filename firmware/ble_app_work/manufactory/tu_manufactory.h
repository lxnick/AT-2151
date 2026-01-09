
#ifndef __TEST_DTM_H__
#define __TEST_DTM_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MANUFACTORY_TEST        (1)

/**@defgroup Manufactory Test initialization definition.
 * @{ */
#define PIN_BATT_ADC            (5)
#define PIN_PCB                 (24)
#define PIN_UART_RX             (29)
#define PIN_UART_TX             (30)

#define TIMES_ENTRY_MANU_TEST	(20)
/**@} */

void TU_check_manufactory_mode(void);
void TU_check_dtm_mode(void);

#ifdef __cplusplus
}
#endif

#endif	// __TEST_DTM_H__
