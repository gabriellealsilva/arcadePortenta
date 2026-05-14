#pragma once
#include "RomDefs.h"
#include <concepts>
#include <span>

namespace cps1
{

/// @brief Concept that every ROM-source backend must satisfy.
///
/// Checked via @c static_assert once the derived class is complete.
/// The template parameter of @ref IRomSource cannot carry this constraint
/// directly because @c Derived is an incomplete type at base-class
/// instantiation time (a fundamental CRTP limitation).
///
/// @tparam T The candidate backend type.
template<typename T>
concept RomSourceImpl = requires(T&             src,
                                 const T&        csrc,
                                 const GameDescriptor& game,
                                 const RomEntry&       entry,
                                 std::span<uint8_t>    dest)
{
    { src.scanImpl() };
    { csrc.availableGamesImpl() } -> std::convertible_to<std::span<const GameDescriptor>>;
    { src.readRomImpl(game, entry, dest) } -> std::same_as<bool>;
};

/// @brief CRTP base providing the public ROM-source interface.
///
/// All dispatch is resolved at compile time — no vtable overhead.
/// Concrete backends:
///  - @c FilesystemRomSource — host emulator, reads from the local filesystem
///  - @c QspiRomSource       — Portenta firmware, reads from QSPI XIP flash
///
/// Each backend must satisfy @ref RomSourceImpl, enforced via
/// @c static_assert at the end of the derived class header.
///
/// @tparam Derived The concrete backend type (CRTP pattern).
template<typename Derived>
class IRomSource
{
public:
    /// @brief Scan for available games.
    ///
    /// Must be called exactly once before @ref availableGames or @ref readRom.
    void scan() { self().scanImpl(); }

    /// @brief Return the games discovered during the last @ref scan call.
    /// @return Non-owning span; valid as long as this object is alive and
    ///         @ref scan has not been called again.
    [[nodiscard]] std::span<const GameDescriptor> availableGames() const
    {
        return self().availableGamesImpl();
    }

    /// @brief Read one ROM file into a caller-supplied buffer.
    /// @param game  Game that owns the file.
    /// @param entry Identifies the specific file and its expected size.
    /// @param dest  Output buffer; must be at least @c entry.size bytes.
    /// @return @c true on success, @c false if the file cannot be opened
    ///         or the buffer is too small.
    [[nodiscard]] bool readRom(const GameDescriptor& game,
                               const RomEntry&       entry,
                               std::span<uint8_t>    dest)
    {
        return self().readRomImpl(game, entry, dest);
    }

private:
    Derived&       self()       { return *static_cast<Derived*>(this); }
    const Derived& self() const { return *static_cast<const Derived*>(this); }
};

} // namespace cps1
