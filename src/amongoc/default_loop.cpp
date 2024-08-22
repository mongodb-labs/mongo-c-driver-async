#include "./pool.hpp"

#include <amongoc/box.h>
#include <amongoc/default_loop.h>
#include <amongoc/handler.h>
#include <amongoc/loop.h>
#include <amongoc/status.h>

#include <asio/bind_cancellation_slot.hpp>
#include <asio/cancellation_signal.hpp>
#include <asio/cancellation_type.hpp>
#include <asio/connect.hpp>
#include <asio/consign.hpp>
#include <asio/deferred.hpp>
#include <asio/error_code.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/steady_timer.hpp>
#include <asio/system_error.hpp>
#include <asio/write.hpp>
#include <neo/fwd.hpp>

#include <chrono>
#include <cstddef>
#include <forward_list>
#include <list>
#include <memory>
#include <ratio>
#include <set>

using namespace amongoc;

using asio::ip::tcp;
using tcp_resolve_results = tcp::resolver::results_type;

template <>
constexpr bool amongoc::enable_trivially_relocatable_v<tcp::socket> = true;
template <>
constexpr bool amongoc::enable_trivially_relocatable_v<tcp_resolve_results> = true;

template <typename T>
using pool = object_pool<T, cxx_allocator<T>>;

namespace {

// Takes an arbitrary value and wraps it in an amongoc_box
auto as_box = [](cxx_allocator<> alloc) {
    return [alloc](auto&& x) { return unique_box::from(alloc, AM_FWD(x)); };
};

/**
 * @brief Implement a pool of Asio cancellation signals, allowing the objects to
 * be reused between Asio operations, reducing the frequency of memory allocations
 */
struct cancellation_pool {
    // We spliec to/from a forward_list
    std::forward_list<asio::cancellation_signal> _entries;
    int                                          count = 0;

    // A single cancellation signal object
    struct entry {
        cancellation_pool*                           owner;
        std::forward_list<asio::cancellation_signal> _one;

        entry(cancellation_pool& owner, std::forward_list<asio::cancellation_signal> il) noexcept
            : owner(&owner)
            , _one(NEO_MOVE(il)) {}

        entry(entry&& o) noexcept
            : owner(o.owner)
            , _one(NEO_MOVE(o._one)) {
            o.owner = nullptr;
        }

        ~entry() {
            if (owner and owner->count < 50) {
                owner->_entries.splice_after(owner->_entries.before_begin(),
                                             _one,
                                             _one.cbefore_begin());
                ++owner->count;
            }
        }

        asio::cancellation_signal* get() noexcept { return &_one.front(); }
    };

    entry checkout() {
        std::forward_list<asio::cancellation_signal> ret;
        if (_entries.empty()) {
            ret.emplace_front();
        } else {
            ret.splice_after(ret.before_begin(), _entries, _entries.cbefore_begin());
            count--;
        }
        return entry{*this, NEO_MOVE(ret)};
    }
};

using cancellation_ticket = pool<asio::cancellation_signal>::ticket;

/**
 * @brief Adapt an amongoc_handler to an Asio completion handler that exposes
 * the cancellation for the amongoc_handler.
 *
 * The adaptor takes an amongoc_handler object and a transformer that must convert the
 * Asio completion value to a unique_box that will be forwarded to the handler
 *
 * @tparam Transform A function object that transforms the Asio result to a unique_box
 */
template <typename Transform>
class adapt_handler {
public:
    explicit adapt_handler(cxx_allocator<>       alloc,
                           unique_handler&&      h,
                           Transform&&           tr,
                           cancellation_ticket&& sig)
        : _alloc(alloc)
        , _handler(NEO_FWD(h))
        , _transform(NEO_FWD(tr))
        , _signal(NEO_MOVE(sig)) {
        if (_handler.stop_possible()) {
            // Connect the amongoc_handler's stopper and the Asio cancellation signal
            _stop_cookie = _handler.register_stop(_signal.get(), [](void* s) {
                static_cast<asio::cancellation_signal*>(s)->emit(asio::cancellation_type::all);
            });
        }
    }

    // Handler for Asio operations that complete with an error code and no value
    void operator()(asio::error_code ec) {
        _handler.complete(status::from(ec), std::move(_transform)(neo::unit{}));
    }

    // Handler for Asio operations that complete with a value and an error code (most operations)
    void operator()(asio::error_code ec, auto&& res) {
        _handler.complete(status::from(ec), std::move(_transform)(AM_FWD(res)));
    }

    // Expose the cancellation slot to asio::associated_cancellation_slot
    using cancellation_slot_type = asio::cancellation_slot;
    asio::cancellation_slot get_cancellation_slot() const noexcept { return _slot; }

    // Expose the memory allocator of the loop to Asio
    using allocator_type = cxx_allocator<>;
    allocator_type get_allocator() const noexcept { return _alloc; }

private:
    cxx_allocator<> _alloc;
    // The adapted handler
    unique_handler _handler;
    // The transformer
    Transform _transform;
    // The cancellation signal for the operation. This object will return the cancellation signal
    // upon destruction
    cancellation_ticket _signal;
    // Cancellation slot, for Asio to use
    asio::cancellation_slot _slot = _signal->slot();

    // The cookie for the registration of our stop callback.
    unique_box _stop_cookie = amongoc_nil.as_unique();
};

template <typename Tr>
explicit adapt_handler(unique_handler, Tr&&, cancellation_ticket&&) -> adapt_handler<Tr>;

// Implementation of the default event loop, based on asio::io_context
struct default_loop {
    cxx_allocator<> alloc;

    asio::io_context ioc;

    pool<asio::cancellation_signal> _cancel_signals{alloc};
    pool<tcp::resolver>             _resolvers{alloc};
    pool<asio::steady_timer>        _timers{alloc};

    void call_soon(status st, box res, amongoc_handler h) {
        asio::post(ioc, [st, res = AM_FWD(res).as_unique(), h = AM_FWD(h).as_unique()] mutable {
            h.complete(st, AM_FWD(res));
        });
    }

    void call_later(std::int64_t dur_us, box value_, amongoc_handler handler) {
        auto uh    = AM_FWD(handler).as_unique();
        auto value = AM_FWD(value_).as_unique();
        auto timer = _timers.checkout([this] { return asio::steady_timer{ioc}; });
        try {
            timer->expires_after(std::chrono::microseconds(dur_us));
        } catch (const asio::system_error& exc) {
            call_soon(status::from(exc.code()), amongoc_nil, handler);
            return;
        }
        auto go = timer->async_wait(asio::deferred);
        std::move(go)(asio::consign(adapt_handler(alloc,
                                                  AM_FWD(uh),
                                                  konst(AM_FWD(value)),
                                                  _cancel_signals.checkout()),
                                    NEO_MOVE(timer)));
    }

    void getaddrinfo(const char* name, const char* svc, amongoc_handler hnd) {
        auto uh  = AM_FWD(hnd).as_unique();
        auto res = _resolvers.checkout([&] { return tcp::resolver{ioc}; });
        auto go  = res->async_resolve(name, svc, asio::deferred);
        std::move(go)(asio::consign(adapt_handler(alloc,
                                                  AM_FWD(uh),
                                                  as_box(alloc),
                                                  _cancel_signals.checkout()),
                                    NEO_MOVE(res)));
    }

    void tcp_connect(amongoc_view ai, amongoc_handler on_connect) {
        auto uh   = AM_FWD(on_connect).as_unique();
        auto sock = std::make_unique<tcp::socket>(ioc);
        auto go   = asio::async_connect(*sock, ai.as<tcp_resolve_results>(), asio::deferred);
        std::move(go)(adapt_handler(
            alloc,
            AM_FWD(uh),
            [sock = NEO_MOVE(sock), this](asio::ip::tcp::endpoint) {
                // Discard the endpoint and return the connected socket
                return unique_box::from(alloc, NEO_MOVE(*sock));
            },
            _cancel_signals.checkout()));
    }

    void tcp_write_some(amongoc_view    sock,
                        const char*     data,
                        std::size_t     maxlen,
                        amongoc_handler on_write) {
        auto uh = AM_FWD(on_write).as_unique();
        sock.as<tcp::socket>().async_write_some(asio::buffer(data, maxlen),
                                                adapt_handler(alloc,
                                                              AM_FWD(uh),
                                                              as_box(alloc),
                                                              _cancel_signals.checkout()));
    }

    void tcp_read_some(amongoc_view sock, char* data, std::size_t maxlen, amongoc_handler on_read) {
        auto uh = AM_FWD(on_read).as_unique();
        sock.as<tcp::socket>().async_read_some(asio::buffer(data, maxlen),
                                               adapt_handler(alloc,
                                                             AM_FWD(uh),
                                                             as_box(alloc),
                                                             _cancel_signals.checkout()));
    }
};

}  // namespace

template <auto Mfun, typename D = decltype(Mfun)>
struct adapt_memfun_x {};

template <auto F, typename... Args>
struct adapt_memfun_x<F, void (default_loop::*)(Args...)> {
    // A free function that takes a type-erased default_loop and invokes the bound member function.
    // Presents a signature based on the signature of the target member function
    static void ap(amongoc_loop* self, Args... args) noexcept {
        return (amongoc_box_cast(default_loop)(self->userdata).*F)(AM_FWD(args)...);
    }
};

// Create a non-member function for default_loop based on a member function for default_loop
template <auto F>
constexpr auto adapt_memfun = &adapt_memfun_x<F>::ap;

// The vtable for the default event loop
static constexpr amongoc_loop_vtable default_loop_vtable = {
    .call_soon      = adapt_memfun<&default_loop::call_soon>,
    .call_later     = adapt_memfun<&default_loop::call_later>,
    .getaddrinfo    = adapt_memfun<&default_loop::getaddrinfo>,
    .tcp_connect    = adapt_memfun<&default_loop::tcp_connect>,
    .tcp_write_some = adapt_memfun<&default_loop::tcp_write_some>,
    .tcp_read_some  = adapt_memfun<&default_loop::tcp_read_some>,
    .get_allocator  = [](const amongoc_loop* l) -> amongoc_allocator {
        return l->userdata.view.as<default_loop>().alloc.c_allocator();
    },
};

void amongoc_default_loop_init_with_allocator(amongoc_loop* loop, amongoc_allocator alloc) {
    loop->userdata
        = unique_box::make<default_loop>(cxx_allocator<>{alloc}, cxx_allocator<>{alloc}).release();
    loop->vtable = &default_loop_vtable;
}

void amongoc_default_loop_run(amongoc_loop* loop) {
    auto& ioc = amongoc_box_cast(default_loop)(loop->userdata).ioc;
    // Restart the IO context, allowing us to call run() more than once
    ioc.restart();
    ioc.run();
}

void amongoc_default_loop_destroy(amongoc_loop* loop) { amongoc_box_destroy(loop->userdata); }
