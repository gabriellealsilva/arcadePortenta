
set (STARTUP_STM32H747XX_CM4  ${CMAKE_SOURCE_DIR}/Hardware/Portenta/CPS1/CM4/startup_stm32h747xx_CM4.s)
set (STARTUP_STM32H747XX_CM7  ${CMAKE_SOURCE_DIR}/Hardware/Portenta/CPS1/CM7/startup_stm32h747xx_CM7.s)

set(HAL_DRIVERS_COMMON_SRC
    ${CMAKE_SOURCE_DIR}/HAL/ST/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c 
    ${CMAKE_SOURCE_DIR}/HAL/ST/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim.c 
    ${CMAKE_SOURCE_DIR}/HAL/ST/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim_ex.c 
    ${CMAKE_SOURCE_DIR}/HAL/ST/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c 
    ${CMAKE_SOURCE_DIR}/HAL/ST/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.c 
    ${CMAKE_SOURCE_DIR}/HAL/ST/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c 
    ${CMAKE_SOURCE_DIR}/HAL/ST/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc_ex.c 
    ${CMAKE_SOURCE_DIR}/HAL/ST/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.c 
    ${CMAKE_SOURCE_DIR}/HAL/ST/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash_ex.c 
    ${CMAKE_SOURCE_DIR}/HAL/ST/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c 
    ${CMAKE_SOURCE_DIR}/HAL/ST/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_hsem.c 
    ${CMAKE_SOURCE_DIR}/HAL/ST/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma.c 
    ${CMAKE_SOURCE_DIR}/HAL/ST/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma_ex.c 
    ${CMAKE_SOURCE_DIR}/HAL/ST/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mdma.c 
    ${CMAKE_SOURCE_DIR}/HAL/ST/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c 
    ${CMAKE_SOURCE_DIR}/HAL/ST/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c.c 
    ${CMAKE_SOURCE_DIR}/HAL/ST/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c_ex.c 
    ${CMAKE_SOURCE_DIR}/HAL/ST/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_exti.c 

)


set(HAL_DRIVERS_INCLUDES
    ${CMAKE_SOURCE_DIR}/HAL/ST/Drivers/STM32H7xx_HAL_Driver/Inc
    ${CMAKE_SOURCE_DIR}/HAL/ST/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy
    ${CMAKE_SOURCE_DIR}/HAL/ST/Drivers/CMSIS/Device/ST/STM32H7xx/Include
    ${CMAKE_SOURCE_DIR}/HAL/ST/Drivers/CMSIS/Include
)

set(HAL_CM7_SRC
${HAL_DRIVERS_COMMON_SRC}
${CMAKE_SOURCE_DIR}/HAL/ST/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2s.c
${STARTUP_STM32H747XX_CM7}
)

set(HAL_CM4_SRC
${HAL_DRIVERS_SRC}
${STARTUP_STM32H747XX_CM4}
)
