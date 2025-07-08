#!/usr/bin/env python3
# simple_ota_server.py

from http.server import HTTPServer, BaseHTTPRequestHandler
import json
import os

class OTAHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/api/version':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            
            version_info = {
                "version": "1.0.1",
                "size": os.path.getsize("firmware.bin") if os.path.exists("firmware.bin") else 0
            }
            self.wfile.write(json.dumps(version_info).encode())
            
        elif self.path == '/api/firmware':
            if os.path.exists("firmware.bin"):
                self.send_response(200)
                self.send_header('Content-type', 'application/octet-stream')
                self.send_header('Content-Length', str(os.path.getsize("firmware.bin")))
                self.end_headers()
                
                with open("firmware.bin", "rb") as f:
                    self.wfile.write(f.read())
            else:
                self.send_response(404)
                self.end_headers()
        else:
            self.send_response(404)
            self.end_headers()

if __name__ == '__main__':
    server = HTTPServer(('0.0.0.0', 8080), OTAHandler)
    print("OTA Server running on port 8080")
    server.serve_forever()