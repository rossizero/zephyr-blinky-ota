# Zephyr blinky ota

## setup
installation
```
Set-ExecutionPolicy Bypass -Scope Process -Force
[System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))

# Zephyr Dependencies installieren
choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System'
choco install ninja gperf python git dtc-msys2


# Python Virtual Environment erstellen
python -m venv venv
venv\Scripts\activate

# West (Zephyr Build Tool) installieren
pip install west

# Zephyr SDK Dependencies
pip install pyelftools

# SDK von https://github.com/zephyrproject-rtos/sdk-ng/releases herunterladen
# z.B. zephyr-sdk-0.17.2_windows-x86_64.7z

# Extrahieren nach C:\zephyr-sdk-0.17.2
# Umgebungsvariable setzen
$env:ZEPHYR_SDK_INSTALL_DIR = "C:\zephyr-sdk-0.17.2"
```
project setup
```
# Zephyr Workspace erstellen
mkdir zephyr-workspace
cd zephyr-workspace

# Zephyr Repository klonen (admin!)
west init .  # dauert bissl
west update  # dauert lang

# Python Requirements installieren
pip install -r zephyr\scripts\requirements.txt
```


# 2. Umgebung aktivieren
.\setup_env.ps1

# 3. Bauen
.\build.ps1 -Action build

# 4. Flashen
.\build.ps1 -Action flash

# 5. OTA Firmware erstellen
.\build.ps1 -Action ota

# 6. OTA Server starten
python ota_server.py