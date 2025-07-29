#!/usr/bin/env python3

import json
import os
import argparse
import logging
import subprocess
import shutil
import os
from http.server import HTTPServer, BaseHTTPRequestHandler

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

# Default values
DEFAULT_PORT = 8080
BOARD = "esp32s3_devkitc_esp32s3_procpu"
DEFAULT_FIRMWARE_PATH = os.path.join("..", "zephyr-project", "builds", BOARD, "latest", "zephyr.signed.bin")

class OTAHandler(BaseHTTPRequestHandler):
    def __init__(self, *args, version, firmware_path=DEFAULT_FIRMWARE_PATH, **kwargs):
        self.version = version
        self.firmware_path = firmware_path
        super().__init__(*args, **kwargs)
    
    def log_message(self, format, *args):
        logger.info("%s - %s", self.address_string(), format % args)
    
    def do_GET(self):
        if self.path == '/api/version':
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
            response_body = json.dumps(version_info).encode()
            
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.send_header('Content-Length', str(len(response_body)))
            self.send_header('Connection', 'close')
            self.end_headers()
            
            self.wfile.write(response_body)

        elif self.path == '/api/firmware':
            if os.path.exists(self.firmware_path):
                firmware_size = os.path.getsize(self.firmware_path)
                self.send_response(200)
                self.send_header('Content-type', 'application/octet-stream')
                self.send_header('Content-Length', str(firmware_size))
                self.send_header('Connection', 'close') 
                self.end_headers()
                
                logger.info(f"Sending firmware file: {self.firmware_path} ({firmware_size} bytes)")
                with open(self.firmware_path, "rb") as f:
                    self.wfile.write(f.read())
                logger.info("Firmware sent successfully")
            else:
                logger.error(f"Firmware file not found: {self.firmware_path}")
                self.send_response(404)
                self.end_headers()
                self.wfile.write(b"Firmware file not found")
        else:
            logger.warning(f"Unknown path requested: {self.path}")
            self.send_response(404)
            self.send_header('Connection', 'close')
            self.end_headers()
            self.wfile.write(b"Not found")

def run_server(version, port=DEFAULT_PORT, firmware_path=DEFAULT_FIRMWARE_PATH):
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


def get_image_version(image_path: str) -> str:
    if not shutil.which("imgtool"):
        raise RuntimeError(
            "imgtool not found. Please install it (e.g., 'pip install imgtool') "
            "and ensure it is in your system's PATH."
        )
    
    if not os.path.isfile(image_path):
        raise FileNotFoundError(f"Image file not found at: {image_path}")
    
    command = ["imgtool", "verify", image_path]

    try:
        output = subprocess.run(
            command,
            capture_output=True,  # Capture stdout and stderr
            text=True,            # Decode output as text
            check=True            # Raise CalledProcessError for non-zero exit codes
        )

        for line in output.stdout.splitlines():
            if line.strip().startswith("Image version:"):
                raw_version_str = line.split(":", 1)[1].strip()
                clean_version = raw_version_str.split("+", 1)[0]

                return clean_version

    except subprocess.CalledProcessError as e:
        error_message = e.stderr.strip()
        raise RuntimeError(
            f"imgtool failed to get version for '{image_path}'.\n"
            f"Exit Code: {e.returncode}\n"
            f"Error: {error_message}"
        )


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='OTA Update Server')
    parser.add_argument('--port', type=int, default=DEFAULT_PORT, help='Server port')
    parser.add_argument('--firmware', default=DEFAULT_FIRMWARE_PATH, help='Path to firmware binary')
    parser.add_argument('--verbose', action='store_true', help='Enable verbose logging')
    
    args = parser.parse_args()
    
    if args.verbose:
        logger.setLevel(logging.DEBUG)
    try:
        version_number = get_image_version(args.firmware)
        run_server(version_number, args.port, args.firmware)
    except Exception as e:
        print(e)
        pass
