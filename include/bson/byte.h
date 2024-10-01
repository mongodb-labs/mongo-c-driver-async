#pragma once

#include <mlib/config.h>

#include <stdint.h>

#if mlib_is_cxx()
#include <cstddef>
#include <cstdint>
#endif

/**
 * @brief A type that is the size of a byte, but does not alias with other types
 * (except char)
 */
typedef struct bson_byte {
    /// The 8-bit value of the byte
    uint8_t v;

#if mlib_is_cxx()
    // Explicit conversions for our byte type
    constexpr explicit operator std::byte() const noexcept { return std::byte(v); }
    constexpr explicit operator std::uint8_t() const noexcept { return v; }
    constexpr explicit operator char() const noexcept { return static_cast<char>(v); }
#endif
} bson_byte;
