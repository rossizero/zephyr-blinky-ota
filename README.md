# ESP32 Blinky with OTA Updates
* Sysbuild intro: https://academy.nordicsemi.com/courses/nrf-connect-sdk-intermediate/lessons/lesson-8-sysbuild/topic/sysbuild-explained/
* List of most SB_CONFIG options: https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/releases_and_maturity/migration/migration_sysbuild.html
* or `west build -t sysbuild_guiconfig`
* also cool: https://docs.nordicsemi.com/bundle/ncs-2.9.0/page/zephyr/build/sysbuild/index.html#zephyr_application_configuration

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
- `update-server/`: Python OTA update server
- `zephyr-project/`: Main application code
  - `app/src`: Source files
  - `app/include/`: Header files
  - `app/boards/`: Board-specific overlay files
  - `app/sysbuild/`: MCUboot configuration

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

```
west sign -t imgtool -- keygen -k root-rsa-2048.pem -t rsa-2048
```

```
west build -t guiconfig
```
### Manual Building

```bash
# Build for ESP32
west build -b esp32_devkitc_esp32_procpu zephyr-project

# Flash the firmware
west flash
```

## OTA Update Process

### Starting the Update Server

```powershell
./build.ps1 -action ota-server
```

Or manually:

```bash
cd update-server
python ota_server.py --version 1.0.1 --firmware firmware.bin
```

```
in /zephyr-project
python -m venv venv
venv/Scripts\Activate.ps1
pip install west
west init .
west update
west zephyr-export
evtl. wg. permissions: pip cache purge
pip install -r zephyr/scripts/requirements.txt
west blobs fetch hal_espressif
```

```
west flash --erase
west flash --esp-device COM10
```

```
pip install imgtool
imgtool keygen -k /keys/[name.pem] -t [type]
imgtool verify .\build\app\zephyr\zephyr.signed.bin -k [keyfile]
```
TODO:
* LEARNING: find out why I can't name the /sysbuild/mcuboot.overlay a board specific name like mcuboot_esp32_devkitc_esp32_procpu.overlay
  * this works, but some options are overridden by SB_CONFIGs from the main sysbuild.conf
* TODO: find out how to set the key.pem path relative without west searching for it in the mcuboot repo /bootloader/mcuboot...
* BIG TODO: find out why signing with rsa and ed25519 doesnt work (IRAM overflow) but using ecdsa 256 it only uses ~33KB IRAM
  * especially why it even doesn't work with the S3 esp32, which should have more RAM available
* MAYDO (seems hard to achieve): try to disable tinycrypt and enable: ```CONFIG_TINYCRYPT=n
CONFIG_MBEDTLS=y
CONFIG_BOOTUTIL_USE_MBED_TLS=y```
* LEARNING: find out if mcuboot.overlay is necessary -> I read that it is, to activate MCUBOOT_BOOTLOADER_MODE_SWAP_SCRATCH in mcuboot.conf and not via SB_MCUBOOT_BOOTLOADER_MODE_SWAP_SCRATCH in sysbuild.conf, but it never worked and made a change...
* LEARNING: mcuboot.conf not merged correctly (e.g when setting encryption type and key path, works only via sysbuild.conf)
  * this is due to some options always taken out of sysbuild.conf, it actually is being merged
  * same with boards/esp32_devkitc_procpu.conf, it is being recognized but has no power to do anything
  * same with /mcuboot/prj.conf
  * but in /app/boards/*.conf for example CONFIG_REBOOT can be overridden...
* /sysbuild/mcuboot/app.overlay correct / what for? (https://docs.nordicsemi.com/bundle/ncs-2.9.0/page/zephyr/build/sysbuild/index.html#zephyr_application_configuration)
* TODO: find out if and why my custom overlays broke the mcuboot upgrade process
* TODO: find out why it only boots from the latest version after the second run/download
* for the c3 board: `error message undefined reference to '__atomic_exchange_4' and undefined reference to '__atomic_fetch_add_4'` (fixed in CMakeLists.txt)
* LEARNING: if app.overlay and /boards/boardname.overlay, app.overlay is being ignored
* find out if there is an UPGRADE_ONLY  SB_CONFIG for mcuboot, since CONFIG_BOOT_UPGRADE_ONLY is ignored at any other place
* LEARNING:
```
in mcuboot.conf
# overflow of 6752 bytes with mbedtls vs 3172 bytes with tinycrypt
CONFIG_BOOT_SIGNATURE_TYPE_ED25519=y
CONFIG_BOOT_ED25519_MBEDTLS=y
CONFIG_BOOT_ED25519_TINYCRYPT=n
CONFIG_BOOT_ED25519_PSA=n
```

* TODO: find out why this error arises when enabling PSA Backand: (even with CONFIG_BOOT_KEY_IMPORT_BYPASS_ASN=y)
```
CMake Error at C:/Users/SebastianR/Desktop/repos/zephyr-blinky-ota/zephyr-project/zephyr/cmake/modules/extensions.cmake:533 (target_sources):
  Cannot find source file:

    /src/asn1parse.c

  Tried extensions .c .C .c++ .cc .cpp .cxx .cu .mpp .m .M .mm .ixx .cppm
  .ccm .cxxm .c++m .h .hh .h++ .hm .hpp .hxx .in .txx .f .F .for .f77 .f90
  .f95 .f03 .hip .ispc
Call Stack (most recent call first):
  CMakeLists.txt:287 (zephyr_library_sources)
```