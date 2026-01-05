# app.py
import asyncio
from fastapi import FastAPI, WebSocket
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles

from ble_scanner import scan_forever, snapshot

app = FastAPI(title="IOT BLE Web Dashboard")

# 靜態網頁
app.mount("/static", StaticFiles(directory="static"), name="static")

@app.on_event("startup")
async def _startup():
    # 背景啟動 BLE 掃描
    asyncio.create_task(scan_forever())

@app.get("/")
def index():
    return FileResponse("static/index.html")

@app.get("/api/devices")
async def api_devices():
    return await snapshot()

@app.websocket("/ws")
async def ws_devices(ws: WebSocket):
    await ws.accept()
    try:
        while True:
            data = await snapshot()
            await ws.send_json(data)
            await asyncio.sleep(0.25)
    except Exception:
        # client disconnected
        pass
