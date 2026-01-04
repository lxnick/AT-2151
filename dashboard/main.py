# main.py
import asyncio
import time
from rich.live import Live
from rich.table import Table
from scanner import devices, scan_loop
from ui_utils import format_status, format_rssi, format_online

def render_table():
    table = Table(
        title="BLE Badge Monitor",
        header_style="bold magenta",
        show_lines=False
    )

    table.add_column("Name", style="bold")
    table.add_column("Address", style="dim")
    table.add_column("Status")
    table.add_column("RSSI", justify="right")
    table.add_column("Last Seen (s)", justify="right")
    table.add_column("State")

    # 穩定排序（Name → Address）
    for d in sorted(devices.values(), key=lambda x: (x.name, x.address)):
        table.add_row(
            d.name,
            d.address,
            format_status(d.status),
            format_rssi(d.rssi),
            f"{d.age:4.1f}",
            format_online(d.online),
        )

    return table

async def main():
    asyncio.create_task(scan_loop())

    with Live(render_table(), refresh_per_second=4) as live:
        while True:
            live.update(render_table())
            await asyncio.sleep(0.25)

if __name__ == "__main__":
    asyncio.run(main())
