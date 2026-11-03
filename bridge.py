#!/usr/bin/env python3
import http.server
import socketserver
import threading
import serial
import time
import json
import glob
import os
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

def serial_reader_thread():
    global ser, latest_data
    while True:
        port = find_esp_port()
        if not port:
            latest_data["connected"] = False
            time.sleep(2)
            continue
        try:
            print(f"[BRIDGE] Conectare la {port} la 115200 baud...")
            with serial_lock:
                ser = serial.Serial(port, 115200, timeout=1)
            latest_data["connected"] = True
            print(f"[BRIDGE] Conectat cu succes la {port}!")

            while True:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line.startswith("{") and line.endswith("}"):
                    try:
                        parsed = json.loads(line)
                        parsed["connected"] = True
                        latest_data = parsed
                    except Exception:
                        pass
        except Exception as e:
            print(f"[BRIDGE] Eroare serial: {e}")
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
                    ser.write(cmd.encode("utf-8"))
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(b'{"status":"ok"}')
        else:
            super().do_GET()

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
