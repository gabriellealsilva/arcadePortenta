#pragma once

/// @file Log.h
/// @brief Conditional diagnostic logging for the emulator host build.
///
/// | Build type | @ref LOG behaviour                       |
/// |------------|------------------------------------------|
/// | Debug      | Forwards to @c SDL_Log (visible output)  |
/// | Release    | Compiles to nothing (zero overhead)      |
///
/// CMake adds @c -DNDEBUG automatically for @c CMAKE_BUILD_TYPE=Release,
/// so no extra define is needed.
///
/// SDL error messages (window creation failures, etc.) use @c SDL_Log directly
/// and are intentionally kept visible in both build types.

#ifdef NDEBUG

/// @brief Emit a diagnostic message.
/// In Release builds this expands to nothing and is fully optimised away.
#  define LOG(...) do {} while (0)

#else

#  ifndef SDL_MAIN_HANDLED
#    define SDL_MAIN_HANDLED
#  endif
#  include <SDL3/SDL.h>

/// @brief Emit a diagnostic message.
/// In Debug builds this forwards to @c SDL_Log.
#  define LOG SDL_Log

#endif
