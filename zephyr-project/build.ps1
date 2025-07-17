$env:ZEPHYR_SDK_INSTALL_DIR = "C:\zephyr-sdk-0.17.2"
$env:ZEPHYR_BASE = "$PWD\zephyr"

west build -b esp32_devkitc/esp32/procpu --sysbuild app --pristine
imgtool verify .\build\app\zephyr\zephyr.signed.bin