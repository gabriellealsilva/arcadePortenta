set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_ARM_COMPILATION 1)

# Toolchain path - adjust if needed
set(TOOLCHAIN_PATH "C:/arm-none-eabi/14.3_rel1")

set(TOOLCHAIN_PREFIX "arm-none-eabi-")

if (${CMAKE_HOST_WIN32})
    set(SUFFIX ".exe")
    set(SIMPLE_GREP "findstr")
else ()
    set(SUFFIX "")
    set(SIMPLE_GREP "grep")
endif ()


# Set the toolchain tools
set(CMAKE_C_COMPILER   "${TOOLCHAIN}/bin/arm-none-eabi-gcc${SUFFIX}")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN}/bin/arm-none-eabi-g++${SUFFIX}")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN}/bin/arm-none-eabi-gcc${SUFFIX}")

set(CMAKE_OBJCOPY      "${TOOLCHAIN}/bin/arm-none-eabi-objcopy${SUFFIX}")
set(CMAKE_SIZE         "${TOOLCHAIN}/bin/arm-none-eabi-size${SUFFIX}")
set(CMAKE_OBJDUMP      "${TOOLCHAIN}/bin/arm-none-eabi-objdump${SUFFIX}")
set(CMAKE_NM           "${TOOLCHAIN}/bin/arm-none-eabi-nm${SUFFIX}")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# General c
add_compile_options(-Wno-psabi) # fix note: parameter passing for argument of type 'std::pair<float, float>' when C++17 is enabled changed to match C++14 in GCC 10.1
add_compile_options(-fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common)
add_compile_options(-fabi-version=0)

# Linker options
add_link_options(-Xlinker --gc-sections --specs=nano.specs --specs=nosys.specs)
add_link_options(-Wl,--print-memory-usage)