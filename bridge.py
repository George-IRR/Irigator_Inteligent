#!/usr/bin/env python3
import http.server
import socketserver
import threading
import serial
import time
import json
import glob
import os
import re
import urllib.parse

PORT = 8080
latest_data = {
    "soil": 0,
    "raw": 0,
    "volt": 0.0,
    "flow": 0.0,
    "hz": 0,
    "total": 0.0,
    "pump": 0,
    "connected": False
}
ser = None
serial_lock = threading.Lock()

def find_esp_port():
    ports = glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*")
    return ports[0] if ports else None

def parse_line(line):
    global latest_data
    # 1. Format JSON
    if line.startswith("{") and line.endswith("}"):
        try:
            d = json.loads(line)
            d["connected"] = True
            latest_data.update(d)
            return
        except Exception:
            pass

    # 2. Format Text: [SOL] Raw: 2004 | 1.61V | Umiditate: 68%
    if "[SOL]" in line:
        m = re.search(r"Raw:\s*(\d+).*?([\d\.]+)V.*?Umiditate:\s*(\d+)%", line)
        if m:
            latest_data["raw"] = int(m.group(1))
            latest_data["volt"] = float(m.group(2))
            latest_data["soil"] = int(m.group(3))
            latest_data["connected"] = True

    # [DEBITMETRU] Impulsuri/s: 0 | Debit: 0.00 L/min | Total: 0.0 mL
    if "[DEBITMETRU]" in line:
        m = re.search(r"Impulsuri/s:\s*(\d+).*?Debit:\s*([\d\.]+).*?Total:\s*([\d\.]+)", line)
        if m:
            latest_data["hz"] = int(m.group(1))
            latest_data["flow"] = float(m.group(2))
            latest_data["total"] = float(m.group(3))
            latest_data["connected"] = True

    # [POMPA] Stare: PORNITA (100%) sau OPRITA (0%)
    if "[POMPA]" in line:
        m = re.search(r"\((\d+)%\)", line)
        if m:
            latest_data["pump"] = int(m.group(1))
            latest_data["connected"] = True

def serial_reader_thread():
    global ser, latest_data
    while True:
        port = find_esp_port()
        if not port:
            latest_data["connected"] = False
            time.sleep(1)
            continue
        try:
            with serial_lock:
                ser = serial.Serial(port, 115200, timeout=1)
            latest_data["connected"] = True
            print(f"[BRIDGE] Conectat la {port}!")

            while True:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    parse_line(line)
        except Exception as e:
            latest_data["connected"] = False
            with serial_lock:
                if ser and ser.is_open:
                    ser.close()
                ser = None
            time.sleep(2)

class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=os.path.join(os.path.dirname(__file__), "dashboard"), **kwargs)

    def do_GET(self):
        parsed_url = urllib.parse.urlparse(self.path)
        if parsed_url.path == "/api/data":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(json.dumps(latest_data).encode("utf-8"))
        elif parsed_url.path == "/api/pump":
            query = urllib.parse.parse_qs(parsed_url.query)
            cmd = query.get("cmd", ["0"])[0]
            with serial_lock:
                if ser and ser.is_open:
                    ser.write((cmd + "\n").encode("utf-8"))
                    ser.flush()
            # Actualizam starea optimista
            if cmd == "1":
                latest_data["pump"] = 100
            elif cmd == "2":
                latest_data["pump"] = 50
            elif cmd == "0":
                latest_data["pump"] = 0
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(b'{"status":"ok"}')
        else:
            super().do_GET()

    def log_message(self, format, *args):
        pass # Nu umplem consola de request-uri polling

def main():
    t = threading.Thread(target=serial_reader_thread, daemon=True)
    t.start()

    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("", PORT), Handler) as httpd:
        print(f"==================================================")
        print(f"  AquaSense Dashboard Bridge activ pe:            ")
        print(f"  http://localhost:{PORT}                          ")
        print(f"==================================================")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n[BRIDGE] Oprit.")

if __name__ == "__main__":
    main()
