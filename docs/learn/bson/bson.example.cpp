#include "bson/iterator.h"
#include "bson/mut.h"
#include "bson/types.h"
#include "bson/view.h"
#include <bson/doc.h>

// ex: [create-c]
#include <bson/doc.h>

void c_create() {
    // Create a new document boject
    bson_doc doc = bson_new();

    // Do stuff

    // Destroy the object
    bson_delete(doc);
}
// end.

// ex: [create-cxx]
#include <bson/doc.h>

void cxx_create() {
    // Will be destroyed automatically
    bson::document doc;
}
// end.

// ex: [copying]
const bson_doc* get_some_bson_doc(void);

void copy_a_document() {
    bson_doc dup = bson_new(*get_some_bson_doc());
    // ...
    bson_delete(dup);
}
// end.

// ex: [reserve]
void reserve_space() {
    bson_doc large = bson_new(1024);
    // ...
    bson_delete(large);
}
// end.

// ex: [mutate]
#include <bson/mut.h>

void mutate_a_doc() {
    bson_doc doc = bson_new();

    // Begin mutation
    bson_mut mut = bson_mutate(&doc);
    (void)mut;

    bson_delete(doc);
}
// end.

// ex: [insert]
void insert_something() {
    bson_doc doc = bson_new();
    bson_mut m   = bson_mutate(&doc);
    /* Content:
        {}
    */
    bson_insert(&m, "foo", "bar");
    /* Content:
        {
            "foo": "bar"
        }
    */
    bson_delete(doc);
}
// end.

// ex: [insert-begin]
void prepend_something() {
    bson_doc doc = bson_new();
    bson_mut m   = bson_mutate(&doc);
    /* Content:
        {}
    */
    bson_insert(&m, bson_begin(doc), "foo", "bar");
    /* Content:
        {
            "foo": "bar"
        }
    */
    bson_insert(&m, bson_begin(doc), "baz", "quux");
    /* Content:
        {
            "baz": "quux",
            "foo": "bar"
        }
    */
    bson_delete(doc);
}
// end.

// ex: [subdoc-mutate]
void subdoc_modify() {
    bson_doc      doc     = bson_new();
    bson_mut      top_mut = bson_mutate(&doc);
    bson_iterator it      = bson_insert(&top_mut, "child", BSON_VIEW_NULL);
    /* Content:
        {
            "child": {}
        }
    */
    bson_mut child_mut = bson_mut_child(&top_mut, it);
    bson_insert(&child_mut, "foo", "Within a child document");
    /* Content:
        {
            "child": {
                "foo": "Within a child document"
            }
        }
    */
    bson_delete(doc);
}
// end.

// ex: [as-view]
void do_something(bson_view) { /* ... */ }

void get_view() {
    bson_doc d = bson_new();
    bson_mut m = bson_mutate(&d);
    bson_insert(&m, "foo", "bar");

    // Pass a view to `do_something`
    do_something(bson_as_view(d));

    bson_delete(d);
}
// end.

// ex: [iter-begin]
void inspect_data(bson_view v) {
    bson_iterator it = bson_begin(v);
    printf("Element key is '%s'\n", bson_key(it).data);
}
// end.

// ex: [for-loop]
void do_loop(bson_view data) {
    for (bson_iterator it = bson_begin(data);  // init
         !bson_stop(it);                       // guard
         it = bson_next(it)) {                 // step
        printf("Got an element: %s\n", bson_key(it).data);
    }
}
// end.

// ex: [foreach]
void foreach_loop(bson_view data) {
    bson_foreach(it, data) {
        // `it` refers to the current element.
        printf("Got an element: %s\n", bson_key(it).data);
    }
}
// end.

// ex: [c++-for]
void cxx_for(bson_view data) {
    for (bson_iterator::reference elem : data) {
        // `it` refers to the current element.
        printf("Got an element: %s\n", elem.key().data());
    }
}
// end.

// ex: [get-value]
void get_double(bson_view v) {
    bson_iterator it = bson_begin(v);
    if (bson_iterator_type(it) != bson_type_utf8) {
        fputs("Expected a UTF-8 element", stderr);
        return;
    }
    printf("Element '%s' has value '%s'\n", bson_key(it).data, bson_iterator_utf8(it).data);
}
// end.

// ex: [subdoc-iter]
void do_something_with(bson_iterator) { /* ... */ }

bool subdoc_iter(bson_view top) {
    // Find an element called "some-array"
    bson_iterator it = bson_find(top, "some-array");
    if (bson_stop(it)) {
        // The element was not found
        fputs("Did not find a 'some-array' element", stderr);
        return false;
    }

    // Check that it is actually an array
    if (bson_iterator_type(it) != bson_type_array) {
        fputs("Expected an array element", stderr);
        return false;
    }

    // Decode the array as a subdocument:
    bson_view_errc err;
    bson_view      array = bson_iterator_document(it, &err);
    if (bson_data(array) == NULL) {
        // Decoding the subdocument failed
        fprintf(stderr, "Failed to decode child document [error %d]", err);
        return false;
    }

    // Iterate over each element of the array
    bson_foreach(sub_iter, array) {
        if (bson_iterator_get_error(sub_iter)) {
            // Iterating over a child element encountered an error
            fprintf(stderr,
                    "A subdocument array element is malformed [error %d]",
                    bson_iterator_get_error(sub_iter));
            return false;
        }
        // Pass an iterator within the child document:
        do_something_with(sub_iter);
    }
    return true;
}
// end.

int main() {
    c_create();
    cxx_create();
    copy_a_document();
    reserve_space();
    mutate_a_doc();
    insert_something();
    prepend_something();
    subdoc_modify();
    get_view();
    bson_doc tmp = bson_new();
    bson_mut m   = bson_mutate(&tmp);
    bson_insert(&m, "foo", "bar");
    inspect_data(bson_as_view(tmp));
    do_loop(bson_as_view(tmp));
    foreach_loop(bson_as_view(tmp));
    subdoc_iter(bson_as_view(tmp));
    bson_delete(tmp);
}

// Used by the [copying] example:
const bson_doc* get_some_bson_doc() {
    static bson_doc d = bson_new();
    return &d;
}
