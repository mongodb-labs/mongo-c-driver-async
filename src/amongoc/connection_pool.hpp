#pragma once

#include <amongoc/coroutine.hpp>
#include <amongoc/loop.h>
#include <amongoc/loop.hpp>
#include <amongoc/string.hpp>
#include <amongoc/uri.hpp>
#include <amongoc/wire/client.hpp>
#include <amongoc/wire/proto.hpp>

#include <mlib/alloc.h>
#include <mlib/utility.hpp>

#include <forward_list>
#include <system_error>

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

        member_impl*       _impl() noexcept;
        const member_impl* _impl() const noexcept;
        void               _perish() noexcept;

        static co_task<wire::any_message>
        _request(member& self, wire::client_interface auto& cl, wire::message_type auto msg) {
            try {
                co_return co_await cl.request(mlib_fwd(msg));
            } catch (...) {
                self._perish();
                throw;
            }
        }
    };

    co_task<member> checkout();

private:
    // Pointer-to-impl for connection pools
    mlib::unique_ptr<pool_impl> _impl;
};

/**
 * @brief A wire-protocol client that will automatically check out a connection
 * from the given connection pool when it is used.
 *
 * The checkout happens lazily when a request is issues. If the request generates
 * an exception, the error will be re-thrown and the held connection will be discarded.
 * Attempting to use the client again after an error will cause it to check out another
 * connection from the pool.
 */
class pool_client {
public:
    explicit pool_client(connection_pool& pool) noexcept
        : _pool(&pool) {}

    allocator<> get_allocator() const noexcept { return _pool->get_allocator(); }

    co_task<wire::any_message> request(wire::message_type auto&& msg) {
        return _request(*this, mlib_fwd(msg));
    }

private:
    connection_pool* _pool;

    std::optional<connection_pool::member> _pool_member;

    static co_task<wire::any_message> _request(pool_client& self, wire::message_type auto msg) {
        // If an exception is thrown, reset our checked out pool member
        mlib::scope_fail on_exc = [&] { self._pool_member.reset(); };
        // Lazily check out a new connection from the pool if we haven't already
        if (not self._pool_member.has_value()) {
            self._pool_member.emplace(co_await self._pool->checkout());
        }
        // Issue a request on our checked-out connection
        co_return co_await self._pool_member->request(mlib_fwd(msg));
    }
};

}  // namespace amongoc
