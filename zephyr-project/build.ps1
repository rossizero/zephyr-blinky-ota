param(
    [Alias('p', 'clean')]
    [switch]$pristine,

    [Parameter(Mandatory=$false)]
    [string]$VersionFile = "app/VERSION"
)

$env:ZEPHYR_SDK_INSTALL_DIR = "C:\zephyr-sdk-0.17.2"
$env:ZEPHYR_BASE = "$PWD\zephyr"
$board = "esp32s3_devkitc/esp32s3/procpu" #"esp32c3_devkitc", #"esp32_devkitc/esp32/procpu", "esp32s3_devkitc/esp32s3/procpu"

# 1. Step build the app + mcuboot with sysbuild
$westArgs = @(
    "build",
    "-b", $board,
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

# 2. Step copy zephyr.signed.bin files to /builds/$board with versioned naming for later usage
function Get-Version {
    param([string]$VersionFilePath)
    
    if (-not (Test-Path $VersionFilePath)) {
        Write-Error "VERSION file not found at $VersionFilePath"
        exit 1
    }
    
    $content = Get-Content $VersionFilePath -Raw
    
    $major = if ($content -match 'VERSION_MAJOR\s*=\s*(\d+)') { $Matches[1] } else { $null }
    $minor = if ($content -match 'VERSION_MINOR\s*=\s*(\d+)') { $Matches[1] } else { $null }
    $patch = if ($content -match 'PATCHLEVEL\s*=\s*(\d+)') { $Matches[1] } else { $null }
    $tweak = if ($content -match 'VERSION_TWEAK\s*=\s*(\d+)') { $Matches[1] } else { $null }
    
    if (-not $major -or -not $minor -or -not $patch -or -not $tweak) {
        Write-Error "Could not parse all version components from $VersionFilePath"
        Write-Host "Found: Major=$major, Minor=$minor, Patch=$patch, Tweak=$tweak"
        exit 1
    }
    
    return "${major}.${minor}.${patch}_${tweak}"
}

try {
    $ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
    $SourcePath = Join-Path $ScriptDir "build/app/zephyr/"
    $VersionFilePath = Join-Path $ScriptDir $VersionFile
    $board = $board -replace '/', '_'
    $Version = Get-Version -VersionFilePath $VersionFilePath
    $BinFilePath = Join-Path $SourcePath "zephyr.signed.bin"
    
    if (-not (Test-Path $BinFilePath)) {
        Write-Error "zephyr.signed.bin file not found at $BinFilePath"
        exit 1
    }
    $BuildsDir = Join-Path $ScriptDir "builds"
    $BoardDir = Join-Path $BuildsDir $Board
    $LatestDir = Join-Path $BoardDir "latest"
    
    if (-not (Test-Path $BuildsDir)) {
        New-Item -ItemType Directory -Path $BuildsDir -Force | Out-Null
    }
    
    if (-not (Test-Path $BoardDir)) {
        New-Item -ItemType Directory -Path $BoardDir -Force | Out-Null
    }
    if (-not (Test-Path $LatestDir)) {
        New-Item -ItemType Directory -Path $LatestDir -Force | Out-Null
    }
    
    $NewFileName = "${Board}_${Version}.bin"
    $DestinationFile = Join-Path $BoardDir $NewFileName
    
    Copy-Item -Path $BinFilePath -Destination $DestinationFile -Force

    $LatestDestinationFile = Join-Path $LatestDir "zephyr.signed.bin"
    Copy-Item -Path $BinFilePath -Destination $LatestDestinationFile -Force
} catch {
    Write-Error "Script failed: $($_.Exception.Message)"
    exit 1
}