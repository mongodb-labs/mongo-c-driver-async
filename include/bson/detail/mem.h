#pragma once

#include <bson/byte.h>

#include <mlib/config.h>

#include <string.h>  // memcpy

mlib_extern_c_begin();

/**
 * @internal
 * @brief Write a 2's complement little-endian 32-bit integer into the given
 * memory location.
 *
 * @param bytes The destination to write into
 * @param v The value to write
 */
mlib_constexpr bson_byte* _bson_write_u32le(bson_byte* bytes, uint32_t u32) mlib_noexcept {
    if (mlib_is_consteval() || !mlib_is_little_endian()) {
        bytes[0].v = (u32 >> 0) & 0xff;
        bytes[1].v = (u32 >> 8) & 0xff;
        bytes[2].v = (u32 >> 16) & 0xff;
        bytes[3].v = (u32 >> 24) & 0xff;
    } else {
        memcpy(bytes, &u32, sizeof u32);
    }
    return bytes + 4;
}

/**
 * @internal
 * @brief Write a 2's complement little-endian 64-bit integer into the given
 * memory location.
 *
 * @param bytes The destination of the write
 * @param v  The value to write
 */
mlib_constexpr bson_byte* _bson_write_u64le(bson_byte* out, uint64_t u64) mlib_noexcept {
    if (mlib_is_consteval() || !mlib_is_little_endian()) {
        out = _bson_write_u32le(out, (uint32_t)(u64));
        out = _bson_write_u32le(out, (uint32_t)(u64 >> 32));
    } else {
        memcpy(out, &u64, sizeof u64);
        out += sizeof u64;
    }
    return out;
}

/**
 * @internal
 * @brief Read a little-endian 32-bit unsigned integer from the given array of
 * four octets
 *
 * @param bytes Pointer-to an array of AT LEAST four octets
 * @return uint32_t The decoded integer value.
 */
mlib_constexpr uint32_t _bson_read_u32le(const bson_byte* bytes) mlib_noexcept {
    uint32_t u32 = 0;
    if (mlib_is_consteval() || !mlib_is_little_endian()) {
        // Platform-independent int32 reading
        u32 |= bytes[3].v;
        u32 <<= 8;
        u32 |= bytes[2].v;
        u32 <<= 8;
        u32 |= bytes[1].v;
        u32 <<= 8;
        u32 |= bytes[0].v;
    } else {
        // Optimize to a memcpy
        memcpy(&u32, bytes, sizeof u32);
    }
    return u32;
}

/**
 * @internal
 * @brief Read a little-elendian 64-bit unsigned integer from the given array of
 * eight octets.
 *
 * @param bytes Pointer-to an array of AT LEAST eight octets
 * @return uint64_t The decoded integer value.
 */
mlib_constexpr uint64_t _bson_read_u64le(const bson_byte* bytes) mlib_noexcept {
    if (mlib_is_consteval() || !mlib_is_little_endian()) {
        // Platform-indepentent int64 reading
        const uint64_t lo  = _bson_read_u32le(bytes);
        const uint64_t hi  = _bson_read_u32le(bytes + 4);
        const uint64_t u64 = (hi << 32) | lo;
        return u64;
    } else {
        // Optimize to a memcpy
        uint64_t n = 0;
        memcpy(&n, bytes, sizeof n);
        return n;
    }
}

/**
 * @brief Perform a memcpy into the destination, returning that address
 * immediately following the copied data
 *
 * @param out The destination at which to write
 * @param src The source of the copy
 * @param len The number of bytes to copy
 * @return bson_byte* The value of (out + len)
 */
mlib_constexpr bson_byte*
_bson_memcpy(bson_byte* dst, const bson_byte* src, size_t len) mlib_noexcept {
    if (src && mlib_is_consteval()) {
        for (size_t n = 0; n < len; ++n) {
            dst[n] = src[n];
        }
    } else if (src && len) {
        memcpy(dst, src, len);
    }
    return dst + len;
}

// constexpr-compatible memmove()
mlib_constexpr bson_byte*
_bson_memmove(bson_byte* dst, const bson_byte* src, size_t len) mlib_noexcept {
    if (src && mlib_is_consteval()) {
        if (src < dst) {
            for (size_t n = len; n; --n) {
                dst[n - 1] = src[n - 1];
            }
        } else {
            for (size_t n = 0; n < len; ++n) {
                dst[n] = src[n];
            }
        }

    } else if (src && len) {
        memmove(dst, src, len);
    }
    return dst + len;
}

// constexpr-comparible memset()
mlib_constexpr bson_byte* _bson_memset(bson_byte* dst, char v, size_t len) mlib_noexcept {
    if (mlib_is_consteval()) {
        for (size_t n = 0; n < len; ++n) {
            dst[n].v = (uint8_t)v;
        }
    } else {
        memset(dst, v, len);
    }
    return dst + len;
}

// constexpr-compatible memcpy() from char
mlib_constexpr bson_byte*
_bson_memcpy_chr(bson_byte* dst, const char* src, size_t len) mlib_noexcept {
    if (src && mlib_is_consteval()) {
        for (size_t n = 0; n < len; ++n) {
            dst[n].v = (uint8_t)src[n];
        }
    } else if (src && len) {
        memcpy(dst, src, len);
    }
    return dst + len;
}

// constexpr-compatible memcpy() from uint8_t
mlib_constexpr bson_byte*
_bson_memcpy_u8(bson_byte* dst, const uint8_t* src, size_t len) mlib_noexcept {
    if (src && mlib_is_consteval()) {
        for (uint32_t n = 0; n < len; ++n) {
            dst[n].v = src[n];
        }
    } else if (src && len) {
        memcpy(dst, src, len);
    }
    return dst + len;
}

/**
 * @internal
 * @brief Accepts a pointer-to-const `bson_byte` and returns it.
 *
 * Used as a type guard in the bson_data() macro
 */
mlib_constexpr const bson_byte* _bson_data_as_const(const bson_byte* p) mlib_noexcept { return p; }

// Equivalent to as_const, but accepts non-const
mlib_constexpr bson_byte* _bson_data_as_mut(bson_byte* p) mlib_noexcept { return p; }

mlib_constexpr uint32_t _bson_byte_size(const bson_byte* p) mlib_noexcept {
    // A null document has length=0
    if (!p) {
        return 0;
    }
    // The length of a document is contained in a four-bytes little-endian
    // encoded integer. This size includes the header, the document body, and the
    // null terminator byte on the document.
    return _bson_read_u32le(p);
}

// We implement ssize() as a separate function rather than a cast so that the
// scope operator in `::bson_ssize()` works correctly.
mlib_constexpr int32_t _bson_byte_ssize(const bson_byte* p) mlib_noexcept {
    return (int32_t)_bson_byte_size(p);
}

mlib_extern_c_end();
