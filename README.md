# ESP32 Blinky with OTA Updates

This project demonstrates a Zephyr RTOS-based ESP32 application with WiFi connectivity and Over-The-Air (OTA) firmware update capabilities. The application blinks an LED, connects to WiFi, and periodically checks for firmware updates from a local update server.

## Features

- LED blinking with configurable patterns
- WiFi connectivity
- OTA firmware updates
- MCUboot bootloader integration
- Dual-partition flash layout for safe updates
- Automatic rollback on failed updates
- Python-based update server

## Project Structure

- `zephyr-project/`: Main application code
  - `src/`: Source files
  - `include/`: Header files
  - `boards/`: Board-specific overlay files
  - `child_image/`: MCUboot configuration
- `update-server/`: Python OTA update server

## Prerequisites

- Zephyr RTOS development environment
- ESP32 development board
- Python 3.x for the update server

## Building and Flashing

### Initial Setup

1. Set up your Zephyr development environment
2. Clone this repository
3. Configure your WiFi credentials in `zephyr-project/src/app_config.h`

### Building

Use the provided build script:
```
MCUBOOT:
actiavte venv
go to: zephyr-project\bootloader\mcuboot\boot\zephyr
maybe set env: 
$env:ZEPHYR_SDK_INSTALL_DIR = "C:\zephyr-sdk-0.17.2"
west build -b esp32_devkitc/esp32/procpu
west flash
then continue with the app itself
```

```
west build -b esp32_devkitc/esp32/procpu -p always -d build_bootloader bootloader/mcuboot/boot/zephyr -- -DPM_STATIC_YML_FILE="pm_static.yml"
west build -b esp32_devkitc/esp32/procpu -p always -d build_app . -- -DPM_STATIC_YML_FILE="pm_static.yml"

west flash -d build_bootloader
west flash -d build_app
```

```powershell
# Build the firmware
./build.ps1

# Clean and build
./build.ps1 -clean

# Build and flash
./build.ps1 -flash
```

### Manual Building

```bash
# Build for ESP32
west build -b esp32_devkitc_esp32_procpu zephyr-project

# Flash the firmware
west flash
```

## OTA Update Process

### Preparing a New Firmware Version

1. Update the version number in `zephyr-project/src/app_config.h`
2. Build the firmware with the OTA flag:

```powershell
./build.ps1 -ota -version 1.0.1
```

This will:
- Update the version number in the code
- Build the firmware
- Copy the firmware binary to the update-server directory

### Starting the Update Server

```powershell
./build.ps1 -action ota-server
```

Or manually:

```bash
cd update-server
python ota_server.py --version 1.0.1 --firmware firmware.bin
```

### Testing OTA Updates

1. Flash the initial firmware (version 1.0.0) to your ESP32 device
2. Prepare a new firmware version (1.0.1)
3. Start the update server
4. The device will automatically check for updates and download the new firmware
5. After downloading, the device will reboot into the new firmware
6. If the new firmware works correctly, it will be confirmed after 30 seconds
7. If the new firmware fails, the device will automatically roll back to the previous version

## OTA Implementation Details

### Partition Layout

The ESP32 flash is partitioned as follows:

- Bootloader (64KB): MCUboot bootloader
- Storage (32KB): For configuration data
- Application Slot 0 (1MB): Active firmware
- Application Slot 1 (1MB): Update target
- Scratch Area (256KB): For swap-based updates

### Update Workflow

1. Device connects to WiFi
2. Device periodically checks for updates from the server
3. If a new version is available, the device downloads the firmware
4. The firmware is written to the secondary partition
5. The device reboots into the new firmware
6. If the new firmware runs successfully for 30 seconds, it is confirmed
7. If the new firmware fails, the device rolls back to the previous version

### LED Status Indicators

- Normal blinking (1Hz): Regular operation
- Slow blinking (0.5Hz): Downloading firmware
- Fast blinking (10Hz): Applying update
- Rapid blinking (4Hz): Error state

## Troubleshooting

- **WiFi Connection Issues**: Check your WiFi credentials in `app_config.h`
- **Update Server Not Found**: Verify the server IP address in `app_config.h`
- **Flash Errors**: Ensure your ESP32 has sufficient flash memory (4MB recommended)
- **Bootloader Errors**: Check MCUboot configuration in `child_image/mcuboot/prj.conf`

## License

This project is licensed under the Apache License 2.0.