#include "./let.hpp"
#include "amongoc/nano/concepts.hpp"
#include "amongoc/nano/just.hpp"

#include <functional>

using namespace amongoc;

static_assert(nanosender<let_t<just<int>, std::function<just<std::string>(int)>>>);
static_assert(multishot_nanosender<let_t<just<int>, std::function<just<std::string>(int)>>>);
static_assert(
    not multishot_nanosender<
        let_t<just<std::unique_ptr<int>>, std::function<just<std::string>(std::unique_ptr<int>)>>>);

static_assert(statically_immediate<let_t<just<int>, std::function<just<std::string>(int)>>>);
