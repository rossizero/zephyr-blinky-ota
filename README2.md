Create App Skeleton
```
Setup Zephyr: https://docs.zephyrproject.org/latest/develop/getting_started/index.html

west init zephyr-project
cd zephyr-project
west update  # dauert lang ~10min & wird viel ~7GB
west blobs fetch hal_espressif

mkdir app
cd app
mkdir src
mkdir boards

# somewhere create a key:
pip install imgtool
imgtool keygen -k root-rsa-2048.pem -t rsa-2048

# To build and flash the first "factory" image
.\build.ps1 -Clean -Flash -COMPort 10
```


```
in /zephyr-project
python -m venv venv
venv/Scripts\Activate.ps1
pip install west
west init .
west update
west zephyr-export
evtl. wg. permissions: pip cache purge
pip install -r zephyr/scripts/requirements.txt
west blobs fetch hal_espressif
```