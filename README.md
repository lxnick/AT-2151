# IOT-BLE-2026
Project notes on IOT-BLE-2026

# Memo 
事情是這樣的, 2025/12/30 下午 1:30, 突然收到指令, 要做一個 IOT+BLE 的裝置. 2026 一月中旬要好.   
這事很突然的, 主晶片我沒甚麼經驗, 連開發環境都要重裝. 還要接一個我沒聽說過的 G-Sensor, 然後主機的連線軟體也沒有.
最令人不解的是, PCBA 已經好了, 表示已經進行了一段時間了, 那為什麼拖到現在才告知 ? 

# Project Information
- SOC nRF52832
- IMU ICM-42607

# Function
## Firmware
1. BLE 裝置. 連線後進入睡眠裝置 
2. IMU 中斷(裝置傾倒)->系統喚醒->送出 Notification (Alarm)
3. RTC 中斷(睡眠時間逾時)->系統喚醒->送出 Notification (Alive)
4. Host 可指定 DEVICE 
## Software
1. 掃瞄並顯示連線裝置
2. 顯示裝置代號及裝置代號,時間



