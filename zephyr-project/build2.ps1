[CmdletBinding()]
param(
    [switch]$Clean,
    [switch]$Flash,
    [string]$COMPort
)
$env:MCUBOOT_SIGNING_KEY="C:\Users\SebastianR\Documents\zephyr-keys\root-rsa-2048.pem"
$env:ZEPHYR_SDK_INSTALL_DIR = "C:\zephyr-sdk-0.17.2"
$env:ZEPHYR_BASE = "$PWD\zephyr"

# --- Configuration ---
$Board = "esp32_devkitc/esp32/procpu"
$BoardOverlay = "esp32_devkitc_esp32_procpu.yml"
$SigningKeyFile = $env:MCUBOOT_SIGNING_KEY
# --- End Configuration ---

function Log-Message($Message, $Color = "White") { Write-Host $Message -ForegroundColor $Color }
function Invoke-CommandAndCheck($Command, $ErrorMessage) {
    Log-Message "Executing: $Command" -Color Cyan; Invoke-Expression $Command
    if ($LASTEXITCODE -ne 0) { Log-Message "ERROR: $ErrorMessage" -Color Red; exit 1 }
}

if ($Flash -and -not $COMPort) { Log-Message "ERROR: -COMPort required for -Flash." -Color Red; exit 1 }
if (-not $env:MCUBOOT_SIGNING_KEY) { Log-Message "ERROR: MCUBOOT_SIGNING_KEY environment variable is not set." -Color Red; exit 1}
if (-not (Test-Path $env:MCUBOOT_SIGNING_KEY)) { Log-Message "ERROR: Key file not found at path specified by MCUBOOT_SIGNING_KEY." -Color Red; exit 1}

$BuildDirApp = "build"

if ($Clean) {
    Log-Message "[STEP 1] Cleaning build directory..." -Color Yellow
    if (Test-Path $BuildDirApp) { Remove-Item -Recurse -Force $BuildDirApp }
}

Log-Message "[STEP 2] Building Application and MCUboot child image..." -Color Yellow
$appCmd = "west build -p auto -b $Board -d $BuildDirApp app -- -DESP32_BOOT_PARTITION_TABLE=`"boards/$($BoardOverlay)`" -DCONFIG_BOOTLOADER_MCUBOOT=y"
Invoke-CommandAndCheck -Command $appCmd "Application build failed."

Log-Message "[STEP 3] Merging binaries..." -Color Yellow
$MergedImage = "flash_image.bin"

# --- THE CRITICAL FIX IS HERE: Point to the correct build directories ---
# The ESP-IDF bootloader and partition table are now built as part of the MCUboot child image.
$BootloaderBin = "$BuildDirApp/mcuboot/esp-idf/build/bootloader/bootloader.bin"
$PartitionsBin = "$BuildDirApp/mcuboot/esp-idf/build/partition_table/partition-table.bin"
$McubootBin    = "$BuildDirApp/mcuboot/zephyr/zephyr.bin"
$AppBin        = "$BuildDirApp/zephyr/zephyr.bin" # This is the signed application binary

# Also updated 'merge_bin' to the new recommended 'merge-bin' syntax
$mergeCmd = "esptool --chip esp32 merge-bin -o $MergedImage 0x1000 $BootloaderBin 0x8000 $PartitionsBin 0x10000 $McubootBin 0x20000 $AppBin"
Invoke-CommandAndCheck -Command $mergeCmd "Failed to merge binaries."

if ($Flash) {
    Log-Message "[STEP 4] Flashing to $COMPort..." -Color Yellow
    $flashCmd = "esptool --chip esp32 --port $COMPort --baud 921600 write_flash 0x1000 $MergedImage"
    Invoke-CommandAndCheck -Command $flashCmd "Flashing failed."
}

Log-Message "==============================" -Color Green
Log-Message "  BUILD PROCESS COMPLETE" -Color Green