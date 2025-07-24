You are absolutely right to challenge that. My apologies. I gave you the command-line method, but you are looking for a permanent, file-based solution that you can check into your version control system.

You absolutely **CAN** have different signing types (and other sysbuild settings) for different boards, and you can define this entirely in files. You do this using the `Kconfig.sysbuild` file.

This is the advanced solution you're looking for.

### The Solution: Using `Kconfig.sysbuild` for Logic

While `sysbuild.conf` is a simple list of `key=value` pairs, `Kconfig.sysbuild` is a file where you can write Kconfig logic (`if`, `depends on`, `default`) that is processed at the **sysbuild level**. It allows you to set `SB_CONFIG_` values conditionally based on the selected board.

Here is how to solve your problem step-by-step:

**Goal:**
*   **Board A (`nrf52840dk_nrf52840`):** Use `SB_CONFIG_BOOT_SIGNATURE_TYPE_RSA`
*   **Board B (`nrf5340dk_nrf5340_cpuapp`):** Use `SB_CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256`

---

#### Step 1: Create a `Kconfig.sysbuild` File

In the root of your application directory (the same place as `prj.conf` and `sysbuild.conf`), create a new file named `Kconfig.sysbuild`.

```
my_app/
├── prj.conf
├── sysbuild.conf
├── Kconfig.sysbuild   <-- NEW FILE
└── src/
    └── main.c
```

#### Step 2: Add Board-Specific Logic to `Kconfig.sysbuild`

Open `Kconfig.sysbuild` and add the following logic. This code sets the *default* value for the sysbuild signature choice based on the board name.

```kconfig
# Kconfig.sysbuild

# This file adds configuration logic to the System Build (sysbuild) level.

if BOARD = "nrf52840dk_nrf52840"
    # For this board, default the sysbuild signing choice to RSA
    config SB_CONFIG_BOOT_SIGNATURE_TYPE_RSA
        bool
        default y
endif

if BOARD = "nrf5340dk_nrf5340_cpuapp"
    # For this board, default the sysbuild signing choice to ECDSA
    config SB_CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256
        bool
        default y
endif
```

#### Step 3: Clean up `sysbuild.conf`

Now, you must **remove** the signature type setting from your `sysbuild.conf` file. If you leave it in, it will create a conflict or override the logic you just wrote.

Your `sysbuild.conf` should now only contain settings that are truly common to all boards.

```conf
# sysbuild.conf

# Enable MCUboot for all builds
SB_CONFIG_BOOTLOADER_MCUBOOT=y

# DO NOT set the signature type here anymore.
# SB_CONFIG_BOOT_SIGNATURE_TYPE_RSA=y <-- DELETE THIS LINE
```

#### Step 4: Build Without Any Extra Command-Line Flags

Now your build commands become simple and clean. The build system will automatically apply the correct logic from `Kconfig.sysbuild`.

*   **Build for Board A:**
    ```bash
    west build -b nrf52840dk_nrf52840 --sysbuild
    ```
    *(The build system sees the board is `nrf52840dk_nrf52840`, reads `Kconfig.sysbuild`, and sets `SB_CONFIG_BOOT_SIGNATURE_TYPE_RSA=y` for this build.)*

*   **Build for Board B:**
    ```bash
    west build -b nrf5340dk_nrf5340_cpuapp --sysbuild
    ```
    *(The build system sees the board is `nrf5340dk_nrf5340_cpuapp`, reads `Kconfig.sysbuild`, and sets `SB_CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256=y` for this build.)*

### Why This Works

The Zephyr build system processes configuration in a specific order. When you run `west build --sysbuild`:
1.  It starts the **Sysbuild configuration phase**.
2.  It finds your `Kconfig.sysbuild` file and merges its logic into the standard sysbuild Kconfig tree.
3.  It evaluates the Kconfig logic. The `if BOARD = "..."` statements check against the board you passed with the `-b` flag.
4.  The `default y` for the matching board becomes active, setting the correct `SB_CONFIG_` value.
5.  With the sysbuild level now fully configured, it proceeds to configure the child images (`mcuboot`, `app`), forcefully applying the correct `CONFIG_` values based on the `SB_CONFIG_` you just set conditionally.

This `Kconfig.sysbuild` method is the correct, intended way to handle complex, multi-board configurations that need to control system-level features.