#pragma once

#include <catch2/catch_tostring.hpp>

namespace Catch {

template <typename T>
    requires requires(T const obj) {
        obj.has_value();
        *obj;
    }
struct StringMaker<T> {
    static std::string convert(T const& val) {
        if (val.has_value()) {
            return Catch::Detail::stringify(*val);
        } else {
            return "[no value]";
        }
    }
};

}  // namespace Catch
