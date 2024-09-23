#pragma once

#include <amongoc/coroutine.hpp>
#include <amongoc/loop.h>
#include <amongoc/loop.hpp>
#include <amongoc/string.hpp>
#include <amongoc/uri.hpp>
#include <amongoc/wire/client.hpp>
#include <amongoc/wire/proto.hpp>

#include <mlib/alloc.h>

#include <forward_list>

namespace amongoc {

/**
 * @brief Provides a connection pool that automatically spawns and establishes
 * connections as they are needed
 */
class connection_pool {
public:
    /**
     * @brief Construct a new connection pool attached to the given event loop for the given
     * endpoint
     *
     * @param loop The event loop that will own the spawned connections
     * @param uri The URI for the target endpoint
     */
    explicit connection_pool(amongoc_loop& loop, connection_uri uri);
    ~connection_pool();

    // Get the event loop associated with the object
    amongoc_loop& loop() const noexcept;

    // Get the allocator associated with the pool
    mlib::allocator<> get_allocator() const noexcept { return loop().get_allocator(); }

private:
    // Private implementation of pool members
    struct member_impl;
    // Private implementation of the pool data
    struct pool_impl;

    // A forward-list of member implementations. Used for fast splicing under lock
    using member_impl_list = std::forward_list<member_impl, allocator<member_impl>>;

public:
    /**
     * @brief The member of a connection pool.
     *
     * Implements `wire::client_interface`
     */
    class member {
    public:
        // Prevent copying
        member(const member&) = delete;
        // Checked out members are move-only
        member(member&&);
        member& operator=(member&&);
        // Destroying a member returns it to the pool
        ~member();

        template <wire::message_type M>
        co_task<wire::any_message> request(M&& m) {
            return _request(*this, _wire_client(), mlib_fwd(m));
        }

        allocator<> get_allocator() const noexcept;

    private:
        friend connection_pool;
        // Construct a new member object that owns the implementation
        explicit member(member_impl_list&& single_impl) noexcept;

        member_impl_list _single_impl;

        wire::client<tcp_connection_rw_stream>& _wire_client() noexcept;

        static co_task<wire::any_message>
        _request(member& self, wire::client_interface auto& cl, wire::message_type auto msg) {
            return cl.request(mlib_fwd(msg));
        }
    };

    co_task<member> checkout();

private:
    // Pointer-to-impl for connection pools
    mlib::unique_ptr<pool_impl> _impl;
};

}  // namespace amongoc
