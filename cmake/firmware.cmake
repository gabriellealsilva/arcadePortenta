# ---------------------------------------------------------------------------
# CM7 firmware sources
# ---------------------------------------------------------------------------
set(FIRMWARE_SRC
    ${CMAKE_SOURCE_DIR}/Firmware/Loop/mainFirmware.cpp
)

set(FIRMWARE_INCLUDES
    ${CMAKE_SOURCE_DIR}/Firmware
)

set(FIRMWARE_DEFINES
    USE_HAL_DRIVER
)

set(FIRMWARE_LINK_LIBS_CM7
    hal::stm32h747::cm7
)

set(FIRMWARE_LINK_LIBS_CM4
    hal::stm32h747::cm4
)


# ---------------------------------------------------------------------------
# PC emulator (SDL3) sources
# ---------------------------------------------------------------------------
set(EMULATION_SRC
    ${CMAKE_SOURCE_DIR}/Firmware/Emulation/main.cpp
)

set(EMULATION_INCLUDES
    ${CMAKE_SOURCE_DIR}/Firmware/Emulation
    ${CMAKE_SOURCE_DIR}/Videogame/CPS1           
)

set(EMULATOR_DEFINES
    EMULATION
)

set(EMULATOR_MAIN_DEFINES
    ${EMULATOR_DEFINES}
    USE_SDL
)

# SDL3 target selection:
# The official SDL3 MinGW package ships SDL3::SDL3-shared (DLL) only.
# SDL3::SDL3 is the canonical alias (resolves to shared or static depending on
# what is installed).  SDL3main is not shipped with the MinGW DLL package.
set(EMULATOR_MAIN_LINK_LIBS
    SDL3::SDL3
)
