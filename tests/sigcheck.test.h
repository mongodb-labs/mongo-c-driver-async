#pragma once

#include <amongoc/amongoc.h>
#include <amongoc/async.h>
#include <amongoc/box.h>
#include <amongoc/emitter.h>
#include <amongoc/status.h>

#include <bson/mut.h>
#include <bson/types.h>
#include <bson/value.h>
#include <bson/value_ref.h>
#include <bson/view.h>

#include <mlib/alloc.h>
#include <mlib/config.h>
#include <mlib/str.h>

#if mlib_is_cxx()
#include <string>
#endif

mlib_extern_c_begin();
mlib_diagnostic_push();
mlib_gnu_warning_disable("-Wuninitialized");

#define GLOBAL_SCOPE MLIB_IF_CXX(::)

static inline void amongoc_test_all_signatures() {
    bson_view       some_bson_view  = bson_view_null;
    bson_array_view some_bson_array = bson_array_view_null;
    bson_doc        some_bson_doc   = bson_new();
    bson_mut        some_bson_mut   = bson_mutate(&some_bson_doc);

    some_bson_doc = GLOBAL_SCOPE bson_new(42, mlib_default_allocator);
    some_bson_doc = GLOBAL_SCOPE bson_new(some_bson_view, mlib_default_allocator);
    some_bson_doc = GLOBAL_SCOPE bson_new(some_bson_doc, mlib_default_allocator);
    some_bson_doc = GLOBAL_SCOPE bson_new(some_bson_mut, mlib_default_allocator);
    some_bson_doc = GLOBAL_SCOPE bson_new(some_bson_array, mlib_default_allocator);
    some_bson_doc = GLOBAL_SCOPE bson_new();
    some_bson_doc = GLOBAL_SCOPE bson_new(42);
    some_bson_doc = GLOBAL_SCOPE bson_new(mlib_default_allocator);
    some_bson_doc = GLOBAL_SCOPE bson_new(some_bson_view);
    some_bson_doc = GLOBAL_SCOPE bson_new(some_bson_doc);
    some_bson_doc = GLOBAL_SCOPE bson_new(some_bson_mut);
    some_bson_doc = GLOBAL_SCOPE bson_new(some_bson_array);
    const bson_byte*             some_cbyte_ptr;
    (void)some_cbyte_ptr;
    some_cbyte_ptr = GLOBAL_SCOPE bson_data(some_bson_view);
    some_cbyte_ptr = GLOBAL_SCOPE bson_data(some_bson_doc);
    some_cbyte_ptr = GLOBAL_SCOPE bson_data(some_bson_mut);
    some_cbyte_ptr = GLOBAL_SCOPE bson_data(some_bson_array);
    bson_byte*                    some_mbyte_ptr;
    (void)some_mbyte_ptr;
    some_mbyte_ptr = GLOBAL_SCOPE bson_mut_data(some_bson_doc);
    some_mbyte_ptr = GLOBAL_SCOPE bson_mut_data(some_bson_mut);

    uint32_t                some_u32;
    some_u32 = GLOBAL_SCOPE bson_size(some_bson_view);
    some_u32 = GLOBAL_SCOPE bson_size(some_bson_doc);
    some_u32 = GLOBAL_SCOPE bson_size(some_bson_mut);
    some_u32 = GLOBAL_SCOPE bson_size(some_bson_array);
    (void)some_u32;
    int32_t                 some_i32;
    some_i32 = GLOBAL_SCOPE bson_ssize(some_bson_view);
    some_i32 = GLOBAL_SCOPE bson_ssize(some_bson_doc);
    some_i32 = GLOBAL_SCOPE bson_ssize(some_bson_mut);
    some_i32 = GLOBAL_SCOPE bson_ssize(some_bson_array);
    (void)some_i32;
    // bson_view_from
    some_bson_view = GLOBAL_SCOPE bson_view_from(some_bson_view);
    some_bson_view = GLOBAL_SCOPE bson_view_from(some_bson_array);
    some_bson_view = GLOBAL_SCOPE bson_view_from(some_bson_doc);
    some_bson_view = GLOBAL_SCOPE bson_view_from(some_bson_mut);
#if mlib_is_cxx()
    bson::document cxx_doc{::mlib_default_allocator};
    bson::mutator  cxx_mut{cxx_doc};
    some_bson_doc  = ::bson_new(cxx_doc);
    some_bson_doc  = ::bson_new(cxx_doc, ::mlib_default_allocator);
    some_bson_doc  = ::bson_new(cxx_mut);
    some_bson_doc  = ::bson_new(cxx_mut, ::mlib_default_allocator);
    some_bson_view = ::bson_view_from(cxx_doc);
    some_bson_view = ::bson_view_from(cxx_mut);
    some_cbyte_ptr = ::bson_data(cxx_mut);
    some_cbyte_ptr = ::bson_data(cxx_doc);
    some_u32       = ::bson_size(cxx_doc);
    some_u32       = ::bson_size(cxx_mut);
    some_i32       = ::bson_ssize(cxx_doc);
    some_i32       = ::bson_ssize(cxx_mut);
#endif

    // bson_value_ref_from
    bson_value_ref bref;
    (void)bref;
    bref = GLOBAL_SCOPE bson_value_ref_from(1.2);
    bref = GLOBAL_SCOPE bson_value_ref_from(1.2f);
    bref = GLOBAL_SCOPE bson_value_ref_from((int8_t)42);
    bref = GLOBAL_SCOPE bson_value_ref_from((uint8_t)42);
    bref = GLOBAL_SCOPE bson_value_ref_from((int16_t)42);
    bref = GLOBAL_SCOPE bson_value_ref_from((uint16_t)42);
    bref = GLOBAL_SCOPE bson_value_ref_from((int32_t)42);
    bref = GLOBAL_SCOPE bson_value_ref_from((uint32_t)42);
    bref = GLOBAL_SCOPE bson_value_ref_from((int64_t)42);
    /// ! This one intentionally fails, since such a conversion would cause narrowing:
    // bref = GLOBAL_SCOPE bson_value_ref_from((uint64_t)42);
    bson_value bval;
    (void)bval;
    bref = GLOBAL_SCOPE bson_value_ref_from(bval);
    bref = GLOBAL_SCOPE bson_value_ref_from(bref);
    bref = GLOBAL_SCOPE bson_value_ref_from(some_bson_view);
    bref = GLOBAL_SCOPE bson_value_ref_from(some_bson_doc);
    bref = GLOBAL_SCOPE bson_value_ref_from(some_bson_mut);
    bref = GLOBAL_SCOPE bson_value_ref_from(some_bson_array);
    bref = GLOBAL_SCOPE bson_value_ref_from("hey");
    mlib_str            some_string;
    const char*         cstr_ptr = "hey";
    mlib_str_view       some_string_view;
    mlib_str_mut        some_string_mut;
    bref = GLOBAL_SCOPE bson_value_ref_from(cstr_ptr);
    bref = GLOBAL_SCOPE bson_value_ref_from(some_string);
    bref = GLOBAL_SCOPE bson_value_ref_from(some_string_view);
    bref = GLOBAL_SCOPE bson_value_ref_from(some_string_mut);
#if mlib_is_cxx()
    std::string      std_string;
    std::string_view std_string_view;
    bref = ::bson_value_ref_from(std_string);
    bref = ::bson_value_ref_from(std_string_view);
    bref = ::bson_value_ref_from(cxx_doc);
    bref = ::bson_value_ref_from(cxx_mut);
#endif
    bson_binary_view    some_binary;
    bref = GLOBAL_SCOPE bson_value_ref_from(some_binary);
    bson_oid            some_oid;
    bref = GLOBAL_SCOPE bson_value_ref_from(some_oid);
    bson_datetime       some_datetime;
    bref = GLOBAL_SCOPE bson_value_ref_from(some_datetime);
    bson_regex_view     some_regex_view;
    bref = GLOBAL_SCOPE bson_value_ref_from(some_regex_view);
    bson_dbpointer_view some_dbpointer;
    bref = GLOBAL_SCOPE bson_value_ref_from(some_dbpointer);
    bson_code_view      some_code;
    bref = GLOBAL_SCOPE bson_value_ref_from(some_code);
    bson_symbol_view    some_symbol;
    bref = GLOBAL_SCOPE bson_value_ref_from(some_symbol);
    bson_timestamp      some_timestamp;
    bref = GLOBAL_SCOPE bson_value_ref_from(some_timestamp);
    bson_decimal128     some_decimal;
    bref = GLOBAL_SCOPE bson_value_ref_from(some_decimal);

    // ? bson_value_copy does not need checking because it is written in terms of
    // ? bson_value_ref_from

    amongoc_emitter          some_emitter;
    amongoc_box              some_userdata = amongoc_nil;
    enum amongoc_async_flags some_aflags   = amongoc_async_default;
    amongoc_box (*then_fn)(amongoc_box userdata, amongoc_status* status, amongoc_box result) = NULL;
    amongoc_emitter (*let_fn)(amongoc_box userdata, amongoc_status status, amongoc_box result)
        = NULL;
    some_emitter = GLOBAL_SCOPE amongoc_then(some_emitter, then_fn);
    some_emitter = GLOBAL_SCOPE amongoc_let(some_emitter, let_fn);
    some_emitter = GLOBAL_SCOPE amongoc_then(some_emitter, some_userdata, then_fn);
    some_emitter = GLOBAL_SCOPE amongoc_let(some_emitter, some_userdata, let_fn);
    some_emitter = GLOBAL_SCOPE amongoc_then(some_emitter, amongoc_async_default, then_fn);
    some_emitter = GLOBAL_SCOPE amongoc_let(some_emitter, amongoc_async_default, let_fn);
    some_emitter = GLOBAL_SCOPE amongoc_then(some_emitter, some_aflags, then_fn);
    some_emitter = GLOBAL_SCOPE amongoc_let(some_emitter, some_aflags, let_fn);
    some_emitter
        = GLOBAL_SCOPE amongoc_then(some_emitter, amongoc_async_default, some_userdata, then_fn);
    some_emitter
        = GLOBAL_SCOPE amongoc_let(some_emitter, amongoc_async_default, some_userdata, let_fn);
    some_emitter = GLOBAL_SCOPE amongoc_then(some_emitter, some_aflags, some_userdata, then_fn);
    some_emitter = GLOBAL_SCOPE amongoc_let(some_emitter, some_aflags, some_userdata, let_fn);
    some_emitter
        = GLOBAL_SCOPE amongoc_then(some_emitter, mlib_default_allocator, some_userdata, then_fn);
    some_emitter
        = GLOBAL_SCOPE amongoc_let(some_emitter, mlib_default_allocator, some_userdata, let_fn);
    some_emitter = GLOBAL_SCOPE amongoc_then(some_emitter,
                                             amongoc_async_default,
                                             mlib_default_allocator,
                                             some_userdata,
                                             then_fn);
    some_emitter = GLOBAL_SCOPE amongoc_let(some_emitter,
                                            amongoc_async_default,
                                            mlib_default_allocator,
                                            some_userdata,
                                            let_fn);
    some_emitter = GLOBAL_SCOPE amongoc_then(some_emitter,
                                             some_aflags,
                                             mlib_default_allocator,
                                             some_userdata,
                                             then_fn);
    some_emitter = GLOBAL_SCOPE amongoc_let(some_emitter,
                                            some_aflags,
                                            mlib_default_allocator,
                                            some_userdata,
                                            let_fn);

    // just()
    some_emitter = GLOBAL_SCOPE amongoc_just(amongoc_okay, amongoc_nil, mlib_default_allocator);
    some_emitter = GLOBAL_SCOPE amongoc_just(amongoc_okay);
    some_emitter = GLOBAL_SCOPE amongoc_just(amongoc_nil);
    some_emitter = GLOBAL_SCOPE amongoc_just(amongoc_okay, amongoc_nil);
    some_emitter = GLOBAL_SCOPE amongoc_just(amongoc_nil, mlib_default_allocator);
    some_emitter = GLOBAL_SCOPE amongoc_just();
}

mlib_diagnostic_pop();
mlib_extern_c_end();
