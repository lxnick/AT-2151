# BLE Badge (original AT-2151 )
* Project notes on BLE Badge (original AT-2151 )
* A BLE device + G-sensor + Dash board (Host Web UI)
* Project start @ 2025/12/30

# Project Components
- SOC nRF52832
- IMU ICM-42607
- GUI Dash board

# Toolchain Setup
- SDK nRF5_SDK_17.1.0_ddde560
[Toolchain](./toolchain-setup.md)

# nRF52832 VS nRF52840
|功能/規格 |nRF52832 | nRF52840 |
|:----------|:----------|:----------|
| 處理器   | ARM Cortex-M4F | ARM Cortex-M4F |
| Flash | 512KB/256KB | 1MB|
| RAM | 64KB/32KB | 256KB|
| Wireless | BLE/ANT/2.4GHz | BLE/ANT/2.4GHz, Thread, Zigbeee, 802.15.4 |
| USB | N/A |USB 2.0 |
| Security | Basic AES | CryptoCell-310 |
| Peak Current | ~7.5mA | ~14.8mA |
| Votage Supply | 1.7V~3.6V | 1.7V~5.5V|



# Function Description
## Firmware
1. BLE 裝置. 連線後進入睡眠裝置 
2. IMU 中斷(裝置傾倒)->系統喚醒->送出 Notification (Alarm)
3. RTC 中斷(睡眠時間逾時)->系統喚醒->送出 Notification (Alive)
4. ? Host 可指定 DEVICE 
## Software
1. 掃瞄並顯示連線裝置
2. 顯示裝置代號及裝置代號,時間

# Project Development
## 1. develop BLE device firmware on nRF52840 DK
    * Using armgcc + Makefile + Ozone
### 1.1 leverage from examples\ble_peripheral\ble_app_template
### 1.2 Sync ble_app_template_gcc_nrf52.ld with ble_app_pwr_profiling
    * memory boundary (ble_app_template_gcc_nrf52.ld ) depends on BLE comnfig
    * ble_app_pwr_profiling with 1 customer service fit out requirement
### 1.3 Modify pca10056\s140\config\sdk_config.h
	
	
### 1.4 develop  
    * Add BLE customer service for monitor program to discover
    * Add custom ADV data for connectionless
    * Enter Sleep mode
    * Add Timer to wakeup
    * Add Button as Interrupt wakeup

## 2. Web based dash board 

## 3. Porting to nRF52832 PCBA
## 3.1 Sync ble_app_template_gcc_nrf52.ld with ble_app_pwr_profiling


2. 
## Target 
- nRF52832 PCBA
- SoftDevice S132
- no debug interface
## Develop
- nRF52840 DK
- SoftDevice S140
## Flow
### 1. 在 pca10056 (nRF52840 DK) 下建立 SoftDevice S132 的工作目錄
#### ✅ Phase 1：建立「pca10056/s132」主工作環境
複製 ble_app_template/pca10040/s132 到 ble_app_template/pca10056/s132
以 ble_app_template/pca10056/s132 為開發環境
* Makefile：BOARD := PCA10056
* linker：用 52840 的 linker，但 S132 的 Flash 起始位址
* SoftDevice：仍然用 components/softdevice/s132
#### ✅ Phase 2：日常開發 @ (pca10056/s132)
* BLE advertising / notify
* Button / timer
* power management
#### ✅ Phase 3：轉移到 nRF52832
1️⃣ 複製整個專案
```
pca10056/s132
→ pca10040/s132   （或 my_board/s132）
```
2️⃣ 
* board 定義（LED / Button / I2C pin）
* linker（52832 的 RAM/Flash）
* 👉 BLE / App 邏輯完全不用改

這個 Project 要在 nRF52832 PCBA 上面執行, 沒有 debug interface. 僅有 nRF52840 DK, 所以需要設定

# Resource
"Custom Service Tutorial"
Create Custom Service @ nRF52 SDK
https://github.com/NordicPlayground/nRF5x-custom-ble-service-tutorial

