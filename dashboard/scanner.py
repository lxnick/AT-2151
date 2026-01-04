# scanner.py
import asyncio
from bleak import BleakScanner
from model import DeviceState

APP_ID = 0x3412
MANUFACTURER_ID = 0xFFFF

devices = {}

def parse_manufacturer_data(md):
    if MANUFACTURER_ID not in md:
        return None
    data = md[MANUFACTURER_ID]
    if len(data) < 2:
        return None

    app_id = data[0] | (data[1] << 8)   # little-endian
    if app_id != APP_ID:
        return None

    device__id = data[2] | (data[3] << 8)   # little-endian
 
    status = data[4]
    motion = data[5]

    return status  # status

async def scan_loop():
    def detection_callback(device, advertisement_data):
        if not device.name:
            return
        if not device.name.startswith("BLE Badge"):
            return

        status = parse_manufacturer_data(advertisement_data.manufacturer_data)
        if status is None:
            return

        if device.address not in devices:
            devices[device.address] = DeviceState(device.address, device.name)

        devices[device.address].update(status, advertisement_data.rssi)

    scanner = BleakScanner(detection_callback)
    await scanner.start()
    while True:
        await asyncio.sleep(1)
