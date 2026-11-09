# model.py
import time
from dataclasses import dataclass, asdict

OFFLINE_TIMEOUT = 15.0  # seconds

@dataclass
class DeviceState:
    address: str
    name: str
    device_id:int
    event: int | None = None
    x: int | None = None
    y: int | None = None
    z: int | None = None
    bat: int | None = None
    rssi: int | None = None
    last_seen: float = 0.0

    def touch(self, event: int,x: int,y: int,z: int,bat: int, rssi: int, flags: int = 0):
        self.event = event
        self.x = x
        self.y = y
        self.z = z
        self.bat = bat                
        self.rssi = rssi
        self.flags = flags
        self.last_seen = time.time()

    @property
    def age(self) -> float:
        return time.time() - self.last_seen if self.last_seen else 9999.0

    @property
    def online(self) -> bool:
        return self.age < OFFLINE_TIMEOUT

    def to_dict(self):
        d = asdict(self)
        d["age"] = round(self.age, 1)
        d["online"] = self.online
        return d
