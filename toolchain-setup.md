# Toolchain setup 
需要安裝以下套件
* Compiler -- ARM GNU Tools
    gcc-arm-none-eabi-10.3-2021.10-win32.exe
* SWD Programming Tools -- nRF5x Command Line Tools
    nrf-command-line-tools-10.24.2-x64.exe
* nRF5 SDK
    nRF5_SDK_17.1.0_ddde560.zip

這裡有一個安裝的過程可以參考
[Installation reference](https://learn.sparkfun.com/tutorials/nrf52840-advanced-development-with-the-nrf5-sdk/all)

# Compiler -- ARM GNU Tools
* gcc-arm-none-eabi-10.3-2021.10-win32.exe
[Download Link](https://developer.arm.com/downloads/-/gnu-rm)
預設安裝路徑在 "C:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2021.10"

# SWD Programming Tools -- nRF5x Command Line Tools
[Download Link](https://docs.nordicsemi.com/bundle/ug_nrf_cltools/page/UG/cltools/nrf_installation.html)
[Direct Download](https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools/download#infotabs)

# nRF5 SDK
[Download Link](https://www.nordicsemi.com/Products/Development-software/nRF5-SDK/Download)

# Setup Inplace Build environment
## 1. Modify SDK toolchain path
修改 "SDK Rroot"\components\toolchain\gcc\Makefile.windows
```
GNU_INSTALL_ROOT := C:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10/bin/
GNU_VERSION := 10.3.1
```
GNU_VERSION 可以由以下指令得出 
```
arm-none-eabi-gcc --version 
```
## 2. In Place Build
### 2.1 Copy project from example folder 
for example "SDK_ROOT"/example/examples\ble_peripheral\ble_app_template
### 2.2 Modify Makefile
for example pca10040\s132\armgcc\Makefile
```
From
SDK_ROOT := ../../../../../..
Change to
SDK_ROOT := ../../../../../../nRF5_SDK/nRF5_SDK_17.1.0_ddde560
```


## The J-Link Debugger
Ozone_Windows_V340e_x64.exe
[Ozone](https://www.segger.com/downloads/jlink/#Ozone)
### 1️⃣ 安裝
### 2️⃣ New Debug Project
1. Target Device -> nRF52840_xxAA
2. Debugger -> J-Link/Interface-> SWD
3. Program File -> nrf52840_xxaa.out
4. Load File (Load symbols and code)
New Project Wizard
    -> 