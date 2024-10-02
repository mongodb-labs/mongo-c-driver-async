#include <mlib/alloc.h>

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace mlib;

extern constexpr void* mlib_reallocate(mlib_allocator, void*, size_t, size_t, size_t*) noexcept;
extern constexpr void* mlib_allocate(mlib_allocator, size_t) noexcept(true);
extern constexpr void  mlib_deallocate(mlib_allocator alloc, void* p, size_t sz) noexcept(true);

// Implementation of reallocate() for the default allocator
static inline void* default_reallocate(void*,
                                       void*   ptr,
                                       size_t  req_size,
                                       size_t  alignment,
                                       size_t  prev_size,
                                       size_t* out_new_size) noexcept {
    if (req_size == 0) {
        std::free(ptr);
        (out_new_size && (*out_new_size = 0));
        return nullptr;
    } else if (alignment <= alignof(std::max_align_t)) {
        // Object is not over-aligned, so we can defer to realloc() which will
        // use the default max alignment for new objects.
        auto p = std::realloc(ptr, req_size);
        if (p) {
            *out_new_size = req_size;
        }
        return p;
    } else {
        // The requested alignment is greater than the default alignment from realloc()
        if (ptr) {
            // Reallocating an old over-aligned region.
            void* new_ptr;
            if (posix_memalign(&new_ptr, alignment, req_size)) {
                // Failure
                return nullptr;
            }
            // Copy the common prefix into the new memory location
            std::memcpy(new_ptr, ptr, (std::min)(req_size, prev_size));
            // Release the prior region
            std::free(ptr);
            *out_new_size = req_size;
            return new_ptr;
        } else {
            // Allocated a new over-aligned region
            int err = posix_memalign(&ptr, alignment, req_size);
            if (err == 0) {
                *out_new_size = req_size;
            }
            return ptr;
        }
    }
}

static constexpr mlib_allocator_impl mlib_default_allocator_impl = {nullptr, default_reallocate};

// Definition of the terminating allocator
static constexpr mlib_allocator_impl mlib_terminating_allocator_impl = {
    nullptr,
    [](void*, void* prev_ptr, size_t req_size, size_t align, size_t, size_t*) noexcept -> void* {
        if (req_size == 0 and prev_ptr == nullptr) {
            // Freeing a null pointer is always no-op
            return nullptr;
        }
        // This allocation is expected to never be used. Terminate the program now.
        std::fprintf(
            stderr,
            "FATAL: An operation attempted to allocate using the amongoc_terminating_allocator!\n");
        std::fprintf(stderr,
                     "       Requested allocation: %zu bytes, %zu-aligned\n",
                     req_size,
                     align);
        std::abort();
    },
};

constexpr mlib_allocator mlib_default_allocator     = {&mlib_default_allocator_impl};
constexpr mlib_allocator mlib_terminating_allocator = {&mlib_terminating_allocator_impl};
