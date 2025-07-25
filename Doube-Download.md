ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0x1 (POWERON),boot:0x8 (SPI_FAST_FLASH_BOOT)
SPIWP:0xee
mode:DIO, clock div:1
load:0x3fcb5400,len:0x20b0
load:0x403ba400,len:0x9610
load:0x403c6400,len:0x49c
entry 0x403be38c
I (40) soc_init: MCUboot 2nd stage bootloader
I (40) soc_init: compile time Jul 25 2025 14:53:15
W (41) soc_init: Unicore bootloader
I (41) soc_init: chip revision: v0.2
I (44) qio_mode: Enabling default flash chip QIO
I (49) flash_init: Boot SPI Speed : 80MHz
I (52) flash_init: SPI Mode       : QIO
I (56) flash_init: SPI Flash Size : 8MB
I (104) boot: Image index: 0, Swap type: none
I (515) boot: Loading image 0 - slot 0 from flash, area id: 2
I (516) boot: Application start=4037cee0h
I (516) boot: DRAM segment: paddr=0002dca0h, vaddr=3fc91c30h, size=03each ( 16044) load
I (522) boot: IRAM segment: paddr=00020080h, vaddr=40374000h, size=0dc20h ( 56352) load
I (540) boot: IROM segment: paddr=00040000h, vaddr=42000000h, size=5E1D6h (385494) map
I (540) boot: DROM segment: paddr=000a0000h, vaddr=3c060000h, size=0E8A0h ( 59552) map
I (555) boot: libc heap size 207 kB.
I (556) spi_flash: detected chip: generic
I (556) spi_flash: flash io: qio
W (556) spi_flash: Detected size(16384k) larger than the size in the binary image header(8192k). Using the size in the binary image header.

[00:00:00.602,000] <inf> wifi_init: rx ba win: 6
*** Booting Zephyr OS build v4.2.0-rc3-44-g366d45fd34fe ***
[00:00:00.603,000] <inf> blinky: Blinky subsystem initialized - blinking every 1000 ms
[00:00:00.603,000] <inf> wifi_mgmt: WiFi management subsystem initialized
[00:00:00.603,000] <inf> ota_mgmt: Scheduling initial OTA check in 30 seconds.
[00:00:00.603,000] <inf> ota_mgmt: OTA management subsystem initialized
[00:00:00.603,000] <inf> main: ====================================
[00:00:00.603,000] <inf> main:    ESP32 OTA Application Booting
[00:00:00.603,000] <inf> main: ====================================
[00:00:00.603,000] <inf> main: Running a confirmed image.
[00:00:00.603,000] <inf> main: Address of sample 0x42000000

[00:00:00.603,000] <inf> main: Current running version: 1.2.2
[00:00:00.603,000] <inf> ota_mgmt: Successfully opened flash area for slot0:
[00:00:00.603,000] <inf> ota_mgmt:   Area ID: 2
[00:00:00.603,000] <inf> ota_mgmt:   Device: flash-controller@60002000
[00:00:00.603,000] <inf> ota_mgmt:   Offset: 0x20000
[00:00:00.603,000] <inf> ota_mgmt:   Size: 0x150000
[00:00:00.603,000] <inf> main: Current running version: 32.0.0
[00:00:00.603,000] <inf> main: Main loop started. System is running.
[00:00:02.603,000] <inf> wifi_mgmt: Attempting WiFi connection to:  :)
[00:00:03.202,000] <inf> wifi_mgmt: *** WiFi connected successfully ***
[00:00:03.202,000] <inf> wifi_mgmt: === Setting up network interface ===
[00:00:03.202,000] <inf> wifi_mgmt: Interface up before: YES
[00:00:03.202,000] <inf> wifi_mgmt: Interface admin up: YES
[00:00:03.202,000] <inf> wifi_mgmt: Starting DHCP client
[00:00:03.202,000] <inf> wifi_mgmt: Interface up after: YES
[00:00:05.603,000] <inf> wifi_mgmt: === IP Address Debug ===
[00:00:05.603,000] <inf> wifi_mgmt: Interface up: YES
[00:00:05.603,000] <wrn> wifi_mgmt: No IP addresses found
[00:00:05.603,000] <inf> main: No IP
[00:00:10.603,000] <inf> wifi_mgmt: === IP Address Debug ===
[00:00:10.603,000] <inf> wifi_mgmt: Interface up: YES
[00:00:10.603,000] <inf> wifi_mgmt: Found IP (state 1): 192.168.2.140
[00:00:10.604,000] <inf> main: IP: 192.168.2.140
[00:00:13.202,000] <inf> wifi_mgmt: WiFi connected - checking for IP address...
[00:00:13.202,000] <inf> wifi_mgmt: === IP Address Debug ===
[00:00:13.202,000] <inf> wifi_mgmt: Interface up: YES
[00:00:13.202,000] <inf> wifi_mgmt: Found IP (state 1): 192.168.2.140
[00:00:13.202,000] <inf> wifi_mgmt: *** IP address obtained: 192.168.2.140 ***
[00:00:13.202,000] <inf> wifi_mgmt: Testing connectivity to 8.8.8.8:53...
[00:00:13.224,000] <inf> wifi_mgmt: ✓ Internet connectivity confirmed!
[00:00:13.224,000] <inf> wifi_mgmt: *** Full network connectivity confirmed! ***
[00:00:30.603,000] <inf> main: OTA: Checking for updates...
[00:00:31.125,000] <inf> ota_mgmt: Connected to 192.168.2.86:8080
[00:00:31.125,000] <inf> ota_mgmt: Checking for updates at http://192.168.2.86:8080/api/version
[00:00:31.133,000] <inf> ota_mgmt: HTTP headers complete, content length: 36
[00:00:31.133,000] <inf> ota_mgmt: Server version: 1.2.3
[00:00:31.133,000] <inf> ota_mgmt: New version available: 1.2.3 (current: 1.2.2)
[00:00:31.133,000] <inf> main: OTA: Update available, preparing download
[00:00:36.133,000] <inf> ota_mgmt: Tryning to download firmware
[00:00:36.133,000] <inf> blinky: Changing blink interval from 1000 ms to 2000 ms
[00:00:36.133,000] <inf> main: OTA: Downloading firmware update
[00:00:36.133,000] <inf> ota_mgmt: area ID of slot1: 3
[00:00:36.133,000] <inf> ota_mgmt: Flash image using area ID: 3
[00:00:36.146,000] <inf> ota_mgmt: Connected to 192.168.2.86:8080
[00:00:36.146,000] <inf> ota_mgmt: Downloading firmware from http://192.168.2.86:8080/api/firmware
[00:00:36.153,000] <inf> ota_mgmt: HTTP headers complete, content length: 583978
[00:00:36.153,000] <inf> ota_mgmt: Flash initialized for firmware download
uart:~$ I (5439) boot: Image index: 0, Swap type: none
[00:00:41.130,000] <inf> ota_mgmt: Downloaded: 583978 bytes (*float*%)
[00:00:41.130,000] <inf> ota_mgmt: Firmware download complete: 583978 bytes
[00:00:41.130,000] <inf> ota_mgmt: Firmware download successful.
[00:00:41.130,000] <inf> blinky: Changing blink interval from 2000 ms to 1000 ms
[00:00:41.130,000] <inf> main: OTA: Download complete, preparing to apply
[00:00:41.230,000] <inf> blinky: Changing blink interval from 1000 ms to 100 ms
[00:00:41.230,000] <inf> main: OTA: Applying update, device will reboot
[00:00:41.230,000] <inf> ota_mgmt: === Boot Status Debug ===
[00:00:41.230,000] <inf> ota_mgmt: Current swap type: 1
[00:00:41.230,000] <inf> ota_mgmt: Slot1 - First 4 bytes (magic): 0x96f3b83d
[00:00:41.230,000] <inf> ota_mgmt: === Image Header Debug ===
[00:00:41.230,000] <inf> ota_mgmt: Slot0 - Image size: 583796
[00:00:41.230,000] <inf> ota_mgmt: Slot0 - Version: 1.2.2+4
[00:00:41.230,000] <inf> ota_mgmt: ✓ Slot1 - Valid image found!
[00:00:41.230,000] <inf> ota_mgmt: Slot1 - Image size: 534580
[00:00:41.230,000] <inf> ota_mgmt: Slot1 - Version: 1.0.2+0
[00:00:41.230,000] <inf> ota_mgmt: Update ready - rebooting in 3 seconds
uart:~$ x▒▒ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0xc (RTC_SW_CPU_RST),boot:0x8 (SPI_FAST_FLASH_BOOT)
Saved PC:0x40375dd4
SPIWP:0xee
mode:DIO, clock div:1
load:0x3fcb5400,len:0x20b0
load:0x403ba400,len:0x9610
load:0x403c6400,len:0x49c
entry 0x403be38c
I (42) soc_init: MCUboot 2nd stage bootloader
I (42) soc_init: compile time Jul 25 2025 14:53:15
W (42) soc_init: Unicore bootloader
I (43) soc_init: chip revision: v0.2
I (46) qio_mode: Enabling default flash chip QIO
I (50) flash_init: Boot SPI Speed : 80MHz
I (54) flash_init: SPI Mode       : QIO
I (58) flash_init: SPI Flash Size : 8MB
I (95) boot: Image index: 0, Swap type: test
I (16434) boot: Loading image 0 - slot 0 from flash, area id: 2
I (16434) boot: Application start=4037cee0h
I (16435) boot: DRAM segment: paddr=0002dca0h, vaddr=3fc91c30h, size=03each ( 16044) load
I (16441) boot: IRAM segment: paddr=00020080h, vaddr=40374000h, size=0dc20h ( 56352) load
I (16460) boot: IROM segment: paddr=00040000h, vaddr=42000000h, size=5E1D6h (385494) map
I (16460) boot: DROM segment: paddr=000a0000h, vaddr=3c060000h, size=0E8A0h ( 59552) map
I (16475) boot: libc heap size 207 kB.
I (16476) spi_flash: detected chip: generic
I (16476) spi_flash: flash io: qio
W (16476) spi_flash: Detected size(16384k) larger than the size in the binary image header(8192k). Using the size in the binary image header.

[00:00:16.523,000] <inf> wifi_init: rx ba win: 6
*** Booting Zephyr OS build v4.2.0-rc3-44-g366d45fd34fe ***
[00:00:16.524,000] <inf> blinky: Blinky subsystem initialized - blinking every 1000 ms
[00:00:16.524,000] <inf> wifi_mgmt: WiFi management subsystem initialized
[00:00:16.524,000] <inf> ota_mgmt: Scheduling initial OTA check in 30 seconds.
[00:00:16.524,000] <inf> ota_mgmt: OTA management subsystem initialized
[00:00:16.524,000] <inf> main: ====================================
[00:00:16.524,000] <inf> main:    ESP32 OTA Application Booting
[00:00:16.524,000] <inf> main: ====================================
[00:00:16.524,000] <inf> main: Running a confirmed image.
[00:00:16.524,000] <inf> main: Address of sample 0x42000000

[00:00:16.524,000] <inf> main: Current running version: 1.2.2
[00:00:16.524,000] <inf> ota_mgmt: Successfully opened flash area for slot0:
[00:00:16.524,000] <inf> ota_mgmt:   Area ID: 2
[00:00:16.524,000] <inf> ota_mgmt:   Device: flash-controller@60002000
[00:00:16.524,000] <inf> ota_mgmt:   Offset: 0x20000
[00:00:16.524,000] <inf> ota_mgmt:   Size: 0x150000
[00:00:16.524,000] <inf> main: Current running version: 32.0.0
[00:00:16.524,000] <inf> main: Main loop started. System is running.
[00:00:18.524,000] <inf> wifi_mgmt: Attempting WiFi connection to:  :)
[00:00:19.139,000] <inf> wifi_mgmt: *** WiFi connected successfully ***
[00:00:19.139,000] <inf> wifi_mgmt: === Setting up network interface ===
[00:00:19.139,000] <inf> wifi_mgmt: Interface up before: YES
[00:00:19.139,000] <inf> wifi_mgmt: Interface admin up: YES
[00:00:19.139,000] <inf> wifi_mgmt: Starting DHCP client
[00:00:19.139,000] <inf> wifi_mgmt: Interface up after: YES
[00:00:21.524,000] <inf> wifi_mgmt: === IP Address Debug ===
[00:00:21.524,000] <inf> wifi_mgmt: Interface up: YES
[00:00:21.524,000] <wrn> wifi_mgmt: No IP addresses found
[00:00:21.524,000] <inf> main: No IP
[00:00:26.524,000] <inf> wifi_mgmt: === IP Address Debug ===
[00:00:26.524,000] <inf> wifi_mgmt: Interface up: YES
[00:00:26.524,000] <inf> wifi_mgmt: Found IP (state 1): 192.168.2.140
[00:00:26.524,000] <inf> main: IP: 192.168.2.140
[00:00:29.139,000] <inf> wifi_mgmt: WiFi connected - checking for IP address...
[00:00:29.139,000] <inf> wifi_mgmt: === IP Address Debug ===
[00:00:29.139,000] <inf> wifi_mgmt: Interface up: YES
[00:00:29.139,000] <inf> wifi_mgmt: Found IP (state 1): 192.168.2.140
[00:00:29.139,000] <inf> wifi_mgmt: *** IP address obtained: 192.168.2.140 ***
[00:00:29.140,000] <inf> wifi_mgmt: Testing connectivity to 8.8.8.8:53...
[00:00:29.163,000] <inf> wifi_mgmt: ✓ Internet connectivity confirmed!
[00:00:29.163,000] <inf> wifi_mgmt: *** Full network connectivity confirmed! ***
[00:00:46.524,000] <inf> main: OTA: Checking for updates...
[00:00:46.929,000] <inf> ota_mgmt: Connected to 192.168.2.86:8080
[00:00:46.929,000] <inf> ota_mgmt: Checking for updates at http://192.168.2.86:8080/api/version
[00:00:46.936,000] <inf> ota_mgmt: HTTP headers complete, content length: 36
[00:00:46.936,000] <inf> ota_mgmt: Server version: 1.2.3
[00:00:46.936,000] <inf> ota_mgmt: New version available: 1.2.3 (current: 1.2.2)
[00:00:46.936,000] <inf> main: OTA: Update available, preparing download
[00:00:51.936,000] <inf> ota_mgmt: Tryning to download firmware
[00:00:51.936,000] <inf> blinky: Changing blink interval from 1000 ms to 2000 ms
[00:00:51.936,000] <inf> main: OTA: Downloading firmware update
[00:00:51.937,000] <inf> ota_mgmt: area ID of slot1: 3
[00:00:51.937,000] <inf> ota_mgmt: Flash image using area ID: 3
[00:00:51.941,000] <inf> ota_mgmt: Connected to 192.168.2.86:8080
[00:00:51.941,000] <inf> ota_mgmt: Downloading firmware from http://192.168.2.86:8080/api/firmware
[00:00:51.948,000] <inf> ota_mgmt: HTTP headers complete, content length: 583978
[00:00:51.948,000] <inf> ota_mgmt: Flash initialized for firmware download
uart:~$ I (4238) boot: Image index: 0, Swap type: none
[00:00:57.825,000] <inf> ota_mgmt: Downloaded: 583978 bytes (*float*%)
[00:00:57.825,000] <inf> ota_mgmt: Firmware download complete: 583978 bytes
[00:00:57.825,000] <inf> ota_mgmt: Firmware download successful.
[00:00:57.825,000] <inf> blinky: Changing blink interval from 2000 ms to 1000 ms
[00:00:57.825,000] <inf> main: OTA: Download complete, preparing to apply
[00:00:57.925,000] <inf> blinky: Changing blink interval from 1000 ms to 100 ms
[00:00:57.925,000] <inf> main: OTA: Applying update, device will reboot
[00:00:57.925,000] <inf> ota_mgmt: === Boot Status Debug ===
[00:00:57.926,000] <inf> ota_mgmt: Current swap type: 1
[00:00:57.926,000] <inf> ota_mgmt: Slot1 - First 4 bytes (magic): 0x96f3b83d
[00:00:57.926,000] <inf> ota_mgmt: === Image Header Debug ===
[00:00:57.926,000] <inf> ota_mgmt: Slot0 - Image size: 583796
[00:00:57.926,000] <inf> ota_mgmt: Slot0 - Version: 1.2.2+4
[00:00:57.926,000] <inf> ota_mgmt: ✓ Slot1 - Valid image found!
[00:00:57.926,000] <inf> ota_mgmt: Slot1 - Image size: 583796
[00:00:57.926,000] <inf> ota_mgmt: Slot1 - Version: 1.2.3+4
[00:00:57.926,000] <inf> ota_mgmt: Update ready - rebooting in 3 seconds
uart:~$ ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0xc (RTC_SW_CPU_RST),boot:0x8 (SPI_FAST_FLASH_BOOT)
Saved PC:0x40375dd4
SPIWP:0xee
mode:DIO, clock div:1
load:0x3fcb5400,len:0x20b0
load:0x403ba400,len:0x9610
load:0x403c6400,len:0x49c
entry 0x403be38c
I (42) soc_init: MCUboot 2nd stage bootloader
I (42) soc_init: compile time Jul 25 2025 14:53:15
W (42) soc_init: Unicore bootloader
I (43) soc_init: chip revision: v0.2
I (46) qio_mode: Enabling default flash chip QIO
I (50) flash_init: Boot SPI Speed : 80MHz
I (54) flash_init: SPI Mode       : QIO
I (58) flash_init: SPI Flash Size : 8MB
I (95) boot: Image index: 0, Swap type: test
I (15164) boot: Loading image 0 - slot 0 from flash, area id: 2
I (15164) boot: Application start=4037cee0h
I (15164) boot: DRAM segment: paddr=0002dca0h, vaddr=3fc91c30h, size=03each ( 16044) load
I (15171) boot: IRAM segment: paddr=00020080h, vaddr=40374000h, size=0dc20h ( 56352) load
I (15190) boot: IROM segment: paddr=00040000h, vaddr=42000000h, size=5E1D2h (385490) map
I (15190) boot: DROM segment: paddr=000a0000h, vaddr=3c060000h, size=0E8A0h ( 59552) map
I (15205) boot: libc heap size 207 kB.
I (15206) spi_flash: detected chip: generic
I (15206) spi_flash: flash io: qio
W (15206) spi_flash: Detected size(16384k) larger than the size in the binary image header(8192k). Using the size in the binary image header.

[00:00:15.253,000] <inf> wifi_init: rx ba win: 6
*** Booting Zephyr OS build v4.2.0-rc3-44-g366d45fd34fe ***
[00:00:15.254,000] <inf> blinky: Blinky subsystem initialized - blinking every 1000 ms
[00:00:15.254,000] <inf> wifi_mgmt: WiFi management subsystem initialized
[00:00:15.254,000] <inf> ota_mgmt: OTA management subsystem initialized
[00:00:15.254,000] <inf> main: ====================================
[00:00:15.254,000] <inf> main:    ESP32 OTA Application Booting
[00:00:15.254,000] <inf> main: ====================================
[00:00:15.254,000] <wrn> main: Running new firmware in TEST mode.
[00:00:15.254,000] <wrn> main: Scheduling confirmation in 30 seconds.
[00:00:15.254,000] <inf> main: Address of sample 0x42000000

[00:00:15.254,000] <inf> main: Current running version: 1.2.3
[00:00:15.254,000] <inf> ota_mgmt: Successfully opened flash area for slot0:
[00:00:15.254,000] <inf> ota_mgmt:   Area ID: 2
[00:00:15.254,000] <inf> ota_mgmt:   Device: flash-controller@60002000
[00:00:15.254,000] <inf> ota_mgmt:   Offset: 0x20000
[00:00:15.254,000] <inf> ota_mgmt:   Size: 0x150000
[00:00:15.254,000] <inf> main: Current running version: 32.0.0
[00:00:15.254,000] <inf> main: Main loop started. System is running.
[00:00:17.254,000] <inf> wifi_mgmt: Attempting WiFi connection to:  :)
[00:00:17.848,000] <inf> wifi_mgmt: *** WiFi connected successfully ***
[00:00:17.848,000] <inf> wifi_mgmt: === Setting up network interface ===
[00:00:17.848,000] <inf> wifi_mgmt: Interface up before: YES
[00:00:17.848,000] <inf> wifi_mgmt: Interface admin up: YES
[00:00:17.848,000] <inf> wifi_mgmt: Starting DHCP client
[00:00:17.848,000] <inf> wifi_mgmt: Interface up after: YES
[00:00:20.254,000] <inf> wifi_mgmt: === IP Address Debug ===
[00:00:20.254,000] <inf> wifi_mgmt: Interface up: YES
[00:00:20.254,000] <wrn> wifi_mgmt: No IP addresses found
[00:00:20.254,000] <inf> main: No IP
[00:00:25.254,000] <inf> wifi_mgmt: === IP Address Debug ===
[00:00:25.254,000] <inf> wifi_mgmt: Interface up: YES
[00:00:25.254,000] <inf> wifi_mgmt: Found IP (state 1): 192.168.2.140
[00:00:25.254,000] <inf> main: IP: 192.168.2.140
[00:00:27.848,000] <inf> wifi_mgmt: WiFi connected - checking for IP address...
[00:00:27.848,000] <inf> wifi_mgmt: === IP Address Debug ===
[00:00:27.848,000] <inf> wifi_mgmt: Interface up: YES
[00:00:27.848,000] <inf> wifi_mgmt: Found IP (state 1): 192.168.2.140
[00:00:27.848,000] <inf> wifi_mgmt: *** IP address obtained: 192.168.2.140 ***
[00:00:27.848,000] <inf> wifi_mgmt: Testing connectivity to 8.8.8.8:53...
[00:00:27.867,000] <inf> wifi_mgmt: ✓ Internet connectivity confirmed!
[00:00:27.867,000] <inf> wifi_mgmt: *** Full network connectivity confirmed! ***
[00:00:45.254,000] <inf> main: Confirming running firmware as valid.
[00:00:45.254,000] <inf> main: Image confirmed successfully. The update is now permanent.
[00:00:45.254,000] <inf> main: OTA: Checking for updates...
[00:00:48.255,000] <err> ota_mgmt: Failed to connect to 192.168.2.86:8080: 116
[00:00:48.255,000] <err> ota_mgmt: Server connection failed. Entering 1-hour backoff.
[00:00:48.255,000] <inf> blinky: Changing blink interval from 1000 ms to 250 ms
[00:00:48.255,000] <err> main: OTA: Error occurred, code: 1
[00:00:48.255,000] <inf> blinky: Changing blink interval from 250 ms to 1000 ms
uart:~$
