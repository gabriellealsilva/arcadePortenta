cmake_minimum_required(VERSION 3.22)

# Using Cross compiling toolchain:
# cd build
# Debug version:
# cmake .. -DTOOLCHAIN=/opt/toolchains/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi

#
# Release version:
# -DCMAKE_BUILD_TYPE=Release
# cmake .. -DCMAKE_BUILD_TYPE=Release
# cmake .. -DTOOLCHAIN=/opt/toolchains/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi -DCMAKE_BUILD_TYPE=Release
#
# Install into local bin folder
# -DCMAKE_INSTALL_PREFIX=.
#
# Disable copy protection
# Works only for release firmware and bootloader
# -DCOPY_PROTECTION_DISABLED=ON


# Define cross compile compiler (gcc), need to be defined before project
if (TOOLCHAIN)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/toolchain.cmake)
endif ()

# builds with toolchain (CMAKE_CROSSCOMPILING), don´t set CMAKE_BUILD_TYPE
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif ()

project(Arcade-Portenta LANGUAGES C CXX ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ASAN "compile and link with address sanitizer" OFF )
option(COPY_PROTECTION_DISABLED "Disable Copy Protection checks (only Release and Cross Compile)" OFF)
option(REPORT_GCC_INCLUDES "Enable GCC header include tracing (-H) for C/C++ compilation." OFF)

# Language standards: C++23, GNU/C 23
set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)

message("Cross Compiling is " ${CMAKE_CROSSCOMPILING})
message("CMAKE_BUILD_TYPE is " ${CMAKE_BUILD_TYPE})
message("COPY_PROTECTION_DISABLED is " ${COPY_PROTECTION_DISABLED})
message("ASAN is " ${ASAN})

if (NOT CMAKE_CROSSCOMPILING)
    find_package(SDL3 REQUIRED)
endif ()

#include(cmake/bootloader.cmake)
include(cmake/consoles.cmake)
include(cmake/cpp.cmake)
include(cmake/cubeMx.cmake)
include(cmake/firmware.cmake)
include(cmake/get_cpm.cmake)
include(cmake/HAL.cmake)
include(cmake/stm32.cmake)


CPMAddPackage(NAME m68000 GIT_REPOSITORY "https://github.com/kstenerud/Musashi.git" GIT_TAG "master") #repo has no tags

# IOC / hardware pin config (STM32CubeMX generated)
add_subdirectory(Hardware/Portenta/CPS1)
add_subdirectory(Videogame/CPS-1)

# Bootloader
#add_subdirectory(Bootloader)


if (NOT CMAKE_CROSSCOMPILING)
    SET(TARGET_PREFIX Emulator-)
endif ()

# Smoke test: verify core targets build cleanly
if (CMAKE_CROSSCOMPILING)
    add_custom_target(
        smoke_builds
        DEPENDS Portenta_CM7_CPS1 Portenta_CM4_CPS1
    )
else()
    add_custom_target(
        smoke_builds
        DEPENDS Emulator-CPS-1
    )
endif()
