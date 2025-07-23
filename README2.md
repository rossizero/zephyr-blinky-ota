Create App Skeleton
```
Setup Zephyr: https://docs.zephyrproject.org/latest/develop/getting_started/index.html

west init zephyr-project
cd zephyr-project
west update  # dauert lang ~10min & wird viel ~7GB
west blobs fetch hal_espressif

mkdir app
cd app
mkdir src
mkdir boards

# somewhere create a key:
pip install imgtool
imgtool keygen -k root-rsa-2048.pem -t rsa-2048

# To build and flash the first "factory" image
.\build.ps1 -Clean -Flash -COMPort 10
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

TODO:
* find out why I can't name the /sysbuild/mcuboot.overlay a board specific name like mcuboot_esp32_devkitc_esp32_procpu.overlay
* find out how to set the key.pem path relative without west searching for it in the mcuboot repo /bootloader/mcuboot...
* find out why signing with rsa and ed25519 doesnt work (IRAM overflow) but using ecdsa 256 it only uses ~33KB IRAM
* try to disable tinycrypt and enable: ```CONFIG_TINYCRYPT=n
CONFIG_MBEDTLS=y
CONFIG_BOOTUTIL_USE_MBED_TLS=y```
* find out if mcuboot.overlay is necessary -> I read that it is, to activate MCUBOOT_BOOTLOADER_MODE_SWAP_SCRATCH in mcuboot.conf and not via SB_MCUBOOT_BOOTLOADER_MODE_SWAP_SCRATCH in sysbuild.conf, but it never worked and made a change...
* find out why my custom overlays broke the mcuboot upgrade process
* find out why it only boots from the latest version after the second run/download