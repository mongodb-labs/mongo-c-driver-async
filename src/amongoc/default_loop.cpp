#include "./pool.hpp"

#include <amongoc/box.h>
#include <amongoc/default_loop.h>
#include <amongoc/handler.h>
#include <amongoc/loop.h>
#include <amongoc/status.h>

#include <mlib/algorithm.hpp>

#include <asio/bind_cancellation_slot.hpp>
#include <asio/buffer.hpp>
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
#include <boost/container/static_vector.hpp>
#include <neo/iterator_facade.hpp>

#include <chrono>
#include <cstddef>
#include <forward_list>
#include <list>
#include <memory>
#include <new>
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
using pool = object_pool<T, allocator<T>>;

namespace {

// Takes an arbitrary value and wraps it in an amongoc_box
auto as_box = [](allocator<> alloc) {
    return [alloc](auto&& x) { return unique_box::from(alloc, mlib_fwd(x)); };
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
    explicit adapt_handler(unique_handler&& h, Transform&& tr, cancellation_ticket&& sig)
        : _handler(mlib_fwd(h))
        , _transform(mlib_fwd(tr))
        , _signal(std::move(sig)) {
        if (_handler.stop_possible()) {
            // Connect the amongoc_handler's stopper and the Asio cancellation signal
            _stop_cookie = _handler.register_stop(_signal.get(), [](void* s) {
                static_cast<asio::cancellation_signal*>(s)->emit(asio::cancellation_type::all);
            });
        }
    }

    // Handler for Asio operations that complete with an error code and no value
    void operator()(asio::error_code ec) {
        _handler.complete(status::from(ec), std::move(_transform)(mlib::unit{}));
    }

    // Handler for Asio operations that complete with a value and an error code (most operations)
    void operator()(asio::error_code ec, auto&& res) {
        _handler.complete(status::from(ec), std::move(_transform)(mlib_fwd(res)));
    }

    // Expose the cancellation slot to asio::associated_cancellation_slot
    using cancellation_slot_type = asio::cancellation_slot;
    asio::cancellation_slot get_cancellation_slot() const noexcept { return _slot; }

    // Expose the memory allocator of the loop to Asio
    using allocator_type = allocator<>;
    allocator_type get_allocator() const noexcept { return _handler.get_allocator(); }

private:
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
    allocator<> _alloc;

    asio::io_context ioc;

    pool<asio::cancellation_signal> _cancel_signals{_alloc};
    pool<tcp::resolver>             _resolvers{_alloc};
    pool<asio::steady_timer>        _timers{_alloc};

    // TODO: Define behavior when the below operations fail to allocate memory.

    void call_soon(status st, box res, amongoc_handler h) {
        auto a = h.get_allocator();
        asio::post(ioc,
                   mlib::bind_allocator(a,
                                        [st,
                                         res = mlib_fwd(res).as_unique(),
                                         h   = mlib_fwd(h).as_unique()] mutable {
                                            h.complete(st, mlib_fwd(res));
                                        }));
    }

    void call_later(std::timespec dur_ts, box value_, amongoc_handler handler) {
        auto uh    = mlib_fwd(handler).as_unique();
        auto value = mlib_fwd(value_).as_unique();
        auto timer = _timers.checkout([this] { return asio::steady_timer{ioc}; });
        auto dur   = std::chrono::seconds(dur_ts.tv_sec) + std::chrono::nanoseconds(dur_ts.tv_nsec);
        try {
            timer->expires_after(dur);
        } catch (const asio::system_error& exc) {
            call_soon(status::from(exc.code()), amongoc_nil, handler);
            return;
        }
        auto go = timer->async_wait(asio::deferred);
        std::move(go)(asio::consign(adapt_handler(mlib_fwd(uh),
                                                  constant(mlib_fwd(value)),
                                                  _cancel_signals.checkout()),
                                    std::move(timer)));
    }

    void getaddrinfo(const char* name, const char* svc, amongoc_handler hnd) {
        auto uh  = mlib_fwd(hnd).as_unique();
        auto res = _resolvers.checkout([&] { return tcp::resolver{ioc}; });
        auto go  = res->async_resolve(name, svc, asio::deferred);
        auto a   = uh.get_allocator();
        std::move(go)(
            asio::consign(adapt_handler(mlib_fwd(uh), as_box(a), _cancel_signals.checkout()),
                          std::move(res)));
    }

    void tcp_connect(amongoc_view ai, amongoc_handler on_connect) {
        auto uh   = mlib_fwd(on_connect).as_unique();
        auto sock = std::make_unique<tcp::socket>(ioc);
        auto go   = asio::async_connect(*sock, ai.as<tcp_resolve_results>(), asio::deferred);
        auto a    = uh.get_allocator();
        std::move(go)(adapt_handler(
            mlib_fwd(uh),
            [sock = std::move(sock), a](asio::ip::tcp::endpoint) {
                // Discard the endpoint and return the connected socket
                return unique_box::from(a, std::move(*sock));
            },
            _cancel_signals.checkout()));
    }

    void tcp_write_some(amongoc_view                  sock,
                        ::amongoc_const_buffer const* bufs,
                        std::size_t                   nbufs,
                        amongoc_handler               on_write) {
        boost::container::static_vector<asio::const_buffer, 16> buf_vec;
        mlib::copy_to_capacity(std::views::transform(std::span(bufs, nbufs), _amc_buf_to_asio_buf),
                               buf_vec);
        auto a  = on_write.get_allocator();
        auto uh = mlib_fwd(on_write).as_unique();
        sock.as<tcp::socket>().async_write_some(buf_vec,
                                                adapt_handler(mlib_fwd(uh),
                                                              as_box(a),
                                                              _cancel_signals.checkout()));
    }

    void tcp_read_some(amongoc_view                    sock,
                       ::amongoc_mutable_buffer const* bufs,
                       std::size_t                     nbufs,
                       amongoc_handler                 on_read) {
        boost::container::static_vector<asio::mutable_buffer, 16> buf_vec;
        mlib::copy_to_capacity(std::views::transform(std::span(bufs, nbufs), _amc_buf_to_asio_buf),
                               buf_vec);
        auto a  = on_read.get_allocator();
        auto uh = mlib_fwd(on_read).as_unique();
        sock.as<tcp::socket>().async_read_some(buf_vec,
                                               adapt_handler(mlib_fwd(uh),
                                                             as_box(a),
                                                             _cancel_signals.checkout()));
    }

private:
    static inline constexpr struct {
        asio::const_buffer operator()(::amongoc_const_buffer cb) const noexcept {
            return asio::const_buffer{cb.buf, cb.len};
        }

        asio::mutable_buffer operator()(::amongoc_mutable_buffer mb) const noexcept {
            return asio::mutable_buffer{mb.buf, mb.len};
        }
    } _amc_buf_to_asio_buf{};
};

}  // namespace

template <auto Mfun, typename D = decltype(Mfun)>
struct adapt_memfun_x {};

template <auto F, typename... Args>
struct adapt_memfun_x<F, void (default_loop::*)(Args...)> {
    // A free function that takes a type-erased default_loop and invokes the bound member function.
    // Presents a signature based on the signature of the target member function
    static void ap(amongoc_loop* self, Args... args) noexcept {
        return (amongoc_box_cast(default_loop)(self->userdata).*F)(mlib_fwd(args)...);
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
    .get_allocator  = [](const amongoc_loop* l) noexcept -> mlib_allocator {
        return l->userdata.view.as<default_loop>()._alloc.c_allocator();
    },
};

amongoc_status amongoc_default_loop_init_with_allocator(amongoc_loop*  loop,
                                                        mlib_allocator alloc) noexcept {
    try {

        loop->userdata
            = unique_box::make<default_loop>(allocator<>{alloc}, allocator<>{alloc}).release();
        loop->vtable = &default_loop_vtable;
        return amongoc_okay;
    } catch (std::bad_alloc) {
        return amongoc_status(&amongoc_generic_category, ENOMEM);
    }
}

void amongoc_default_loop_run(amongoc_loop* loop) noexcept {
    auto& ioc = amongoc_box_cast(default_loop)(loop->userdata).ioc;
    // Restart the IO context, allowing us to call run() more than once
    ioc.restart();
    ioc.run();
}

void amongoc_default_loop_destroy(amongoc_loop* loop) noexcept {
    amongoc_box_destroy(loop->userdata);
}
