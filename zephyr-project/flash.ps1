param(
    [Alias('p')]
    [string]$port = "COM3"
)

west flash --esp-device $port; putty -serial $port -sercfg 115200