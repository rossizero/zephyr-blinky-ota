#
# PowerShell build script for Zephyr + MCUboot on ESP32
#
[CmdletBinding()]
param(
    [switch]$Clean,
    [switch]$Flash,
    [string]$COMPort
)

$env:MCUBOOT_SIGNING_KEY="C:\Users\SebastianR\Documents\zephyr-keys\root-rsa-2048.pem"
$env:ZEPHYR_SDK_INSTALL_DIR = "C:\zephyr-sdk-0.17.2"
$env:ZEPHYR_BASE = "$PWD\..\zephyr"

# --- Configuration ---
$Board = "esp32_devkitc/esp32/procpu"
$BoardOverlay = "esp32_devkitc_esp32_procpu.yml"
$SigningKeyFile = $env:MCUBOOT_SIGNING_KEY
# --- End Configuration ---

function Log-Message($Message, $Color = "White") { Write-Host $Message -ForegroundColor $Color }
function Invoke-CommandAndCheck($Command, $ErrorMessage) {
    Log-Message "Executing: $Command" -Color Cyan
    Invoke-Expression $Command
    if ($LASTEXITCODE -ne 0) {
        Log-Message "ERROR: $ErrorMessage" -Color Red; exit 1
    }
}

Log-Message "======================================" -Color Green
Log-Message "  ESP32 OTA Build & Flash Script      " -Color Green
Log-Message "======================================" -Color Green

if ($Flash -and -not $COMPort) { Log-Message "ERROR: -COMPort required for -Flash." -Color Red; exit 1 }
if (-not (Test-Path $SigningKeyFile)) { Log-Message "ERROR: Signing key '$SigningKeyFile' not found." -Color Red; exit 1 }

$BuildDirApp = "build_app"
$BuildDirMcuboot = "build_mcuboot"
$MCUbootSourceDir = Join-Path $env:ZEPHYR_BASE ".." | Join-Path -ChildPath "bootloader/mcuboot/boot/zephyr"

if ($Clean) {
    Log-Message "[STEP 1] Cleaning..." -Color Yellow
    if (Test-Path $BuildDirApp) { Remove-Item -Recurse -Force $BuildDirApp }
    if (Test-Path $BuildDirMcuboot) { Remove-Item -Recurse -Force $BuildDirMcuboot }
}

Log-Message "[STEP 2] Building MCUboot..." -Color Yellow
$mcubootCmd = "west build -p auto -b $Board -d $BuildDirMcuboot $MCUbootSourceDir"
Invoke-CommandAndCheck -Command $mcubootCmd -ErrorMessage "MCUboot build failed."

Log-Message "[STEP 3] Building Application..." -Color Yellow
# We tell west build where the board-specific partition file is located.
# It will automatically find app.overlay in the current directory.
$appCmd = "west build -p auto -b $Board -d $BuildDirApp . -- -DESP32_BOOT_PARTITION_TABLE=`"boards/$($BoardOverlay).yml`""

Invoke-CommandAndCheck -Command $appCmd -ErrorMessage "Application build failed."

Log-Message "[STEP 4] Merging binaries..." -Color Yellow
$BootloaderBin = "$BuildDirApp/esp-idf/build/bootloader/bootloader.bin"
$PartitionsBin = "$BuildDirApp/esp-idf/build/partition_table/partition-table.bin"
$McubootBin = "$BuildDirMcuboot/zephyr/zephyr.bin"
$AppBin = "$BuildDirApp/zephyr/zephyr.bin"
$MergedImage = "flash_image.bin"

# Offsets MUST match partitions.yml!
# 0x1000  -> ESP-IDF bootloader
# 0x8000  -> Partition table
# 0x10000 -> MCUboot
# 0x20000 -> Application Slot 0
$mergeCmd = "esptool --chip esp32 merge-bin -o $MergedImage 0x1000 $BootloaderBin 0x8000 $PartitionsBin 0x10000 $McubootBin 0x20000 $AppBin"
Invoke-CommandAndCheck -Command $mergeCmd -ErrorMessage "Failed to merge binaries."

if ($Flash) {
    Log-Message "[STEP 5] Flashing to $COMPort..." -Color Yellow
    $flashCmd = "esptool --chip esp32 --port $COMPort --baud 921600 write_flash 0x1000 $MergedImage"
    Invoke-CommandAndCheck -Command $flashCmd -ErrorMessage "Flashing failed."
}

Log-Message "======================================" -Color Green
Log-Message "  BUILD PROCESS COMPLETE              " -Color Green
Log-Message "======================================" -Color Green