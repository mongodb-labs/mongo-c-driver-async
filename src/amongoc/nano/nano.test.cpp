#include <amongoc/nano/nano.hpp>

using namespace amongoc;

static_assert(nanoreceiver_of<archetype_nanoreceiver<int>, int>);
static_assert(nanoreceiver_of<archetype_nanoreceiver<int>, double>);
static_assert(nanoreceiver_of<archetype_nanoreceiver<double>, int>);
static_assert(nanosender<archetype_nanosender<int>>);
