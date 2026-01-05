# ble_scanner.py
import asyncio
from bleak import BleakScanner
from model import DeviceState

# ====== 依你的協定調整 ======
MANUFACTURER_ID = 0xFFFF
APP_ID = 0x3412          # <- 改成你的 2-byte APP_ID
NAME_PREFIX = "BLE Badge"  # <- 裝置名稱前綴
# ===========================

# 全域裝置表（被 Web 讀取）
devices: dict[str, DeviceState] = {}
_devices_lock = asyncio.Lock()

def parse_manufacturer_data(md: dict[int, bytes]):
    """
    Manufacturer Data 格式（你目前設計）:
    [0] APP_ID_L
    [1] APP_ID_H
    [2] DEVICE_ID
    [3] DEVICE_ID
    [4] DEVICE_ID
    [5] DEVICE_ID
    [6] Event
    [7] Posture
    [9] FLAGS (可選)
    """
    if MANUFACTURER_ID not in md:
        return None

    data = md[MANUFACTURER_ID]
    if len(data) < 8:
        return None

    app_id = data[0] | (data[1] << 8)  # little-endian
    if app_id != APP_ID:
        return None

    device_id = int.from_bytes(data[2:6], "little")       

    event = data[6]
    posture = data[7]
    
    flags = data[3] if len(data) >= 4 else 0
    return device_id, event, posture, flags

async def scan_forever():
    def detection_callback(device, advertisement_data):
        if not device.name:
            return
        if not device.name.startswith(NAME_PREFIX):
            return

        parsed = parse_manufacturer_data(advertisement_data.manufacturer_data)
        if parsed is None:
            return

        device_id, event, posture, flags = parsed
        rssi = advertisement_data.rssi  # ✅ Windows/WinRT 正確來源

        async def update():
            async with _devices_lock:
                if device.address not in devices:
                    devices[device.address] = DeviceState(address=device.address, name=device.name, device_id=device_id)
                devices[device.address].touch(event=event,posture = posture, rssi=rssi, flags=flags)

        # callback 不能 await，丟回 event loop
        asyncio.get_running_loop().create_task(update())

    scanner = BleakScanner(detection_callback)
    await scanner.start()
    try:
        while True:
            await asyncio.sleep(1.0)
    finally:
        await scanner.stop()

async def snapshot():
    async with _devices_lock:
        # 依 name/address 穩定排序
        arr = sorted((d.to_dict() for d in devices.values()), key=lambda x: (x["name"], x["address"]))
    return arr
