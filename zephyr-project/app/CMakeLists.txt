# app/CMakeLists.txt (Korrigierte und vollständige Version)

# Definiert die minimale CMake-Version
cmake_minimum_required(VERSION 3.20.0)

# Findet das Zephyr-Paket und lädt dessen CMake-Funktionen.
# Dies ist auch innerhalb von Sysbuild für jede Anwendung notwendig.
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# Definiert den Namen dieses spezifischen Anwendungs-Projekts.
# Der Name MUSS mit dem Ordnernamen übereinstimmen.
project(app)

# Fügt Ihre C-Quelldateien zum "app"-Ziel hinzu.
target_sources(app PRIVATE
    src/main.c
    src/blinky.c
    src/wifi_mgmt.c
    src/ota_mgmt.c
    src/watchdog_mgmt.c
)

#set(DTC_OVERLAY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/boards/${BOARD}.overlay)

target_include_directories(app PRIVATE 
    include
)
