# model.py
import time

OFFLINE_TIMEOUT = 15.0  # seconds

class DeviceState:
    def __init__(self, address, name):
        self.address = address
        self.name = name
        self.status = None
        self.flags = 0
        self.rssi = None
        self.last_seen = time.time()

    def update(self, status, rssi, flags=0):
        self.status = status
        self.rssi = rssi
        self.flags = flags
        self.last_seen = time.time()

    @property
    def age(self):
        return time.time() - self.last_seen
    @property
    def online(self):
        return self.age < OFFLINE_TIMEOUT