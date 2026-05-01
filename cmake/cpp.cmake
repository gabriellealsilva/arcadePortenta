# global cpp options, definitions, warnings, ...

# Compiler warnings
add_compile_options(
    -Wall
    -Wextra
#    -Wpedantic
# only for C++: -Wuseless-cast 
    -Werror=all
    -Werror=extra
    -Werror=double-promotion
    -Werror=unused
    -Werror=undef
    -Werror=uninitialized
    -Werror=pointer-arith
    -Werror=missing-declarations
    -Werror=vla
    -Werror=overlength-strings
    -Werror=unused-local-typedefs
    -Werror=unused-result
    -Werror=varargs
    -Werror=write-strings
    -Werror=overflow


#    -Wold-style-cast
#    -Wcast-qual
#    -Wconversion-null
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU") # Clang don't support -Wlogical-op
    add_compile_options(
        -Werror=logical-op
    )
endif()
    
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang") # Warnings supproted only by Clang
    add_compile_options(
        -Werror=constant-conversion
        -Werror=absolute-value
        -Werror=return-stack-address
    )
endif()

if (${COPY_PROTECTION_DISABLED})
    # Disable of copy protection only for release firmware and bootloader
    if (${CMAKE_BUILD_TYPE} STREQUAL "Release" AND ${CMAKE_CROSSCOMPILING})
        message("Build Bootloader and Firmware with copy protection. Only for internal use!")
        add_compile_definitions(COPY_PROTECTION_DISABLED)
    else ()
        error("COPY_PROTECTION_DISABLED only for release builds / not for emulation")
    endif ()
endif ()

if (NOT CMAKE_CROSSCOMPILING AND ASAN)
        message("Address Sanitizer enabled")

        add_compile_options(-fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined)
        add_link_options(-fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined)

endif ()

# C++ only options
add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,GNU>:-fno-rtti>)
add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,GNU>:-fno-threadsafe-statics>)
add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,GNU>:-fno-use-cxa-atexit>)
add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,GNU>:-fno-exceptions>)

# C++ only options warnings
add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wctor-dtor-privacy>)
add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wno-non-virtual-dtor>)
add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wuseless-cast>)
add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wzero-as-null-pointer-constant>)
add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Werror=suggest-override>)
add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wextra-semi>)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU") # Clang don't support all GCC warnings
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 14.1)
        add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Werror=template-id-cdtor>)        
    endif()
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 15.1)
        add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Werror=deprecated-literal-operator>)
    endif()
endif()

if (${WIN32})
    add_compile_options(
            -mno-ms-bitfields
    )
endif ()



#add_compile_options(-fno-inline-functions -fno-move-loop-invariants)

find_package(Git REQUIRED)

if (BUILD_NUMBER)
    add_compile_definitions(BUILD_NUMBER=${BUILD_NUMBER})
    message("build nummer = ${BUILD_NUMBER}")
endif ()

if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_definitions(DEBUG)
    if (CMAKE_CROSSCOMPILING)
        add_compile_options(-Og -g3 -ggdb)    # -g is default for Debug
    else()    
        add_compile_options(-O0 -ggdb)    # -g is default for Debug
    endif()
    add_compile_options(-fno-omit-frame-pointer)

else ()
    add_compile_options(-O2)    # -g only increase .elf file, not the .hex / memory usage in the flash
    add_compile_options(-g)    # -g only increase .elf file, not the .hex / memory usage in the flash
    #set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    # NDEBUG and -O3 is default for Release
    # Firmware need to fit together with 
    add_compile_definitions(USE_BOOTLOADER)
endif ()
