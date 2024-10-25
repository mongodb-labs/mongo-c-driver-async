#include "./impl.hpp"

amongoc::wire::checking_pool_client _amongoc_client_impl::checking_wire_client() {
    return amongoc::wire::checking_client(amongoc::pool_client(_pool));
}
