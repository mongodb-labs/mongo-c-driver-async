#include <bson/doc.h>
#include <bson/view.h>
//
#include <bson/iterator.h>

#include <algorithm>

static_assert(std::ranges::forward_range<bson_view>);
static_assert(std::ranges::forward_range<bson_doc>);
static_assert(std::ranges::forward_range<bson_array_view>);

template class std::span<const bson_byte>;

std::span<const bson_byte> bson_view::bytes() const noexcept {
    return {_bson_document_data, this->byte_size()};
}

std::span<const bson_byte> bson_array_view::bytes() const noexcept {
    return bson_view(*this).bytes();
}

bool bson_view::operator==(const bson_view other) const noexcept {
    return std::ranges::equal(bytes(), other.bytes());
}
