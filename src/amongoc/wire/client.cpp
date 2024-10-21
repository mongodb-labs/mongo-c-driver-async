#include "./client.hpp"

namespace amongoc::wire {

template class client<tcp_connection_rw_stream&>;
template co_task<any_message> client<tcp_connection_rw_stream&>::request(one_bson_view_op_msg&&);

}  // namespace amongoc::wire
