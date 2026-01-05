
# 1. Setup Python
```
py -3.10 -m venv venv
.\venv\Scripts\activate
# pip install --upgrade pip
(venv) python -m pip install --upgrade pip
```
# 2. Install pcakages
```
pip install fastapi uvicorn[standard] bleak
```
# 3 Execute Backend
```
cd C:\Project\AT-2151\webdash
.\venv\Scripts\activate
(venv) uvicorn app:app --host 127.0.0.1 --port 8000
```

# 4. Launch Web GUI
```
http://127.0.0.1:8000/
```

