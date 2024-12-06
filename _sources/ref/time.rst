##############
Time Utilities
##############

.. header-file:: mlib/time.h

  Contains utilities for working with times and durations


Types
#####

.. struct:: [[zero_initializable]] mlib_duration

  Represent a strongly-typed duration. This is a trivial type. A duration may be
  positive, negative, or zero.

  :zero-initialized: A |zero-initialized| `mlib_duration` represents a zero
    duration, corresponding to no time elapsing.


Functions & Macros
##################

.. function::
  mlib_duration mlib_microseconds(int64_t n)
  mlib_duration mlib_milliseconds(int64_t n)
  mlib_duration mlib_seconds(int64_t n)

  Create an `mlib_duration` representing a duration of the given number of time
  units.

  Passing a too-small/too-large value will result in a duration that is clamped
  to a minimum/maximum value.


.. function::
  int64_t mlib_microseconds_count(mlib_duration d)
  int64_t mlib_milliseconds_count(mlib_duration d)
  int64_t mlib_seconds_count(mlib_duration d)

  Return the count of the number of time units represented by the given
  duration. The count is rounded toward zero for sub-unit durations.

  The returned value may be incorrect if your program is moving near the speed
  of light. Such use cases are unsupported.
