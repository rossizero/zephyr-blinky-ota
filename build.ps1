# ESP32 OTA Build Script
param (
    [string]$action = "build",
    [string]$board = "esp32_devkitc/esp32/procpu",
    [switch]$clean = $false,
    [switch]$flash = $false,
    [switch]$ota = $false,
    [string]$version = ""
)

# Zephyr Umgebung setzen
$env:ZEPHYR_SDK_INSTALL_DIR = "C:\zephyr-sdk-0.17.2"
$env:ZEPHYR_BASE = "$PWD\zephyr-project\zephyr"

$ErrorActionPreference = "Stop"
$projectDir = "zephyr-project"
$buildDir = "build"
$firmwareFile = "firmware.bin"

function Show-Help {
    Write-Host "ESP32 OTA Build Script"
    Write-Host "Usage: ./build.ps1 [options]"
    Write-Host ""
    Write-Host "Options:"
    Write-Host "  -action <action>   Action to perform: build, clean, flash, ota-server"
    Write-Host "  -board <board>     Target board (default: esp32_devkitc_esp32_procpu)"
    Write-Host "  -clean             Clean build directory before building"
    Write-Host "  -flash             Flash after building"
    Write-Host "  -ota               Prepare firmware for OTA update"
    Write-Host "  -version <version> Set firmware version (format: major.minor.patch)"
    Write-Host ""
    Write-Host "Examples:"
    Write-Host "  ./build.ps1                           # Build the firmware"
    Write-Host "  ./build.ps1 -clean                    # Clean and build"
    Write-Host "  ./build.ps1 -flash                    # Build and flash"
    Write-Host "  ./build.ps1 -ota -version 1.0.1       # Build firmware with version 1.0.1 for OTA"
    Write-Host "  ./build.ps1 -action ota-server        # Start OTA server"
}

function Update-Version {
    param (
        [string]$version
    )

    if ($version -eq "") {
        Write-Host "No version specified, using current version"
        return
    }

    $versionParts = $version.Split(".")
    if ($versionParts.Count -ne 3) {
        Write-Error "Invalid version format. Use major.minor.patch (e.g., 1.0.1)"
        exit 1
    }

    $major = $versionParts[0]
    $minor = $versionParts[1]
    $patch = $versionParts[2]

    $configFile = Join-Path $projectDir "src/app_config.h"
    $content = Get-Content $configFile

    $content = $content -replace '#define APP_VERSION_MAJOR \d+', "#define APP_VERSION_MAJOR $major"
    $content = $content -replace '#define APP_VERSION_MINOR \d+', "#define APP_VERSION_MINOR $minor"
    $content = $content -replace '#define APP_VERSION_PATCH \d+', "#define APP_VERSION_PATCH $patch"

    Set-Content -Path $configFile -Value $content
    Write-Host "Updated version to $version in $configFile"
}

function Clean-Build {
    if (Test-Path $buildDir) {
        Write-Host "Cleaning build directory..."
        Remove-Item -Recurse -Force $buildDir
    }
}

function Build-Firmware {
    Write-Host "Building firmware..."
    west build -b $board $projectDir --pristine
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Build failed"
        exit 1
    }
    Write-Host "Build successful"
}

function Flash-Firmware {
    Write-Host "Flashing firmware..."
    west flash
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Flash failed"
        exit 1
    }
    Write-Host "Flash successful"
}

function Prepare-OTA {
    Write-Host "Preparing firmware for OTA update..."
    
    # Copy firmware to update-server directory
    $sourcePath = Join-Path $buildDir "zephyr/zephyr.bin"
    $targetPath = Join-Path "update-server" $firmwareFile
    
    Copy-Item -Path $sourcePath -Destination $targetPath -Force
    Write-Host "Firmware copied to $targetPath"
    
    # Get firmware size
    $fileInfo = Get-Item $targetPath
    $fileSizeKB = [math]::Round($fileInfo.Length / 1024, 2)
    Write-Host "Firmware size: $fileSizeKB KB"
}

function Start-OTAServer {
    $serverScript = Join-Path "update-server" "ota_server.py"
    $firmwarePath = Join-Path "update-server" $firmwareFile
    
    if (-not (Test-Path $firmwarePath)) {
        Write-Error "Firmware file not found: $firmwarePath"
        Write-Host "Build firmware with -ota flag first"
        exit 1
    }
    
    # Get current version from app_config.h
    $configFile = Join-Path $projectDir "src/app_config.h"
    $content = Get-Content $configFile
    
    $major = ($content | Select-String -Pattern '#define APP_VERSION_MAJOR (\d+)').Matches.Groups[1].Value
    $minor = ($content | Select-String -Pattern '#define APP_VERSION_MINOR (\d+)').Matches.Groups[1].Value
    $patch = ($content | Select-String -Pattern '#define APP_VERSION_PATCH (\d+)').Matches.Groups[1].Value
    
    $currentVersion = "$major.$minor.$patch"
    
    # Increment patch version for OTA update
    $newPatch = [int]$patch + 1
    $newVersion = "$major.$minor.$newPatch"
    
    Write-Host "Starting OTA server with firmware version $newVersion"
    python $serverScript --version $newVersion --firmware $firmwarePath --verbose
}

# Main script logic
if ($action -eq "help") {
    Show-Help
    exit 0
}

if ($version -ne "") {
    Update-Version -version $version
}

if ($clean) {
    Clean-Build
}

if ($action -eq "clean") {
    Clean-Build
    exit 0
}

if ($action -eq "ota-server") {
    Start-OTAServer
    exit 0
}

Build-Firmware

if ($flash) {
    Flash-Firmware
}

if ($ota) {
    Prepare-OTA
}

Write-Host "Done!"