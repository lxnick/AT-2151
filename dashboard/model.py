# model.py
import time

class DeviceState:
    def __init__(self, address, name):
        self.address = address
        self.name = name
        self.status = None
        self.rssi = None
        self.last_seen = time.time()

    def update(self, status, rssi):
        self.status = status
        self.rssi = rssi
        self.last_seen = time.time()

    @property
    def age(self):
        return time.time() - self.last_seen
