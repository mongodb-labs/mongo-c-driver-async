
#include <bson/doc.h>
#include <bson/iterator.h>
#include <bson/mut.h>
#include <bson/types.h>
#include <bson/value.h>
#include <bson/view.h>

#include <mlib/alloc.h>
#include <mlib/str.h>

#include <stdio.h>

mlib_diagnostic_push();
mlib_gnu_warning_disable("-Wuninitialized");

int main(int argc, char** argv) {
    puts(
        "This executable does nothing. It's purpose is to check the linkability of the BSON "
        "library.");
    if (argc) {  // Prevent DCE
        return 0;
    }
    (void)argv;

    bson_byte              bytes[512] = {{0}};
    bson_view              view       = bson_view_null;
    bson_doc               doc        = bson_new();
    bson_mut               mut        = BSON_MUT_v2_NULL;
    mlib_str               u8str      = mlib_str_null;
    mlib_str_view          u8view     = mlib_str_view_null;
    bson_iterator          iter       = BSON_ITERATOR_NULL;
    bson_code_view         code;
    bson_symbol_view       sym;
    bson_datetime          dt;
    bson_timestamp         ts;
    bson_binary_view       bin;
    bson_oid               oid;
    bson_regex_view        rx;
    struct bson_decimal128 dec;
    bson_dbpointer_view    dbp;

    bson_value_ref vref = bson_value_ref_from(42);
    bson_value     val  = bson_value_copy(42);

    // Viewing APIs
    bson_data(view);
    bson_mut_data(doc);
    bson_mut_data(mut);
    bson_size(view);
    bson_ssize(view);
    bson_view_from_data(bytes, sizeof bytes, NULL);
    bson_stop(iter);
    bson_iterator_get_error(iter);
    bson_key(iter);
    bson_iterator_type(iter);
    bson_iterator_data(iter);
    (void)bson_view_from(mut);
    bson_begin(doc);
    bson_end(doc);
    bson_iterator_eq(iter, iter);
    bson_key_eq(iter, u8view);

    bson_insert(&mut, "hey", 1.0);
    bson_insert(&mut, iter, "hey", 1.0);
    bson_insert(&mut, "hey", u8view);
    bson_insert(&mut, "hey", "hi");
    bson_insert(&mut, "hey", view);
    bson_insert(&mut, "hey", doc);
    bson_insert(&mut, "hey", mut);
    bson_insert(&mut, "hey", bin);
    bson_insert(&mut, "hey", oid);
    bson_insert(&mut, "hey", true);
    bson_insert(&mut, "hey", dt);
    bson_insert(&mut, "hey", rx);
    bson_insert(&mut, "hey", dbp);
    bson_insert(&mut, "hey", code);
    bson_insert(&mut, "hey", sym);
    bson_insert(&mut, "hey", (int32_t)42);
    bson_insert(&mut, "hey", ts);
    bson_insert(&mut, "hey", (int64_t)42);
    bson_insert(&mut, "hey", dec);
    bson_insert(&mut, "hey", vref);

    // Document APIs
    bson_doc_capacity(doc);
    bson_doc_get_allocator(doc);
    bson_doc_reserve(&doc, 42);
    bson_new();                              // [[1]]
    bson_new(5);                             // [[2]]
    bson_new(5, mlib_default_allocator);     // [[3]]
    bson_new(mlib_default_allocator);        // [[4]]
    bson_new(doc);                           // [[5]]
    bson_new(view);                          // [[6]]
    bson_new(view, mlib_default_allocator);  // [[7]] v1
    bson_new(doc, mlib_default_allocator);   // [[7]] v2
    bson_delete(doc);

    // Mutator
    bson_mutate(&doc);
    bson_mut_capacity(mut);

    bson_insert_code_with_scope(&mut, iter, u8view, code, view);

    bson_set_key(&mut, iter, u8view);
    bson_u32_string_create(42);
    bson_relabel_array_elements_at(&mut, iter, 0);
    bson_relabel_array_elements(&mut);
    bson_splice_disjoint_ranges(&mut, iter, iter, iter, iter);
    bson_insert_disjoint_range(&mut, iter, iter, iter);
    bson_erase_range(&mut, iter, iter);
    bson_erase(&mut, iter);
    bson_mut_child(&mut, iter);
    bson_mut_parent_iterator(mut);

    bson_value_ref_from(3.1);
    bson_value_ref_from(3.1f);
    bson_value_ref_from(u8str);
    bson_value_ref_from(u8view);
    bson_value_ref_from("hello");
    bson_value_ref_from(view);
    bson_value_ref_from(doc);
    bson_value_ref_from(mut);
    bson_value_ref_from(bin);
    bson_value_ref_from(true);
    bson_value_ref_from(dt);
    bson_value_ref_from(rx);
    bson_value_ref_from(dbp);
    bson_value_ref_from(code);
    bson_value_ref_from(sym);
    bson_value_ref_from((int32_t)42);
    bson_value_ref_from(ts);
    bson_value_ref_from((int64_t)42);
    bson_value_ref_from(dec);
    bson_value_ref_from(val);

    bson_value_delete(bson_value_copy(3.1));
    bson_value_delete(bson_value_copy(3.1f));
    bson_value_delete(bson_value_copy(u8str));
    bson_value_delete(bson_value_copy(u8view));
    bson_value_delete(bson_value_copy("Hello"));
    bson_value_delete(bson_value_copy(view));
    bson_value_delete(bson_value_copy(doc));
    bson_value_delete(bson_value_copy(mut));
    bson_value_delete(bson_value_copy(bin));
    bson_value_delete(bson_value_copy(true));
    bson_value_delete(bson_value_copy(dt));
    bson_value_delete(bson_value_copy(rx));
    bson_value_delete(bson_value_copy(dbp));
    bson_value_delete(bson_value_copy(code));
    bson_value_delete(bson_value_copy(sym));
    bson_value_delete(bson_value_copy((int32_t)42));
    bson_value_delete(bson_value_copy(ts));
    bson_value_delete(bson_value_copy((int64_t)42));
    bson_value_delete(bson_value_copy(dec));

    return 0;
}
