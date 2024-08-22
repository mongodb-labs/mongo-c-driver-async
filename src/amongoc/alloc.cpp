#include <amongoc/alloc.h>

#include <cinttypes>
#include <cstdio>
#include <cstdlib>

// Implementation of reallocate() for the default allocator
static inline void* default_reallocate(void*,
                                       void*   ptr,
                                       size_t  req_size,
                                       size_t  prev_size,
                                       size_t* out_new_size) noexcept {
    if (req_size == 0) {
        std::free(ptr);
        *out_new_size = 0;
        return nullptr;
    } else {
        auto p = std::realloc(ptr, req_size);
        if (p) {
            *out_new_size = req_size;
        }
        return p;
    }
}

constexpr amongoc_allocator amongoc_default_allocator = {nullptr, default_reallocate};

// Definition of the terminating allocator
constexpr amongoc_allocator amongoc_terminating_allocator = {
    nullptr,
    [](void*, void* prev_ptr, size_t req_size, size_t, size_t*) noexcept -> void* {
        if (req_size == 0 and prev_ptr == nullptr) {
            // Freeing a null pointer is always no-op
            return nullptr;
        }
        // This allocation is expected to never be used. Terminate the program now.
        std::fprintf(
            stderr,
            "FATAL: An operation attempted to allocate using the amongoc_terminating_allocator!\n");
        std::fprintf(stderr, "       Requested allocation size: %zu\n", req_size);
        std::abort();
    },
};