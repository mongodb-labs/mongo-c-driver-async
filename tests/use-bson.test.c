
#include "bson/iterator.h"
#include <bson/doc.h>
#include <bson/mut.h>
#include <bson/types.h>
#include <bson/view.h>

#include "mlib/str.h"
#include <mlib/alloc.h>

#include <stdio.h>

int main(int argc, char** argv) {
    puts(
        "This executable does nothing. It's purpose is to check the linkability of the BSON "
        "library.");
    if (argc) {  // Prevent DCE
        return 0;
    }
    (void)argv;

    bson_byte              bytes[512] = {{0}};
    bson_view              view       = BSON_VIEW_NULL;
    bson_doc               doc        = bson_new();
    bson_mut               mut        = BSON_MUT_v2_NULL;
    mlib_str_view          u8         = mlib_str_view_null;
    bson_iterator          iter       = BSON_ITERATOR_NULL;
    bson_code              code;
    bson_symbol            sym;
    bson_datetime          dt;
    bson_timestamp         ts;
    bson_binary            bin;
    bson_oid               oid;
    bson_regex             rx;
    struct bson_decimal128 dec;
    bson_dbpointer         dbp;

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
    bson_iterator_data_size(iter);
    bson_as_view(mut);
    bson_begin(doc);
    bson_end(doc);
    bson_iterator_eq(iter, iter);
    bson_iterator_double(iter);
    bson_iterator_utf8(iter);
    bson_iterator_document(iter, NULL);
    bson_iterator_binary(iter);
    bson_iterator_oid(iter);
    bson_iterator_bool(iter);
    bson_iterator_datetime(iter);
    bson_iterator_regex(iter);
    bson_iterator_dbpointer(iter);
    bson_iterator_code(iter);
    bson_iterator_symbol(iter);
    bson_iterator_int32(iter);
    bson_iterator_timestamp(iter);
    bson_iterator_int64(iter);
    bson_iterator_decimal128(iter);
    bson_iterator_as_double(iter, NULL);
    bson_iterator_as_bool(iter);
    bson_iterator_as_int32(iter, NULL);
    bson_iterator_as_int64(iter, NULL);
    bson_key_eq(iter, u8);

    bson_insert(&mut, "hey", 1.0);
    bson_insert(&mut, iter, "hey", 1.0);
    bson_insert(&mut, "hey", u8);
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
    _bson_insert_double(&mut, iter, u8, 3.14);
    bson_insert_utf8(&mut, iter, u8, u8);
    _bson_insert_doc(&mut, iter, u8, view);
    _bson_insert_array(&mut, iter, u8);
    _bson_insert_binary(&mut, iter, u8, (bson_binary){});
    _bson_insert_undefined(&mut, iter, u8);
    _bson_insert_oid(&mut, iter, u8, (bson_oid){});
    _bson_insert_bool(&mut, iter, u8, true);
    _bson_insert_datetime(&mut, iter, u8, dt);
    _bson_insert_null(&mut, iter, u8);
    _bson_insert_regex(&mut, iter, u8, (bson_regex){});
    _bson_insert_dbpointer(&mut, iter, u8, BSON_DBPOINTER_NULL);
    _bson_insert_code(&mut, iter, u8, code);
    _bson_insert_symbol(&mut, iter, u8, sym);
    bson_insert_code_with_scope(&mut, iter, u8, code, view);
    _bson_insert_int32(&mut, iter, u8, 42);
    _bson_insert_timestamp(&mut, iter, u8, ts);
    _bson_insert_int64(&mut, iter, u8, 42);
    _bson_insert_decimal128(&mut, iter, u8, (struct bson_decimal128){});
    _bson_insert_maxkey(&mut, iter, u8);
    bson_insert_minkey(&mut, iter, u8);
    bson_set_key(&mut, iter, u8);
    bson_tmp_uint_string(42);
    bson_relabel_array_elements_at(&mut, iter, 0);
    bson_relabel_array_elements(&mut);
    bson_splice_disjoint_ranges(&mut, iter, iter, iter, iter);
    bson_insert_disjoint_range(&mut, iter, iter, iter);
    bson_erase_range(&mut, iter, iter);
    bson_erase(&mut, iter);
    bson_mut_child(&mut, iter);
    bson_mut_parent_iterator(mut);

    return 0;
}
