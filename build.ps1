#!/usr/bin/env powershell
# build.ps1 - Windows Build Script für Zephyr

param(
    [string]$Board = "esp32_devkitc/esp32/appcpu",
    [string]$Action = "build"
)

# Zephyr Environment aktivieren
if (Test-Path "zephyr-env\Scripts\activate.ps1") {
    & .\zephyr-env\Scripts\activate.ps1
}

# Zephyr Umgebung setzen
$env:ZEPHYR_BASE = "$PWD\zephyr"
$env:ZEPHYR_SDK_INSTALL_DIR = "C:\zephyr-sdk-0.17.2"

Write-Host "Building for board: $Board" -ForegroundColor Green

switch ($Action) {
    "build" {
        #west build -b $Board . --pristine
        west build -b esp32_devkitc/esp32/procpu --pristine
    }
    "flash" {
        west flash --bossac="C:\Program Files (x86)\BOSSA\bossac.exe" --bossac-port="COM9"
    }
    "clean" {
        Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
    }
    "menuconfig" {
        west build -t menuconfig
    }
    "ota" {
        west build -t app_signed_bin
        Copy-Item "build\zephyr\app_signed.bin" "firmware.bin"
        Write-Host "OTA firmware ready: firmware.bin" -ForegroundColor Yellow
    }
    default {
        Write-Host "Usage: .\build.ps1 [-Board <board>] [-Action <build|flash|clean|menuconfig|ota>]"
    }
}