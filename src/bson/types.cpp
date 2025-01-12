#include <bson/types.h>

#include <algorithm>  // std::ranges::equal

bool bson_binary_view::operator==(const bson_binary_view& other) const noexcept {
    return (this->subtype == other.subtype)  //
        and std::ranges::equal(this->bytes_span(), other.bytes_span());
}
