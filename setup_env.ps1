# setup_env.ps1 - Zephyr Umgebung für Windows

$env:ZEPHYR_BASE = "$PWD\zephyr"
$env:ZEPHYR_SDK_INSTALL_DIR = "C:\zephyr-sdk-0.17.2"
$env:PATH += ";C:\zephyr-sdk-0.17.2\arm-zephyr-eabi\bin"

Write-Host "Zephyr Umgebung konfiguriert:" -ForegroundColor Green
Write-Host "  ZEPHYR_BASE: $env:ZEPHYR_BASE"
Write-Host "  SDK: $env:ZEPHYR_SDK_INSTALL_DIR"

# Virtual Environment aktivieren
if (Test-Path "zephyr-env\Scripts\Activate.ps1") {
    & .\zephyr-env\Scripts\Activate.ps1
    Write-Host "  Python venv aktiviert" -ForegroundColor Green
}