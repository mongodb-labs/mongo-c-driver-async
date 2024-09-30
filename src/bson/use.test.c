
#include <bson/doc.h>
#include <bson/mut.h>
#include <bson/types.h>
#include <bson/view.h>

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

    bson_byte      bytes[512] = {{0}};
    bson_view      view       = BSON_VIEW_NULL;
    bson_doc       doc        = BSON_DOC_NULL;
    bson_mut       mut        = BSON_MUT_v2_NULL;
    bson_utf8_view u8         = BSON_UTF8_NULL;
    bson_iterator  iter       = BSON_ITERATOR_NULL;
    bson_code      code;
    bson_symbol    sym;
    bson_datetime  dt;
    bson_timestamp ts;

    // Viewing APIs
    bson_data(view);
    bson_mut_data(doc);
    bson_mut_data(mut);
    bson_size(view);
    bson_ssize(view);
    bson_view_from_data(bytes, sizeof bytes, NULL);
    bson_utf8_view_from_data("foo", 3);
    bson_utf8_view_from_cstring("foo");
    bson_utf8_view_autolen("foo", 3);
    bson_utf8_view_chopnulls(u8);
    bson_done(iter);
    bson_iterator_get_error(iter);
    bson_key(iter);
    bson_iterator_type(iter);
    bson_iterator_data(iter);
    bson_iterator_data_size(iter);
    bson_view_of(mut);
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
    bson_key_eq(iter, "foo", 3);

    // Document APIs
    bson_doc_capacity(doc);
    bson_doc_get_allocator(doc);
    bson_reserve(&doc, 42);
    bson_new_ex(mlib_default_allocator, 42);
    bson_new();
    bson_copy_view(view, mlib_default_allocator);
    bson_doc_copy(doc);
    bson_delete(doc);

    // Mutator
    bson_mutate(&doc);
    bson_mut_capacity(mut);
    bson_insert_double(&mut, iter, u8, 3.14);
    bson_mut_insert_utf8(&mut, iter, u8, u8);
    bson_insert_doc(&mut, iter, u8, view);
    bson_insert_array(&mut, iter, u8);
    bson_insert_binary(&mut, iter, u8, (bson_binary){});
    bson_insert_undefined(&mut, iter, u8);
    bson_insert_oid(&mut, iter, u8, (bson_oid){});
    bson_insert_bool(&mut, iter, u8, true);
    bson_insert_datetime(&mut, iter, u8, dt);
    bson_insert_null(&mut, iter, u8);
    bson_insert_regex(&mut, iter, u8, (bson_regex){});
    bson_insert_dbpointer(&mut, iter, u8, BSON_DBPOINTER_NULL);
    bson_insert_code(&mut, iter, u8, code);
    bson_insert_symbol(&mut, iter, u8, sym);
    bson_insert_code_with_scope(&mut, iter, u8, code, view);
    bson_insert_int32(&mut, iter, u8, 42);
    bson_insert_timestamp(&mut, iter, u8, ts);
    bson_insert_int64(&mut, iter, u8, 42);
    bson_insert_decimal128(&mut, iter, u8, (struct bson_decimal128){});
    bson_insert_maxkey(&mut, iter, u8);
    bson_insert_minkey(&mut, iter, u8);
    bson_set_key(&mut, iter, u8);
    bson_tmp_uint_string(42);
    bson_relabel_array_elements_at(&mut, iter, 0);
    bson_relabel_array_elements(&mut);
    bson_splice_disjoint_ranges(&mut, iter, iter, iter, iter);
    bson_insert_disjoint_range(&mut, iter, iter, iter);
    bson_erase_range(&mut, iter, iter);
    bson_mut_erase(&mut, iter);
    bson_mut_child(&mut, iter);
    bson_mut_parent_iterator(mut);
}
