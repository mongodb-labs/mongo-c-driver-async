#include <bson/make.hpp>

namespace bson::make {

template struct doc<>;
template struct doc<std::index_sequence<>>;

#define X(_nth, ViewType, _owner, _name, _safename)                                                \
    template void append_value(mutator&, std::string_view, ViewType const&);                       \
    template struct pair<ViewType>;                                                                \
    BSON_TYPE_X_LIST
template struct pair<bson_value_ref>;
template struct pair<std::string_view>;
template void append_value(mutator&, std::string_view, std::string_view const&);
template void append_value(mutator&, std::string_view, bson_value_ref const&);
#undef X

template struct optional_pair<bson_value_ref>;
template struct optional_pair<std::int32_t>;
template struct optional_pair<std::int64_t>;
template struct optional_pair<bool>;
template struct optional_pair<mlib_str_view>;
template struct optional_pair<bson_view>;

}  // namespace bson::make
