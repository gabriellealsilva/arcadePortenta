set(MUSASHI_SRC "${m68000_SOURCE_DIR}")
set(MUSASHI_GEN "${CMAKE_CURRENT_BINARY_DIR}/gen")
file(MAKE_DIRECTORY "${MUSASHI_GEN}")

if(CMAKE_CROSSCOMPILING)
    # m68kmake must run on the BUILD HOST — compile and run it at configure time
    # using the MSYS2 GCC, before the ARM build starts.
    find_program(HOST_C_COMPILER
        NAMES gcc.exe gcc cc
        PATHS "C:/msys64/ucrt64/bin" "C:/msys64/mingw64/bin"
        DOC "Host C compiler used to build the m68kmake code-generator tool"
        REQUIRED
    )

    if(CMAKE_HOST_WIN32)
        set(M68KMAKE_EXE "${MUSASHI_GEN}/m68kmake.exe")
    else()
        set(M68KMAKE_EXE "${MUSASHI_GEN}/m68kmake")
    endif()

    # Prepend the compiler's own bin dir so its runtime DLLs are found
    # even when the cross-compile preset doesn't add MSYS2 to PATH.
    get_filename_component(HOST_COMPILER_DIR "${HOST_C_COMPILER}" DIRECTORY)

    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env "PATH=${HOST_COMPILER_DIR};$ENV{PATH}"
            "${HOST_C_COMPILER}" -w "${MUSASHI_SRC}/m68kmake.c" -o "${M68KMAKE_EXE}"
        RESULT_VARIABLE compile_result
    )
    if(compile_result)
        message(FATAL_ERROR "Failed to compile m68kmake with ${HOST_C_COMPILER}")
    endif()

    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env "PATH=${HOST_COMPILER_DIR};$ENV{PATH}"
            "${M68KMAKE_EXE}" "${MUSASHI_GEN}" "${MUSASHI_SRC}/m68k_in.c"
        RESULT_VARIABLE gen_result
    )
    if(gen_result)
        message(FATAL_ERROR "m68kmake failed to generate opcode tables")
    endif()

    add_library(musashi STATIC
        "${MUSASHI_SRC}/m68kcpu.c"
        "${MUSASHI_SRC}/softfloat/softfloat.c"
        "${MUSASHI_GEN}/m68kops.c"
    )
else()
    # Host build: m68kmake can be compiled with the same toolchain as everything else.
    add_executable(m68kmake EXCLUDE_FROM_ALL "${MUSASHI_SRC}/m68kmake.c")
    target_compile_options(m68kmake PRIVATE -w)

    add_custom_command(
        OUTPUT  "${MUSASHI_GEN}/m68kops.h" "${MUSASHI_GEN}/m68kops.c"
        COMMAND m68kmake "${MUSASHI_GEN}" "${MUSASHI_SRC}/m68k_in.c"
        DEPENDS m68kmake "${MUSASHI_SRC}/m68k_in.c"
        COMMENT "Generating Musashi 68k opcode tables"
    )

    add_library(musashi STATIC
        "${MUSASHI_SRC}/m68kcpu.c"
        "${MUSASHI_SRC}/softfloat/softfloat.c"
        "${MUSASHI_GEN}/m68kops.c"
    )
    add_dependencies(musashi m68kmake)
endif()

target_compile_options(musashi PRIVATE -w)
target_include_directories(musashi PUBLIC "${MUSASHI_SRC}" "${MUSASHI_GEN}")
