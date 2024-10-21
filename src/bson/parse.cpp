#include <bson/parse.hpp>

namespace bson::parse {

template std::back_insert_iterator<std::string> write_str(std::back_insert_iterator<std::string>,
                                                          std::string_view);

}  // namespace bson::parse
