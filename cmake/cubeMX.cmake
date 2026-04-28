if (${CI_CUBEMX})
    set(xvfbCommand xvfb-run --server-args=/"-screen 0 1280x800x24/")
    set(javaCommand /usr/local/STMicroelectronics/STM32Cube/STM32CubeMX/jre/bin/java)
    set(CubeMXCommand ${xvfbCommand} ${javaCommand})
    set (CubeMXParam "/usr/local/STMicroelectronics/STM32Cube/STM32CubeMX/STM32CubeMX")

else ()
    if (${CMAKE_HOST_WIN32})
        set (CubeMXCommand "C:/Users/gabri/AppData/Local/Programs/STM32CubeMX/jre/bin/java.exe")
        set (CubeMXParam "C:/Users/gabri/AppData/Local/Programs/STM32CubeMX/STM32CubeMX.exe")
    else ()
        set (CubeMXCommand "$ENV{HOME}/STM32CubeMX/jre/bin/java")
        set (CubeMXParam "$ENV{HOME}/STM32CubeMX/STM32CubeMX")
    endif ()

endif ()


