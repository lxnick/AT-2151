nrfjprog --recover

nrfjprog --program s140_nrf52_7.2.0_softdevice.hex --sectorerase
nrfjprog --reset

nrfjprog --program _build\nrf52840_xxaa.hex --verify --reset
nrfjprog --reset

@PAUSE