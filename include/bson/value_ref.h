#pragma once

#include <bson/types.h>
#include <bson/view.h>

#include <mlib/config.h>
#include <mlib/str.h>

#include <math.h>

#if mlib_is_cxx()
#include <mlib/type_traits.hpp>
#endif

struct bson_value;

/**
 * @brief A reference to a dynamically typed BSON value
 */
typedef struct bson_value_ref {
    /**
     * @brief The type of the referred-to value.
     *
     * The special type bson_type_eod (zero) indicates a null reference
     */
    bson_type type;
    union {
#define X(_octet, ViewType, _own_type, _basename, SafeName) ViewType SafeName;
        BSON_TYPE_X_LIST
#undef X
    };

#if mlib_is_cxx()
    constexpr bool     has_value() const noexcept { return type != bson_type_eod; }
    constexpr explicit operator bool() const noexcept { return has_value(); }

    template <typename F>
    constexpr decltype(auto) visit(F&& fn) const {
        switch (this->type) {
#define X(_octet, _viewtype, _ownertype, BaseName, SafeName)                                       \
    case MLIB_PASTE(bson_type_, BaseName):                                                         \
        return fn(this->SafeName);
            BSON_TYPE_X_LIST
#undef X
        case bson_type_codewscope:
            assert(false && "TODO: Code with scope");
            abort();
        default:
            std::terminate();
        }
    }

    // Conversions
    // Copy an existing value ref
    template <typename R>
    constexpr static mlib::detail::requires_t<bson_value_ref, std::is_same<R, bson_value_ref>>
    from(R r) noexcept {
        return r;
    }

    constexpr static bson_value_ref from(bson_value const&) noexcept;  // Defined in value.h

#define X(_octet, ViewType, _owner, BaseName, SafeName)                                            \
    constexpr bool MLIB_PASTE(is_, BaseName)() const noexcept {                                    \
        return type == MLIB_PASTE(bson_type_, BaseName);                                           \
    }                                                                                              \
    constexpr ViewType MLIB_PASTE(get_, BaseName)(ViewType default_value = ViewType())             \
        const noexcept {                                                                           \
        if (not this->MLIB_PASTE(is_, BaseName)())                                                 \
            return default_value;                                                                  \
        return this->SafeName;                                                                     \
    }
    BSON_TYPE_X_LIST
#undef X

#define DECL_CONVERSION(ParamType, TypeEnumerator, Member, Init)                                   \
    constexpr static bson_value_ref from(ParamType arg [[maybe_unused]]) noexcept {                \
        return bson_value_ref{.type = TypeEnumerator, .Member = Init};                             \
    }                                                                                              \
    static_assert(true, "")
    DECL_CONVERSION(double, ::bson_type_double, double_, arg);
    DECL_CONVERSION(std::string_view, ::bson_type_utf8, utf8, mlib_str_view::from(arg));
    DECL_CONVERSION(bson::view, ::bson_type_document, document, arg);
    DECL_CONVERSION(bson_array_view, ::bson_type_array, array, arg);
    DECL_CONVERSION(bson_binary_view, ::bson_type_binary, binary, arg);
    DECL_CONVERSION(bson_oid, ::bson_type_oid, oid, arg);
    DECL_CONVERSION(bson_datetime, ::bson_type_datetime, datetime, arg);
    DECL_CONVERSION(bson_regex_view, ::bson_type_regex, regex, arg);
    DECL_CONVERSION(bson_dbpointer_view, ::bson_type_dbpointer, dbpointer, arg);
    DECL_CONVERSION(bson_code_view, ::bson_type_code, code, arg);
    DECL_CONVERSION(bson_symbol_view, ::bson_type_symbol, symbol, arg);
    DECL_CONVERSION(std::int32_t, ::bson_type_int32, int32, arg);
    DECL_CONVERSION(bson_timestamp, ::bson_type_timestamp, timestamp, arg);
    DECL_CONVERSION(std::uint32_t, ::bson_type_int64, int64, arg);
    DECL_CONVERSION(std::int64_t, ::bson_type_int64, int64, arg);
    DECL_CONVERSION(bson_decimal128, ::bson_type_decimal128, decimal128, arg);
    DECL_CONVERSION(bson::null, ::bson_type_null, int32, 0);
    DECL_CONVERSION(bson::undefined, ::bson_type_undefined, int32, 0);
    DECL_CONVERSION(bson::maxkey, ::bson_type_maxkey, int32, 0);
    DECL_CONVERSION(bson::minkey, ::bson_type_minkey, int32, 0);
#undef DECL_CONVERSION
    template <typename B, int = 0>
    constexpr static mlib::detail::requires_t<bson_value_ref, std::is_same<B, bool>>
    from(B b) noexcept {
        return bson_value_ref{.type = ::bson_type_bool, .bool_ = b};
    }

    constexpr bool         as_bool() const noexcept;
    constexpr std::int32_t as_int32(bool* okay = nullptr) const noexcept;
    constexpr std::int64_t as_int64(bool* okay = nullptr) const noexcept;
    constexpr double       as_double(bool* okay = nullptr) const noexcept;

    /**
     * @brief Obtain a document or array view from the value reference.
     *
     * @param dflt The default value if the referred-to value is not a document nor an array
     *
     * If the value holds a document, returns a view of that document. If it holds
     * an array, returns a view of that array as a `bson_view` with integer keys.
     * If neither, returns the default value argument.
     */
    constexpr bson_view get_document_or_array(bson_view dflt = {}) const noexcept {
        if (this->type == bson_type_document) {
            return this->get_document();
        } else if (this->type == bson_type_array) {
            return bson_view(this->get_array());
        } else {
            return dflt;
        }
    }

    bool operator==(const bson_value_ref& oher) const noexcept;

    template <typename I>
    constexpr mlib::detail::requires_t<bool, std::is_integral<I>>
    operator==(const I i) const noexcept {
        bool is_int = false;
        bool eq_i   = this->as_int64(&is_int) == i;
        return eq_i and is_int;
    }

    constexpr bool operator==(std::string_view sv) const noexcept {
        return this->is_utf8() and this->get_utf8() == sv;
    }

#endif  // C++
} bson_value_ref;

mlib_extern_c_begin();

mlib_constexpr bool bson_value_as_bool(bson_value_ref val) mlib_noexcept {
    switch (val.type) {
    case bson_type_eod:
        return false;
    case bson_type_double:
        return fpclassify(val.double_) != FP_ZERO;
    case bson_type_utf8:
        return val.utf8.len != 0;
    case bson_type_document:
    case bson_type_array: {
        // Return `true` if the pointed-to document is non-empty
        return _bson_read_u32le(bson_data(val.document)) > 5;
    }
    case bson_type_binary: {
        bson_binary_view bin = val.binary;
        return bin.data_len != 0;
    }
    case bson_type_undefined:
        return false;
    case bson_type_oid:
        return true;
    case bson_type_bool:
        return val.bool_;
    case bson_type_datetime:
        return true;
    case bson_type_null:
        return false;
    case bson_type_regex:
    case bson_type_dbpointer:
    case bson_type_code:
    case bson_type_symbol:
    case bson_type_codewscope:
        return true;
    case bson_type_int32:
        return val.int32 != 0;
    case bson_type_timestamp:
        return true;
    case bson_type_int64:
        return val.int64 != 0;
    case bson_type_decimal128:
        return true;
    case bson_type_maxkey:
    case bson_type_minkey:
        return false;
    }
    return false;
}

mlib_constexpr double bson_value_as_double(bson_value_ref val, bool* okay) mlib_noexcept {
    switch (val.type) {
    case bson_type_double:
        (okay && (*okay = true));
        return val.double_;
    case bson_type_eod:
    case bson_type_utf8:
    case bson_type_document:
    case bson_type_array:
    case bson_type_binary:
    case bson_type_undefined:
    case bson_type_oid:
        (okay && (*okay = false));
        return 0;
    case bson_type_bool:
        (okay && (*okay = true));
        return val.bool_ ? 1 : 0;
    case bson_type_datetime:
    case bson_type_null:
    case bson_type_regex:
    case bson_type_dbpointer:
    case bson_type_code:
    case bson_type_symbol:
    case bson_type_codewscope:
        (okay && (*okay = false));
        return 0;
    case bson_type_int32:
        (okay && (*okay = true));
        return val.int32;
    case bson_type_timestamp:
        (okay && (*okay = false));
        return 0;
    case bson_type_int64:
        (okay && (*okay = true));
        return (double)val.int64;
    case bson_type_decimal128:
    case bson_type_maxkey:
    case bson_type_minkey:
    default:
        (okay && (*okay = false));
        return 0;
    }
}

mlib_constexpr int64_t bson_value_as_int64(bson_value_ref val, bool* okay) mlib_noexcept {
    switch (val.type) {

    case bson_type_eod:
    case bson_type_utf8:
    case bson_type_document:
    case bson_type_array:
    case bson_type_binary:
    case bson_type_undefined:
    case bson_type_oid:
    case bson_type_datetime:
    case bson_type_null:
    case bson_type_regex:
    case bson_type_dbpointer:
    case bson_type_code:
    case bson_type_symbol:
    case bson_type_codewscope:
    case bson_type_timestamp:
    case bson_type_decimal128:
    case bson_type_maxkey:
    case bson_type_minkey:
    default:
        (okay && (*okay = false));
        return 0;
    case bson_type_double:
        (okay && (*okay = true));
        return (int64_t)val.double_;
    case bson_type_bool:
        (okay && (*okay = true));
        return val.bool_ ? 1 : 0;
    case bson_type_int32:
        (okay && (*okay = true));
        return val.int32;
    case bson_type_int64:
        (okay && (*okay = true));
        return val.int64;
    }
}

mlib_constexpr int32_t bson_value_as_int32(bson_value_ref val, bool* okay) mlib_noexcept {
    return (int32_t)bson_value_as_int64(val, okay);
}

mlib_extern_c_end();

#if mlib_is_cxx()

constexpr bool   bson_value_ref::as_bool() const noexcept { return ::bson_value_as_bool(*this); }
constexpr double bson_value_ref::as_double(bool* okay) const noexcept {
    return ::bson_value_as_double(*this, okay);
}
constexpr int32_t bson_value_ref::as_int32(bool* okay) const noexcept {
    return ::bson_value_as_int32(*this, okay);
}
constexpr int64_t bson_value_ref::as_int64(bool* okay) const noexcept {
    return ::bson_value_as_int64(*this, okay);
}

#endif  // C++
