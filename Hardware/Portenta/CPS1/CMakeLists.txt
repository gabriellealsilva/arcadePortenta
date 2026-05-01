add_custom_target(CPS_1_Portenta)

add_custom_command(
  TARGET CPS_1_Portenta POST_BUILD
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND ${CubeMXCommand} -jar ${CubeMXParam} -q CPS_1.cubemx_script
)

if (CMAKE_CROSSCOMPILING)

set(COMMON_SRC
    ${CMAKE_CURRENT_LIST_DIR}/Common/Src/syscalls.c
#    ${CMAKE_CURRENT_LIST_DIR}/Common/Src/sysmem.c
    ${CMAKE_CURRENT_LIST_DIR}/Common/Src/system_stm32h7xx_dualcore_boot_cm4_cm7.c
)

add_library(libPortentaCPS1_CM7 INTERFACE)
target_sources(libPortentaCPS1_CM7 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/CM7/Src/main.c
    ${CMAKE_CURRENT_LIST_DIR}/CM7/Src/stm32h7xx_hal_msp.c
    ${CMAKE_CURRENT_LIST_DIR}/CM7/Src/stm32h7xx_it.c
    ${CMAKE_CURRENT_LIST_DIR}/Common/Src/syscalls.c
    ${COMMON_SRC}
)
target_include_directories(libPortentaCPS1_CM7 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/CM7/Inc
)
target_compile_definitions(libPortentaCPS1_CM7 INTERFACE
    USE_FLASH_ECC=0
    USE_MULTI_CORE_SHARED_CODE=1
)

add_library(libPortentaCPS1_CM4 INTERFACE)
target_sources(libPortentaCPS1_CM4 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/CM4/Src/main.c
    ${CMAKE_CURRENT_LIST_DIR}/CM4/Src/stm32h7xx_hal_msp.c
    ${CMAKE_CURRENT_LIST_DIR}/CM4/Src/stm32h7xx_it.c
    ${COMMON_SRC}
)
target_include_directories(libPortentaCPS1_CM4 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/CM4/Inc
)
target_compile_definitions(libPortentaCPS1_CM4 INTERFACE
    USE_FLASH_ECC=0
    USE_MULTI_CORE_SHARED_CODE=1
)

add_library(lib::portenta::cps1::cm7 ALIAS libPortentaCPS1_CM7)
add_library(lib::portenta::cps1::cm4 ALIAS libPortentaCPS1_CM4)

endif ()
