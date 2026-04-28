#[=================================================[
target_consume_interface_library()

copy INTERFACE target usage requirements into a concrete build target:
- INTERFACE include directories and compile definitions are re-exposed as PUBLIC
- INTERFACE sources are compiled into the concrete target as PRIVATE

limitation: support ony one (1) interface_library per call to keep code readable. 
            (list of interface_libraries are not supported at the moment!)
#]=================================================]
function(target_consume_interface_library target interface_library)
    if (NOT TARGET ${interface_library})
        message(FATAL_ERROR "target_consume_interface_library: target '${interface_library}' does not exist")
    endif ()

    get_target_property(interface_includes ${interface_library} INTERFACE_INCLUDE_DIRECTORIES)
    if (interface_includes)
        target_include_directories(${target} PUBLIC ${interface_includes})
    endif ()

    get_target_property(interface_system_includes ${interface_library} INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)
    if (interface_system_includes)
        target_include_directories(${target} SYSTEM PUBLIC ${interface_system_includes})
    endif ()

    get_target_property(interface_definitions ${interface_library} INTERFACE_COMPILE_DEFINITIONS)
    if (interface_definitions)
        target_compile_definitions(${target} PUBLIC ${interface_definitions})
    endif ()

    get_target_property(interface_sources ${interface_library} INTERFACE_SOURCES)
    if (interface_sources)
        target_sources(${target} PRIVATE ${interface_sources})
    endif ()
endfunction()


#[=================================================[
post build function

- create .hex from .elf, 
- create .list
- print size info
- create .address_info file
#]=================================================]
function(target_hexfile_post_process target dest_folder)

    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} ARGS -O ihex $<TARGET_FILE_NAME:${target}> $<TARGET_FILE_BASE_NAME:${target}>.hex
        COMMAND ${CMAKE_OBJDUMP} ARGS -h -l --inlines --dwarf=decodedline -S $<TARGET_FILE_NAME:${target}> > "$<TARGET_FILE_BASE_NAME:${target}>.list"        
        COMMAND ${CMAKE_SIZE} ARGS --format=berkeley $<TARGET_FILE_NAME:${target}>
        COMMAND ${CMAKE_NM} $<TARGET_FILE_NAME:${target}> > $<TARGET_FILE_BASE_NAME:${target}>.address_info
    )

    set_property(
        TARGET ${target}
        APPEND
        PROPERTY ADDITIONAL_CLEAN_FILES
        $<TARGET_FILE_BASE_NAME:${target}>.hex
        $<TARGET_FILE_BASE_NAME:${target}>.list
        $<TARGET_FILE_BASE_NAME:${target}>.map
    )

    install(
        FILES $<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_BASE_NAME:${target}>.hex
        # remove/replace this after getting proper formated info
        FILES $<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_BASE_NAME:${target}>.address_info

        DESTINATION bin/${dest_folder}
    )

endfunction()

#[=================================================[
tweak cross compile targets

- set linker options
- force .elf as extension
- rename target, if COPY_PROTECTION_DISABLED
#]=================================================]
function(target_mcu_options target linker_script)
    set(NAME_SUFFIX "_${CMAKE_BUILD_TYPE}")
    if (${CMAKE_BUILD_TYPE} STREQUAL "Release" AND ${COPY_PROTECTION_DISABLED})
        set(NAME_SUFFIX "-NoProtect${NAME_SUFFIX}")
    endif ()

    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME ${target}${NAME_SUFFIX}
        SUFFIX ".elf"
    )

    target_link_options(${target}
        PRIVATE -Wl,-Map,$<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_BASE_NAME:${target}>.map
        -T ${linker_script} -L${CMAKE_SOURCE_DIR}/ldscripts
        # since the STM32 MCU don't have a memory manager, this warning is not relevant.
        -Wl,--no-warn-rwx-segment
    )

endfunction()

#[=================================================[
add_console() meta function to define  

CONSOLE_SOURCES:                      source to build the console firmware
LINKER_SCRIPT:                        .ld file             
#]=================================================]
function(add_console target CONSOLE_SOURCES SYSTEM_SOURCES LINKER_SCRIPT dest_folder) #   CONSOLE_INCLUDE_DIRECTORIES CONSOLE_SYSTEM_INCLUDE_DIRECTORIES)

    add_executable(${target} ${CONSOLE_SOURCES} ${SYSTEM_SOURCES})

    # Disable specific warnings from external sources e.g. STM
    set_source_files_properties(${SYSTEM_SOURCES} PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS} -Wno-unused -Wno-unused-parameter -Wno-missing-declarations")

    target_mcu_options(${target} ${LINKER_SCRIPT})
    target_hexfile_post_process(${target} ${dest_folder})

endfunction()
