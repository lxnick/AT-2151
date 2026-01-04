# ui_utils.py
from rich.text import Text

STATUS_MAP = {
    0x01: ("BOOT", "cyan"),
    0x02: ("HEARTBEAT", "green"),
    0x03: ("INTERRUPT", "yellow"),
}

def format_status(status):
    if status is None:
        return Text("UNKNOWN", style="dim")
    name, color = STATUS_MAP.get(status, (f"0x{status:02X}", "white"))
    return Text(name, style=color)

def format_rssi(rssi):
    if rssi is None:
        return Text("--", style="dim")
    if rssi > -55:
        return Text(str(rssi), style="green")
    if rssi > -70:
        return Text(str(rssi), style="yellow")
    return Text(str(rssi), style="red")

def format_online(online):
    return Text("ONLINE", style="green") if online else Text("LOST", style="red")
