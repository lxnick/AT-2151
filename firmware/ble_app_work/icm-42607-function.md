
/* Function prototypes ---------------------------------------------------*/
uint32_t AccGyroRegReset( void );
    acc_gyro_read_device_id
    acc_gyro_softreset
    acc_gyro_otp_reload
    set_acc_gyro_mode( MODE_ACC_ONLY_LP );

-----------------------------------------
uint32_t AccGyroInitialize( uint32_t previous_err );
    acc_gyro_odr_fsr
    acc_gyro_fifo_config
    acc_gyro_interrupt_ctrl_set
    acc_gyro_exec_mode_set
    AccGyroClearFifoCount

uint32_t AccGyroWakeUpSetting( uint32_t previous_err );
    acc_gyro_power_down_mode
    acc_gyro_softreset
    acc_gyro_poweroff_wakeup_set
    setup_acc_gyro_gpio_pin_wakeup

uint32_t AccGyroReadIntSrc( uint8_t *x_int, uint8_t *y_int, uint8_t *z_int );
    acc_gyro_read_device_id

uint32_t AccGyroReconfig( uint8_t mode_number );
    AccGyroDisableGpioInt
    set_acc_gyro_mode
    reconfig_operation_mode
    AccGyroSetSid
    AccGyroEnableGpioInt

void AccGyroCalcData( ACC_GYRO_DATA_INFO *acc_gyro_info );
    acc_calculation
    gyro_calculation
    acc_gyro_calculation
    acc_calculation

void AccGyroEnableGpioInt( uint32_t pin );
    nrf_gpio_pin_latch_clear
    nrfx_gpiote_in_event_enable

void AccGyroDisableGpioInt( uint32_t pin );
    nrfx_gpiote_in_event_disable
    nrf_gpio_pin_latch_clear

void AccGyroSetSid( uint16_t sid );
    sd_nvic_critical_region_enter
    sd_nvic_critical_region_exit

uint16_t AccGyroIncSid( void );
    sd_nvic_critical_region_enter
    sd_nvic_critical_region_exit

void AccGyroClearFifoCount( void );
    sd_nvic_critical_region_enter
    sd_nvic_critical_region_exit

void AccGyroIncFifoCount( void );
    sd_nvic_critical_region_enter
    sd_nvic_critical_region_exit

uint16_t AccGyroSubFifoCount( void );
    sd_nvic_critical_region_enter
    sd_nvic_critical_region_exit

uint16_t AccGyroGetFifoCount( void );
    sd_nvic_critical_region_enter
    sd_nvic_critical_region_exit

void AccGyroValidateClearFifo( void );
    FifoClearPopFifo
    AccGyroDisableGpioInt
    AccGyroPopFifo
    AccGyroSubFifoCount
    AccGyroSetSid 
    AccGyroEnableGpioInt
    

uint8_t AccGyroGetSendHeader( uint8_t tag_data );
    get_acc_gyro_mode

void AccGyroSelfTest( void );
