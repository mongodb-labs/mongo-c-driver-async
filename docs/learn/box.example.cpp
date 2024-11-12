#include <amongoc/box.hpp>

#include <cstdio>
#include <utility>

bool some_condition() { return false; }
bool other_condition() { return false; }

amongoc_box get_some_box() { return ::amongoc_nil; }
amongoc_box get_different_box() { return ::amongoc_nil; }

// ex: [uses-nil]
void do_work(const amongoc_box*);

void uses_nil_box() {
    // Create a default nil value for the box
    amongoc_box val = amongoc_nil;
    if (some_condition()) {
        // Replace the nil box and do some work on it
        val = get_some_box();
        do_work(&val);
    } else if (other_condition()) {
        // Replace the nil box with something else and do some work on it
        val = get_different_box();
        do_work(&val);
    }
    // We are done with the box
    amongoc_box_destroy(val);
}
// end.

// ex: [init-box-simple]
void do_work(const amongoc_box*);

void init_int_box() {
    amongoc_box box = amongoc_nil;
    // Create boxed storage for an 'int'
    int* iptr = amongoc_box_init(box, int);
    // Set the int value
    *iptr = 42;
    // Pass the box along
    do_work(&box);
    // Clean up
    amongoc_box_destroy(box);
}
// end.

// ex: [init-box-aggregate]
void do_work(const amongoc_box*);

struct large {
    int many_integers[256];
};

void init_large(struct large* l);

void boxed_custom_type() {
    amongoc_box   box;
    struct large* l = amongoc_box_init(box, struct large);
    init_large(l);
    do_work(&box);
    amongoc_box_destroy(box);
}
// end.

// ex: [init-box-dtor]
void takes_box(amongoc_box);

struct user_info {
    char* username;
    char* domain;
    int   uid;
};

void _user_info_box_dtor(void* p) {
    struct user_info* ui = (struct user_info*)p;
    free(ui->username);
    free(ui->domain);
}

void boxed_nontrivial(const char* name, const char* dom, int uid) {
    amongoc_box       ui_box;
    struct user_info* ui = amongoc_box_init(ui_box, struct user_info, &_user_info_box_dtor);
    ui->username         = strdup(name);
    ui->domain           = strdup(dom);
    ui->uid              = uid;
    takes_box(ui_box);
}
// end.

// ex: [box-cast]
void inspect_box(amongoc_box const* ui_box) {
    struct user_info const* ui = &amongoc_box_cast(struct user_info const, *ui_box);
    printf("Username is %s\n", ui->username);
}
// end.

// ex: [box-take]
void take_from_box(amongoc_box ui_box) {
    struct user_info ui;
    amongoc_box_take(ui, ui_box);
    // Do stuff...
    _user_info_box_dtor(&ui);
}
// end.

// ex: [cxx-box]
amongoc_box gets_box();

void cxx_using_box() {
    // `b` will be destroyed automatically
    amongoc::unique_box b = gets_box().as_unique();

    // Get a C-style box
    amongoc_box c_box = gets_box();
    // Move it into a managed box
    auto ubox = std::move(c_box).as_unique();
}
// end.

// ex: [cxx-release-box]
amongoc_box gets_box();

void cxx_release_box() {
    auto ubox = gets_box().as_unique();
    // Do stuff...
    takes_box(std::move(ubox).release());
}
// end.

int main() {
    uses_nil_box();
    init_int_box();
    boxed_custom_type();
    boxed_nontrivial("joe", "example.com", 42);
}

void do_work(const amongoc_box*) {}
void init_large(large*) {}
void takes_box(::amongoc_box b) { ::amongoc_box_destroy(b); }

amongoc_box gets_box() { return amongoc_nil; }
