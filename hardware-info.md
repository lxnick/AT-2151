# AT-2151 hardware design
* nRF52832 
* 6 軸 IMU 
* 外接 RTC 
* Battery Power supply

# Architecture
- SPI/ICM-42607-T (6-axis MEMS)
- I2C/BL5372 RTC
- SWD/Flasher
- Test Points 

# Optional Internal RTC（未使用）
# 外接 RTC（BL5372）
- I2C：SDA / SCL
- INT RTC → nRF GPIO
- 32.768kHz 晶振 X3
# 6-Axis MEMS（ICM-42607-T）
- SPI: MOSI / MISO / SCK / CS
- INT1 / INT2 → nRF GPIO