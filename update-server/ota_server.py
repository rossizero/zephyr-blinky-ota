#!/usr/bin/env python3
# ota_server.py

from http.server import HTTPServer, BaseHTTPRequestHandler
import json
import os
import argparse
import logging

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

# Default values
DEFAULT_PORT = 8080
DEFAULT_VERSION = "1.0.3"
DEFAULT_FIRMWARE_PATH = "zephyr.signed.bin"

class OTAHandler(BaseHTTPRequestHandler):
    def __init__(self, *args, version=DEFAULT_VERSION, firmware_path=DEFAULT_FIRMWARE_PATH, **kwargs):
        self.version = version
        self.firmware_path = firmware_path
        super().__init__(*args, **kwargs)
    
    def log_message(self, format, *args):
        logger.info("%s - %s", self.address_string(), format % args)
    
    def do_GET(self):
        if self.path == '/api/version':
            # --- START: Recommended Changes ---
            firmware_size = 0
            if os.path.exists(self.firmware_path):
                firmware_size = os.path.getsize(self.firmware_path)
            else:
                logger.warning(f"Firmware file not found: {self.firmware_path}")
            
            version_info = {
                "version": self.version,
                "size": firmware_size
            }
            logger.info(f"Sending version info: {version_info}")
            
            # Encode the body *before* sending headers to get its length
            response_body = json.dumps(version_info).encode()
            
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            # 1. Add Content-Length so the client knows when the response is done
            self.send_header('Content-Length', str(len(response_body)))
            # 2. Add Connection: close to politely signal the end of the transaction
            self.send_header('Connection', 'close')
            self.end_headers()
            
            self.wfile.write(response_body)
            # --- END: Recommended Changes ---

        elif self.path == '/api/firmware':
            if os.path.exists(self.firmware_path):
                firmware_size = os.path.getsize(self.firmware_path)
                self.send_response(200)
                self.send_header('Content-type', 'application/octet-stream')
                self.send_header('Content-Length', str(firmware_size))
                # Add this header here as well for consistency
                self.send_header('Connection', 'close') 
                self.end_headers()
                
                logger.info(f"Sending firmware file: {self.firmware_path} ({firmware_size} bytes)")
                with open(self.firmware_path, "rb") as f:
                    self.wfile.write(f.read())
                logger.info("Firmware sent successfully")
            else:
                # ... (rest of the code is fine)
                logger.error(f"Firmware file not found: {self.firmware_path}")
                self.send_response(404)
                self.end_headers()
                self.wfile.write(b"Firmware file not found")
        else:
            logger.warning(f"Unknown path requested: {self.path}")
            self.send_response(404)
            # Also good practice to add Connection: close to error responses
            self.send_header('Connection', 'close')
            self.end_headers()
            self.wfile.write(b"Not found")

def run_server(port=DEFAULT_PORT, version=DEFAULT_VERSION, firmware_path=DEFAULT_FIRMWARE_PATH):
    def handler(*args, **kwargs):
        return OTAHandler(*args, version=version, firmware_path=firmware_path, **kwargs)
    
    server = HTTPServer(('0.0.0.0', port), handler)
    logger.info(f"OTA Server running on port {port}")
    logger.info(f"Serving version: {version}")
    logger.info(f"Firmware path: {firmware_path}")
    
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        logger.info("Server stopped by user")
    finally:
        server.server_close()
        logger.info("Server closed")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='OTA Update Server')
    parser.add_argument('--port', type=int, default=DEFAULT_PORT, help='Server port')
    parser.add_argument('--version', default=DEFAULT_VERSION, help='Firmware version')
    parser.add_argument('--firmware', default=DEFAULT_FIRMWARE_PATH, help='Path to firmware binary')
    parser.add_argument('--verbose', action='store_true', help='Enable verbose logging')
    
    args = parser.parse_args()
    
    if args.verbose:
        logger.setLevel(logging.DEBUG)
    
    run_server(args.port, args.version, args.firmware)