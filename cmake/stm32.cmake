# ---------------------------------------------------------------------------
# STM32H747 Cortex-M7 core (480 MHz, FPv5-D16 double-precision FPU)
# ---------------------------------------------------------------------------
add_library(stm32h747_cm7 INTERFACE)

target_compile_options(stm32h747_cm7 INTERFACE
    $<$<COMPILE_LANGUAGE:C>:${C_DEFS} -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb>
    $<$<COMPILE_LANGUAGE:CXX>:${CXX_DEFS} -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb>
    $<$<COMPILE_LANGUAGE:ASM>:-x assembler-with-cpp -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb>
)

target_compile_definitions(stm32h747_cm7 INTERFACE
    STM32H747xx
    DATA_IN_D2_SRAM
)

target_link_options(stm32h747_cm7 INTERFACE
    -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb
)

add_library(hal::stm32h747::cm7 ALIAS stm32h747_cm7)

# ---------------------------------------------------------------------------
# STM32H747 Cortex-M4 core (240 MHz, FPv4-SP single-precision FPU)
# ---------------------------------------------------------------------------
add_library(stm32h747_cm4 INTERFACE)

target_compile_options(stm32h747_cm4 INTERFACE
    $<$<COMPILE_LANGUAGE:C>:${C_DEFS} -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb>
    $<$<COMPILE_LANGUAGE:CXX>:${CXX_DEFS} -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb>
    $<$<COMPILE_LANGUAGE:ASM>:-x assembler-with-cpp -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb>
)

target_compile_definitions(stm32h747_cm4 INTERFACE
    STM32H747xx
    DATA_IN_D2_SRAM
)

target_link_options(stm32h747_cm4 INTERFACE
    -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb
)

add_library(hal::stm32h747::cm4 ALIAS stm32h747_cm4)
