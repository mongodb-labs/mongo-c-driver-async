#include <bson/iterator.h>
#include <bson/value_ref.h>

#include <algorithm>

using namespace bson;

bool bson_value_ref::operator==(const bson_value_ref& other) const noexcept {
    return this->visit([&]<typename L>(const L& left) -> bool {
        return other.visit([&]<typename R>(const R& right) -> bool {
            if constexpr (std::same_as<L, R>) {
                return left == right;
            } else {
                return false;
            }
        });
    });
}
