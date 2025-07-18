$env:ZEPHYR_SDK_INSTALL_DIR = "C:\zephyr-sdk-0.17.2"
$env:ZEPHYR_BASE = "$PWD\zephyr"

west build -p auto -b esp32_devkitc/esp32/procpu --sysbuild | Tee-Object -FilePath build.log 

# west zephyr-export muss einmal gemacht werden
#$env:CMAKE_MODULE_PATH = "$env:ZEPHYR_BASE\cmake;$env:ZEPHYR_BASE\share\zephyr-package\cmake;$env:ZEPHYR_BASE\share\sysbuild\cmake"


#west build -b esp32_devkitc/esp32/procpu --sysbuild app -p always | Tee-Object -FilePath build.log 
#west build -b esp32_devkitc/esp32/procpu --sysbuild -p always -- -DSB_CONFIG_BOOTLOADER_MCUBOOT=y -DSB_CONFIG_BOOT_SIGNATURE_KEY_FILE="sysbuild/mcuboot/root-rsa-2048.pem"
#imgtool verify .\build\app\zephyr\zephyr.signed.bin
# west flash --esp-device COM10