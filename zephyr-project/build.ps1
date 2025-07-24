param(
    [Alias('p', 'clean')]
    [switch]$pristine
)

$env:ZEPHYR_SDK_INSTALL_DIR = "C:\zephyr-sdk-0.17.2"
$env:ZEPHYR_BASE = "$PWD\zephyr"

$westArgs = @(
    "build",
    "-b", "esp32_devkitc/esp32/procpu",
    "--sysbuild",
    "app"
)

if ($Pristine.IsPresent) {
    $westArgs += "-p", "always"
    Write-Host "Pristine build requested. Forcing a clean build." -ForegroundColor Yellow
}
else {
    $westArgs += "-p", "auto"
}

& west $westArgs
imgtool verify .\build\app\zephyr\zephyr.signed.bin

#west build -b esp32_devkitc/esp32/procpu --sysbuild app -p always | Tee-Object -FilePath build.log
#west build -b esp32_devkitc/esp32/procpu --sysbuild -p always -- -DSB_CONFIG_BOOTLOADER_MCUBOOT=y -DSB_CONFIG_BOOT_SIGNATURE_KEY_FILE="sysbuild/mcuboot/root-rsa-2048.pem"

# west flash --esp-device COM10