	@echo Flashing: s132_nrf52_7.2.0_softdevice.hex
	nrfjprog -f nrf52 --program C:/Project/nRF5_SDK/nRF5_SDK_17.1.0_ddde560/components/softdevice/s132/hex/s132_nrf52_7.2.0_softdevice.hex --sectorerase
	nrfjprog -f nrf52 --reset