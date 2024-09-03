/**
 * Copyright 2024 MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "./config.h"

#include <neo/pp.hpp>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

mlib_extern_c_begin();

/// Return 'true' iff (left * right) would overflow with int64
mlib_constexpr bool _mlib_i64_mul_would_overflow(int64_t left, int64_t right) {
    if (right == -1) {
        // We will perform an integer division, and only (MIN / -1) is undefined
        // for integer division.
        return left == INT64_MIN;
    }

    if (right == 0) {
        // Multiplication by zero never overflows, and we cannot divide by zero
        return false;
    }

    // From here on, all integer division by 'right' is well-defined

    if (left > 0) {
        if (right > 0) {
            /**
            Given: left > 0
              and: right > 0
             then: left * right > 0
             THEN: left * right > MIN

            Define: max_fac         =  MAX / right
              then: max_fac * right = (MAX / right) * right
              then: max_fac * right =  MAX
            */
            const int64_t max_fac = INT64_MAX / right;
            if (left > max_fac) {
                /**
                Given: left         > max_fac
                 then: left * right > max_fac * right
                 with: MAX          = max_fac * right
                 then: left * right > MAX
                */
                return true;
            } else {
                /**
                Given: left         <= max_fac
                 then: left * right <= max_fac * right
                 with: MAX          = max_fac * right
                 THEN: left * right <= MAX
                */
                return false;
            }
        } else {
            /**
            Given: left > 0
              and: right <= 0
             then: left * right < 0
             THEN: left * right < MAX

            Define: min_fac        =  MIN / left
              then: min_Fac * left = (MIN / left) * left
              then: min_Fac * left =  MIN
            */
            const int64_t min_fac = INT64_MIN / left;
            if (right < min_fac) {
                /**
                Given: right          < min_fac
                 then: right   * left < min_fac * left
                 with: min_fac * left = MIN
                 then: right   * left < MIN
                */
                return true;
            } else {
                /**
                Given: right          >= min_fac
                 then: right   * left >= min_fac * left
                 with: min_fac * left =  MIN
                 then: right   * left >= MIN
                */
                return false;
            }
        }
    } else {
        if (right > 0) {
            /**
            Given: left <= 0
              and: right > 0
             then: left * right <= 0
             THEN: left * right <  MAX

            Define: min_fac         =  MIN / right
              then: min_fac * right = (MIN / right) * right
              then: min_fac * right =  MIN
            */
            const int64_t min_fac = INT64_MIN / right;
            if (left < min_fac) {
                /**
                Given: left         < min_fac
                 then: left * right < min_fac * right
                 with: MIN          = min_fac * right
                 then: left * right < MIN
                */
                return true;
            } else {
                /**
                Given: left         >= min_fac
                 then: left * right >= min_fac * right
                 with: MIN          =  min_fac * right
                 then: left * right >= MIN
                */
                return false;
            }
        } else {
            /**
            Given: left  <= 0
              and: right <= 0
             then: left * right >= 0
             THEN: left * right >  MIN
            */
            if (left == 0) {
                // Multiplication by zero will never overflow
                return false;
            } else {
                /**
                Given: left <= 0
                  and: left != 0
                 then: left <  0

                Define: max_fac        =  MAX / left
                  then: max_fac * left = (MAX / left) * left
                  then: max_fac * left =  MAX

                Given:   left < 0
                  and:    MAX > 0
                  and: max_fac = MAX / left
                 then: max_fac < 0  [pos/neg -> neg]
                */
                const int64_t max_fac = INT64_MAX / left;
                if (right < max_fac) {
                    /*
                    Given:        right <  max_fac
                      and: left         <  0
                     then: left * right >  max_fac     * left
                     then: left * right > (MAX / left) * left
                     then: left * right >  MAX
                    */
                    return true;
                } else {
                    /*
                    Given:        right >=  max_fac
                      and: left         <   0
                     then: left * right <=  max_fac     * left
                     then: left * right <= (MAX / left) * left
                     then: left * right <=  MAX
                    */
                    return false;
                }
            }
        }
    }
}

/// Return 'true' iff (left + right) would overflow with int64
mlib_constexpr bool _mlib_i64_add_would_overflow(int64_t left, int64_t right) {
    /**
     * Context:
     *
     * - MAX, MIN, left: right: ℤ
     * - left >= MIN
     * - left <= MAX
     * - right >= MIN
     * - right <= MAX
     * - forall (N, M, Q : ℤ) .
     *    if N = M then
     *       M = N  (Symmetry)
     *    N + 0 = N      (Zero is neutral)
     *    N + M = M + N  (Addition is commutative)
     *    if N < M then
     *       0 - N > 0 - M  (Order inversion)
     *           M >     N  (Symmetry inversion)
     *       0 - M < 0 - N  (order+symmetry inversion)
     *       if M < Q or M = Q then
     *          N < Q       (Order transitivity)
     *    0 - M = -M        (Negation is subtraction)
     *    N - M = N + (-M)
     *    Ord(N, M) = Ord(N+Q, M+Q) (Addition preserves ordering)
     */
    // MAX, MIN, left, right: ℤ
    //* Given: right <= MAX
    //* Given: left <= MAX

    if (right < 0) {
        /**
        Given:        right <         0
         then: left + right <  left + 0
         then: left + right <  left
         then: left + right <= left  [Weakening]

        Given: left <= MAX
          and: left + right <= left
         then: left + right <= MAX
         THEN: left + right CANNOT overflow MAX
        */

        /**
        Given:     right >=     MIN
         then: 0 - right <= 0 - MIN
         then:    -right <=    -MIN

        Given:       -right <=       -MIN
         then: MIN + -right <= MIN + -MIN
         then: MIN + -right <= 0
         then: MIN -  right <= 0
         then: MIN -  right <= MAX
         THEN: MIN - right CANNOT overflow MAX

        Given:     right <     0
         then: 0 - right > 0 - 0
         then: 0 - right > 0
         then:    -right > 0

        Given:        -right  >       0
         then: MIN + (-right) > MIN + 0
         then: MIN - right    > MIN + 0
         then: MIN - right    > MIN
         THEN: MIN - right CANNOT overflow MIN

        Define: legroom = MIN - right

        Given: legroom         = MIN - right
         then: legroom + right = MIN - right + right
         then: legroom + right = MIN
        */
        const int64_t legroom = INT64_MIN - right;

        if (left < legroom) {
            /**
            Given: left         < legroom
             then: left + right < legroom + right

            Given: legroom + right = MIN
              and: left + right < legroom + right
             then: left + right < MIN
             THEN: left + right WILL overflow MIN!
            */
            return true;
        } else {
            /**
            Given: left >= legroom
             then: left + right >= legroom + right

            Given: legroom + right = MIN
              and: left + right >= legroom + right
             THEN: left + right >= MIN

            Given: left + right <= MAX
              and: left + right >= MIN
             THEN: left + right is in [MIN, MAX]
            */
            return false;
        }
    } else if (right > 0) {
        /**
        Given:        right >         0
         then: left + right >  left + 0
         then: left + right >  left
         then: left + right >= left  [Weakening]

        Given: left >= MIN
          and: left + right >= left
         then: left + right >= MIN
         THEN: left + right cannot overflow MIN
        */

        /**
        Given:     right <=     MAX
         then: 0 - right >= 0 - MAX
         then:    -right >=    -MAX

        Given:       -right >=       -MAX
         then: MAX + -right >= MAX + -MAX
         then: MAX + -right >= 0
         then: MAX -  right >= 0
         then: MAX -  right >= MIN
         THEN: MAX - right CANNOT overflow MIN

        Given:         right  > 0
         then:   0 -   right  < 0 - 0
         then:        -right  < 0
         then: MAX + (-right) < MAX + 0
         then: MAX + (-right) < MAX
         then: MAX -   right  < MAX
         THEN: MAX - right CANNOT overflow MAX

        Define: headroom = MAX - right

        Given: headroom         = MAX - right;
         then: headroom + right = MAX - right + right
         then: headroom + right = MAX
        */
        int64_t headroom = INT64_MAX - right;

        if (left > headroom) {
            /**
            Given: left         > headroom
             then: left + right > headroom + right

            Given: left + right > headroom + right
              and: headroom + right = MAX
             then: left + right > MAX
             THEN: left + right WILL overflow MAX!
            */
            return true;
        } else {
            /**
            Given: left         <= headroom
             then: left + rigth <= headroom + right

            Given: left + right <= headroom + right
              and: headroom + right = MAX
             then: left + right <= MAX
             THEN: left + right CANNOT overflow MAX
            */
            return false;
        }
    } else {
        /**
        Given:        right =        0
          and: left + right = left + 0
         then: left + right = left

        Given: left <= MAX
          and: left >= MIN
          and: left + right = left
         then: left + right <= MAX
          and: left + right >= MIN
         THEN: left + right is in [MIN, MAX]
        */
        return false;
    }
}

/// Return 'true' iff (left - right) would overflow with int64
mlib_constexpr bool _mlib_i64_sub_would_overflow(int64_t left, int64_t right) {
    // Lemma: N - M = N + (-M), therefore (N - M) is bounded iff (N + -M)
    // is bounded.
    if (right > 0) {
        return _mlib_i64_add_would_overflow(left, -right);
    } else if (right < 0) {
        if (left > 0) {
            return _mlib_i64_add_would_overflow(-left, right);
        } else {
            // Both negative. Subtracting two negatives will never overflow
            return false;
        }
    } else {
        // Given:        right =        0
        //  then: left - right = left - 0
        //  then: left - right = left
        //? THEN: left - right is bounded
        return false;
    }
}

/**
 * @brief Perform safe integer arithmetic.
 *
 * The math is performed in terms of 64-bit integers, encoded by mlib_integer,
 * which also keeps track of any overflows or arithmetic errors during
 * computation. Any operation that overflows will set a flag indicating the
 * overflow, and return a result as if the value would wrap the int64_t range.
 *
 * If an operaion produces a result R from operating on two expressions X and Y,
 * then R will inherit flags from X and Y, in addition to any flags introduced
 * by the operation (i.e. error flags are infectious).
 *
 * Operations that check the bounds of a value will clamp the return value to
 * the requested range in addition to setting a flag if those boundaries are
 * violated.
 *
 * The following subexpressions may be given as arguments. The bracketed name
 * indicates the error flag behavior of the operation:
 *
 *    • add(...)
 *       ◇ Add a set of values
 *       ∅ [mlib_integer_add_overflow]
 *    • sub(...)
 *       ◇ Subtract values (left-associative)
 *       ∅ [mlib_integer_sub_overflow]
 *    • mul(...)
 *       ◇ Multiply values
 *       ∅ [mlib_integer_mul_overflow]
 *    • div(N, D)
 *       ◇ Divide value N by value D
 *       ∅ [mlib_integer_div_overflow | mlib_integer_divzero]
 *    • neg(N)
 *       ◇ Subtract value N from 0
 *       ∅ [mlib_integer_sub_overflow]
 *    • fromInt(X)
 *       ◇ Create a value from an int64_t-convertible C expression.
 *       ∅ (Never errors)
 *    • fromUnsigned(X)
 *       ◇ Create a value from a uint64_t-convertible C expression.
 *       ∅ [mlib_integer_bounds]
 *    • value(x)
 *       ◇ Create a value from an mlib_integer.
 *       ∅ (Inherits error flags from the given mlib_integer)
 *    • checkBounds(Min, Max, V)
 *       ◇ Check that V is AT LEAST Min, and AT MOST Max. Flags from Min and Max
 *         are propagated onto V.
 *       ∅ [mlib_integer_bounds]
 *    • checkNonNegative(V)
 *       ◇ Check that V is not a negative value
 *       ∅ [mlib_integer_bounds]
 *    • checkNonPositive(V)
 *       ◇ Check that V is not a positive value (i.e. N ≤ 0)
 *       ∅ [mlib_integer_bounds]
 *    • checkPositive(V)
 *       ◇ Check that V is a positive value (i.e. N > 0)
 *       ∅ [mlib_integer_bounds]
 *    • checkInt32(V)
 *       ◇ Check that V fits within a 32-bit integer
 *       ∅ [mlib_integer_bounds]
 *    • checkInt16(V)
 *       ◇ Check that V fits within a 16-bit integer
 *       ∅ [mlib_integer_bounds]
 *    • checkInt8(V)
 *       ◇ Check that V fits within an 8-bit integer
 *       ∅ [mlib_integer_bounds]
 *
 * The following compound checks are also available:
 *
 *    • checkNonNegativeInt32
 *    • checkNonNegativeInt16
 *    • checkNonNegativeInt8
 *    • checkPositiveInt32
 *    • checkPositiveInt16
 *    • checkPositiveInt8
 *
 * In addition, integer literals 0, 1, 2, 3, 4, and 5 are valid direct
 * expressions, as well as power of two literals up to 1024. Other integer
 * values must be wrapped in fromInt() or fromUnsigned()
 *
 * The following may also appear as direct subexpressions:
 *
 *    • strlen(CString)
 *       ◇ Equivalent to fromUnsigned(strlen(CString))
 *    • strnlen(CString, MaxLen)
 *       ◇ Similar to strnlen(), but MaxLen must be an mlibMath expression.
 *    • strlen32(CString)
 *       ◇ Equivalent to: checkInt32(strlen(CString))
 *    • strnlen32(CString, MaxLen)
 *       ◇ Equivalent to: checkInt32(strnlen(CString, MaxLen))
 *    • clearFlags(Bits, V)
 *       ◇ Unset the mlib_integer_flags 'Bits' from the value 'V'
 *    • setFlags(Bits, V)
 *       ◇ Set the mlib_integer_flags 'Bits' on the value 'V'
 *    • assertNot(Bits, V)
 *       ◇ Assert that none of mlib_integer_flags 'Bits' are set on 'V'
 *
 * ! **** Casting and try/catch
 *
 * The following "cast" operations may also be used but require a previous utterance
 * of `mlib_math_try();` within an enclosing scope:
 *
 *    • cast(T, V)
 *      ⋄ Cast the expression `V` to type `T`. Does no bounds checking
 *    • castInt64(V)
 *    • castUint64(V)
 *    • castInt32(V)
 *    • castUInt32(V)
 *    • castPositiveInt32(V)
 *    • castPositiveUInt32(V)
 *    • castNonNegativeInt32(V)
 *      ⋄ Casts the expression `V` to the appropriate type, with bounds checking.
 *
 * If any of the above operations result in a bounds check failure OR the operand `V`
 * has non-zero flags, the error information will be stored in the error context
 * that was declared with by `mlib_math_try()`.
 *
 * Within a scope of `mlib_math_try()`, the `mlib_match_catch(E)` macro may be
 * used to conditionally execute an associated (compound) statement if the current `try`
 * scope has an error, which will be stored in `E`.
 */
#define mlibMath(...) MLIB_EVAL_16(_mlibMath(__VA_ARGS__))
#define _mlibMath(Expression) _mlibMath1 MLIB_NOTHING()(Expression)
#define _mlibMath1(Expression) MLIB_PASTE(_mlibMathSubExpr_, Expression)
#define _mlibMathEval(...) _mlibMath MLIB_NOTHING()(__VA_ARGS__)

// Four basic operations:
#define _mlibMathSubExpr_add(A, B) _mlib_math_add(_mlibMathEval(A), _mlibMathEval(B))
#define _mlibMathSubExpr_sub(A, B) _mlib_math_sub(_mlibMathEval(A), _mlibMathEval(B))
#define _mlibMathSubExpr_mul(A, B) _mlib_math_mul(_mlibMathEval(A), _mlibMathEval(B))
#define _mlibMathSubExpr_div(a, b) _mlib_math_div(_mlibMathEval(a), _mlibMathEval(b))
//  Negate the given argument (subtract from zero)
#define _mlibMathSubExpr_neg(n) _mlibMathEval(sub(0, n))

#define _mlibMathSubExpr_fromInt(x) _mlib_math_from_i64(x)
#define _mlibMathSubExpr_I(x) _mlib_math_from_i64(x)
#define _mlibMathSubExpr_fromUnsigned(x) _mlib_math_from_u64(x)
#define _mlibMathSubExpr_U(x) _mlib_math_from_u64(x)
#define _mlibMathSubExpr_value(x) x
#define _mlibMathSubExpr_V(x) x

#define _mlibMathSubExpr_1024 _mlibMathEval MLIB_NOTHING()(fromInt(1024))
#define _mlibMathSubExpr_512 _mlibMathEval MLIB_NOTHING()(fromInt(512))
#define _mlibMathSubExpr_256 _mlibMathEval MLIB_NOTHING()(fromInt(256))
#define _mlibMathSubExpr_128 _mlibMathEval MLIB_NOTHING()(fromInt(128))
#define _mlibMathSubExpr_64 _mlibMathEval MLIB_NOTHING()(fromInt(64))
#define _mlibMathSubExpr_32 _mlibMathEval MLIB_NOTHING()(fromInt(32))
#define _mlibMathSubExpr_16 _mlibMathEval MLIB_NOTHING()(fromInt(16))
#define _mlibMathSubExpr_8 _mlibMathEval MLIB_NOTHING()(fromInt(8))
#define _mlibMathSubExpr_5 _mlibMathEval MLIB_NOTHING()(fromInt(5))
#define _mlibMathSubExpr_4 _mlibMathEval MLIB_NOTHING()(fromInt(4))
#define _mlibMathSubExpr_3 _mlibMathEval MLIB_NOTHING()(fromInt(3))
#define _mlibMathSubExpr_2 _mlibMathEval MLIB_NOTHING()(fromInt(2))
#define _mlibMathSubExpr_1 _mlibMathEval MLIB_NOTHING()(fromInt(1))
#define _mlibMathSubExpr_0 _mlibMathEval MLIB_NOTHING()(fromInt(0))

#define _mlibMathSubExpr_checkBounds(Min, Max, Value)                                              \
    _mlib_math_check_bounds(_mlibMathEval(Min), _mlibMathEval(Max), _mlibMathEval(Value))

#define _mlibMathSubExpr_checkMin(Min, V) _mlibMathEval(checkBounds(Min, fromInt(INT64_MAX), V))
#define _mlibMathSubExpr_checkMax(Max, V) _mlibMathEval(checkBounds(fromInt(INT64_MIN), Max, V))

#define _mlibMathSubExpr_checkInt32(Value)                                                         \
    _mlibMathSubExpr_checkBounds(fromInt(INT32_MIN), fromInt(INT32_MAX), Value)
#define _mlibMathSubExpr_checkNonNegativeInt32(Value)                                              \
    _mlibMathSubExpr_checkBounds(0, fromInt(INT32_MAX), Value)
#define _mlibMathSubExpr_checkPositiveInt32(Value)                                                 \
    _mlibMathSubExpr_checkBounds(1, fromInt(INT32_MAX), Value)

#define _mlibMathSubExpr_checkInt16(Value)                                                         \
    _mlibMathSubExpr_checkBounds(fromInt(INT16_MIN), fromInt(INT16_MAX), Value)
#define _mlibMathSubExpr_checkNonNegativeInt16(Value)                                              \
    _mlibMathSubExpr_checkBounds(0, fromInt(INT64_MAX), Value)
#define _mlibMathSubExpr_checkPositiveInt16(Value)                                                 \
    _mlibMathSubExpr_checkBounds(1, fromInt(INT64_MAX), Value)

#define _mlibMathSubExpr_checkInt8(Value)                                                          \
    _mlibMathSubExpr_checkBounds(fromInt(INT8_MIN), fromInt(INT8_MAX), Value)
#define _mlibMathSubExpr_checkNonNegativeInt8(Value)                                               \
    _mlibMathSubExpr_checkBounds(0, fromInt(INT8_MAX), Value)
#define _mlibMathSubExpr_checkPositiveInt8(Value)                                                  \
    _mlibMathSubExpr_checkBounds(1, fromInt(INT8_MAX), Value)

#define _mlibMathSubExpr_checkNonNegative(Value)                                                   \
    _mlibMathSubExpr_checkBounds(0, fromInt(INT64_MAX), Value)
#define _mlibMathSubExpr_checkNonPositive(Value)                                                   \
    _mlibMathSubExpr_checkBounds(fromInt(INT64_MIN), 0, Value)
#define _mlibMathSubExpr_checkPositive(Value)                                                      \
    _mlibMathSubExpr_checkBounds(1, fromInt(INT64_MAX), Value)

#define _mlibMathSubExpr_strlen(CString) _mlibMathEval(fromUnsigned(strlen(CString)))

#define _mlibMathSubExpr_strlen32(CString) _mlibMathEval(checkInt32(strlen(CString)))

#define _mlibMathSubExpr_strnlen(CString, MaxLen) _mlib_math_strnlen(CString, _mlibMathEval(MaxLen))

#define _mlibMathSubExpr_strnlen32(CString, MaxLen)                                                \
    _mlibMathEval(checkInt32(strnlen(CString, MaxLen)))

#define _mlibMathSubExpr_clearFlags(Bits, V) _mlib_math_clear_flags(Bits, _mlibMathEval(V))

#define _mlibMathSubExpr_assertNot(Bits, V)                                                        \
    _mlib_math_assert_not_flags(Bits,                                                              \
                                NEO_STR(Bits),                                                     \
                                _mlibMathEval(V),                                                  \
                                NEO_STR(V),                                                        \
                                __FILE__,                                                          \
                                __LINE__)

/**
 * @brief Flags corresponding to arithmetic errors during checked arithmetic
 */
enum mlib_integer_flags {
    // Zero value: No errors
    mlib_integer_okay = 0,
    // Overflow during addition
    mlib_integer_add_overflow = 1 << 0,
    // Overflow during subtraction
    mlib_integer_sub_overflow = 1 << 1,
    // Overflow during multiplication
    mlib_integer_mul_overflow = 1 << 2,
    // Overflow during division
    mlib_integer_div_overflow = 1 << 3,
    // All arithmetic overflow bits
    mlib_integer_overflow_bits = mlib_integer_add_overflow | mlib_integer_sub_overflow
        | mlib_integer_mul_overflow | mlib_integer_div_overflow,
    // Integer bounds violation during a narrowing cast
    mlib_integer_bounds = 1 << 4,
    // Attempt to divide by zero
    mlib_integer_zerodiv = 1 << 5,
    // All error bits
    // XXX: Be sure to update this if more bits are added
    mlib_integer_allbits = (1 << 6) - 1,
};

/**
 * @brief A "checked" integer type for integer arithmetic
 *
 * Internally stores a 64-bit integer, as well as a bitmask of status flags for
 * mathematical operations.
 *
 * Operations accumulate flags in the integer between operands.
 */
typedef struct mlib_integer {
    // The current int64_t value of the integer
    int64_t i64;
    // Status flags for the integer value
    enum mlib_integer_flags flags;
} mlib_integer;

// Unset the given status flags on the given integer
mlib_constexpr mlib_integer _mlib_math_clear_flags(mlib_integer v, enum mlib_integer_flags flags) {
    v.flags = (enum mlib_integer_flags)(v.flags & ~flags);
    return v;
}

// Set additional status flags on the given integer (does not clear any bits)
mlib_constexpr mlib_integer _mlib_math_set_flags(mlib_integer v, enum mlib_integer_flags flags) {
    v.flags = (enum mlib_integer_flags)(flags | v.flags);
    return v;
}

mlib_constexpr mlib_integer _mlib_math_from_i64(int64_t val) {
    return (mlib_integer){val, mlib_integer_okay};
}

mlib_constexpr mlib_integer _mlib_math_from_u64(uint64_t val) {
    mlib_integer v = _mlib_math_from_i64((int64_t)val);
    if (val > INT64_MAX) {
        v = _mlib_math_set_flags(v, mlib_integer_bounds);
    }
    return v;
}

mlib_constexpr mlib_integer _mlib_math_add(mlib_integer l, mlib_integer r) {
    l = _mlib_math_set_flags(l, r.flags);
    if (_mlib_i64_add_would_overflow(l.i64, r.i64)) {
        l = _mlib_math_set_flags(l, mlib_integer_add_overflow);
    }
    uint64_t ret = (uint64_t)l.i64 + (uint64_t)r.i64;
    l.i64        = (int64_t)ret;
    return l;
}

mlib_constexpr mlib_integer _mlib_math_sub(mlib_integer l, mlib_integer r) {
    l = _mlib_math_set_flags(l, r.flags);
    if (_mlib_i64_sub_would_overflow(l.i64, r.i64)) {
        l = _mlib_math_set_flags(l, mlib_integer_sub_overflow);
    }
    uint64_t ret = (uint64_t)l.i64 - (uint64_t)r.i64;
    l.i64        = (int64_t)ret;
    return l;
}

mlib_constexpr mlib_integer _mlib_math_mul(mlib_integer l, mlib_integer r) {
    l = _mlib_math_set_flags(l, r.flags);
    if (_mlib_i64_mul_would_overflow(l.i64, r.i64)) {
        l = _mlib_math_set_flags(l, mlib_integer_mul_overflow);
    }
    uint64_t ret = (uint64_t)l.i64 * (uint64_t)r.i64;
    l.i64        = (int64_t)ret;
    return l;
}

mlib_constexpr mlib_integer _mlib_math_check_bounds(mlib_integer min,
                                                    mlib_integer max,
                                                    mlib_integer value) {
    value = _mlib_math_set_flags(value, min.flags);
    value = _mlib_math_set_flags(value, max.flags);
    if (value.i64 < min.i64) {
        value     = _mlib_math_set_flags(value, mlib_integer_bounds);
        value.i64 = min.i64;
    } else if (value.i64 > max.i64) {
        value     = _mlib_math_set_flags(value, mlib_integer_bounds);
        value.i64 = max.i64;
    }
    return value;
}

mlib_constexpr mlib_integer _mlib_math_div(mlib_integer num, mlib_integer den) {
    num = _mlib_math_set_flags(num, den.flags);
    if (den.i64 == 0) {
        num     = _mlib_math_set_flags(num, mlib_integer_zerodiv);
        num.i64 = INT64_MAX;
    } else if (num.i64 == INT64_MIN && den.i64 == -1) {
        num     = _mlib_math_set_flags(num, mlib_integer_div_overflow);
        num.i64 = 0;
    } else {
        num.i64 /= den.i64;
    }
    return num;
}

mlib_constexpr mlib_integer _mlib_math_strnlen(const char* string, mlib_integer maxlen) {
    if (maxlen.flags) {
        // It is not safe to strlen() the string, since 'maxlen' may have a bogus
        // value.
        mlib_integer r = {0, maxlen.flags};
        return r;
    }
    if (maxlen.i64 < 0) {
        mlib_integer r = {0, mlib_integer_bounds};
        return r;
    }
    int64_t ret = 0;
    while (string[ret] && ret < maxlen.i64) {
        ++ret;
    }
    mlib_integer r = {ret, mlib_integer_okay};
    return r;
}

mlib_constexpr mlib_integer _mlib_math_assert_not_flags(enum mlib_integer_flags flags,
                                                        const char*             bits_str,
                                                        mlib_integer            v,
                                                        const char* const       expr_str,
                                                        const char*             file,
                                                        int                     line) {
    if (flags & v.flags) {
        fprintf(stderr,
                "           mlibMath: assertNot FAILED\n"
                "         Location: %s:%d\n"
                "    Subexpression: %s\n"
                "Checked for flags: %s\n"
                "        Has flags: %#x\n",
                file,
                line,
                expr_str,
                bits_str,
                (unsigned)v.flags);
        abort();
    }
    return v;
}

struct mlib_math_fail_info {
    int64_t                 i64;
    enum mlib_integer_flags flags;
    const char*             file;
    int                     line;
};

mlib_constexpr mlib_integer _mlibMathFillFailureInfo(volatile struct mlib_math_fail_info* info,
                                                     mlib_integer                         v,
                                                     const char*                          file,
                                                     int                                  line) {
    if (v.flags) {
        info->i64   = v.i64;
        info->flags = v.flags;
        info->file  = file;
        info->line  = line;
    }
    return v;
}

#define _mlibMathSubExpr_checkOrFail(V)                                                            \
    _mlibMathFillFailureInfo(&_mlibMathScopeErrorInfo, _mlibMathEval(V), __FILE__, __LINE__)

#define _mlibMathSubExpr_cast(Type, V) (Type) _mlibMathEval(checkOrFail(V)).i64
#define _mlibMathSubExpr_castInt64(V) _mlibMathEval(cast(int64_t, V))
#define _mlibMathSubExpr_castUInt64(V) _mlibMathEval(cast(uint64_t, checkNonNegative(V)))
#define _mlibMathSubExpr_castInt32(V) _mlibMathEval(cast(int32_t, checkInt32(V)))
#define _mlibMathSubExpr_castUInt32(V) _mlibMathEval(cast(uint32_t, checkNonNegativeInt32(V)))
#define _mlibMathSubExpr_castPositiveInt32(V) _mlibMathEval(castInt32(checkPositiveInt32(V)))
#define _mlibMathSubExpr_castPositiveUInt32(V) _mlibMathEval(castUInt32(checkPositive(V)))
#define _mlibMathSubExpr_castNonNegativeInt32(V) _mlibMathEval(castInt32(checkNonNegative(V)))

#define mlibMathInt64(X) mlibMath(castInt64(X))
#define mlibMathPositiveInt64(X) mlibMath(castInt64(checkPositive(V)))
#define mlibMathNonNegativeInt64(X) mlibMath(castInt64(checkNonNegative(V)))
#define mlibMathUInt64(X) mlibMath(castUInt64(X))
#define mlibMathPositiveUInt64(X) mlibMath(castUInt64(checkPositive(V)))

#define mlibMathInt32(X) mlibMath(castInt32(X))
#define mlibMathPositiveInt32(X) mlibMath(castPositiveInt32(X))
#define mlibMathNonNegativeInt32(X) mlibMath(castNonNegativeInt32(X))
#define mlibMathUInt32(X) mlibMath(castUInt32(X))
#define mlibMathPositiveUInt32(X) mlibMath(castPositiveUInt32(X))

#define mlib_math_try() struct mlib_math_fail_info _mlibMathScopeErrorInfo = {0}

#define mlib_math_catch(E)                                                                         \
    if (!_mlibMathScopeErrorInfo.flags) {                                                          \
    } else                                                                                         \
        for (int once = 1; once; once = 0)                                                         \
            for (struct mlib_math_fail_info E = _mlibMathScopeErrorInfo; once; once = 0)

mlib_extern_c_end();
