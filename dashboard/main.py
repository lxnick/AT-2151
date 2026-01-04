# main.py
import asyncio
import time
from rich.live import Live
from rich.table import Table
from scanner import devices, scan_loop

STATUS_MAP = {
    0x00: "NONE",
    0x01: "BOOT",
    0x02: "HEARTBEAT",
    0x03: "INTERRUPT",
}

def render_table():
    table = Table(title="IOT BLE Dashboard")
    table.add_column("Name")
    table.add_column("Address")
    table.add_column("Status")
    table.add_column("RSSI")
    table.add_column("Last Seen (s)")

    now = time.time()
    for d in devices.values():
        table.add_row(
            d.name,
            d.address,
            STATUS_MAP.get(d.status, f"0x{d.status:02X}"),
            str(d.rssi),
            f"{now - d.last_seen:.1f}",
        )
    return table

async def main():
    asyncio.create_task(scan_loop())

    with Live(render_table(), refresh_per_second=2) as live:
        while True:
            live.update(render_table())
            await asyncio.sleep(0.5)

if __name__ == "__main__":
    asyncio.run(main())
