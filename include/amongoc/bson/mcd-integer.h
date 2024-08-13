/**
 * Copyright 2022 MongoDB, Inc.
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

#ifndef MCD_INTEGER_H_INCLUDED
#define MCD_INTEGER_H_INCLUDED

#include <neo/pp.hpp>

#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/// Return 'true' iff (left * right) would overflow with int64
inline bool _mcd_i64_mul_would_overflow(int64_t left, int64_t right) {
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
inline bool _mcd_i64_add_would_overflow(int64_t left, int64_t right) {
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
inline bool _mcd_i64_sub_would_overflow(int64_t left, int64_t right) {
    // Lemma: N - M = N + (-M), therefore (N - M) is bounded iff (N + -M)
    // is bounded.
    if (right > 0) {
        return _mcd_i64_add_would_overflow(left, -right);
    } else if (right < 0) {
        if (left > 0) {
            return _mcd_i64_add_would_overflow(-left, right);
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

#define _mcMathEval16(...) _mcMathEval8(_mcMathEval8(_mcMathEval8(_mcMathEval8(__VA_ARGS__))))
#define _mcMathEval8(...) _mcMathEval4(_mcMathEval4(__VA_ARGS__))
#define _mcMathEval4(...) _mcMathEval2(_mcMathEval2(__VA_ARGS__))
#define _mcMathEval2(...) _mcMathEval1(_mcMathEval1(__VA_ARGS__))
#define _mcMathEval1(...) __VA_ARGS__
#define _mcMathPaste(A, ...) _mcMathPasteImpl(A, __VA_ARGS__)
#define _mcMathPasteImpl(A, ...) A##__VA_ARGS__
#define _mcMathNothing(...)

/**
 * @brief Perform safe integer arithmetic.
 *
 * The math is performed in terms of 64-bit integers, encoded by mcd_integer,
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
 *       ∅ [MC_INTEGER_ADD_OVERFLOW]
 *    • sub(...)
 *       ◇ Subtract values (left-associative)
 *       ∅ [MC_INTEGER_SUB_OVERFLOW]
 *    • mul(...)
 *       ◇ Multiply values
 *       ∅ [MC_INTEGER_MUL_OVERFLOW]
 *    • div(N, D)
 *       ◇ Divide value N by value D
 *       ∅ [MC_INTEGER_DIV_OVERFLOW | MC_INTEGER_DIVZERO]
 *    • neg(N)
 *       ◇ Subtract value N from 0
 *       ∅ [MC_INTEGER_SUB_OVERFLOW]
 *    • fromInt(X)
 *       ◇ Create a value from an int64_t-convertible C expression.
 *       ∅ (Never errors)
 *    • fromUnsigned(X)
 *       ◇ Create a value from a uint64_t-convertible C expression.
 *       ∅ [MC_INTEGER_BOUNDS]
 *    • value(x)
 *       ◇ Create a value from an mcd_integer.
 *       ∅ (Inherits error flags from the given mcd_integer)
 *    • checkBounds(Min, Max, V)
 *       ◇ Check that V is AT LEAST Min, and AT MOST Max. Flags from Min and Max
 *         are propagated onto V.
 *       ∅ [MC_INTEGER_BOUNDS]
 *    • checkNonNegative(V)
 *       ◇ Check that V is not a negative value
 *       ∅ [MC_INTEGER_BOUNDS]
 *    • checkNonPositive(V)
 *       ◇ Check that V is not a positive value (greater than zero)
 *       ∅ [MC_INTEGER_BOUNDS]
 *    • checkInt32(V)
 *       ◇ Check that V fits within a 32-bit integer
 *       ∅ [MC_INTEGER_BOUNDS]
 *    • checkInt16(V)
 *       ◇ Check that V fits within a 16-bit integer
 *       ∅ [MC_INTEGER_BOUNDS]
 *    • checkInt8(V)
 *       ◇ Check that V fits within an 8-bit integer
 *       ∅ [MC_INTEGER_BOUNDS]
 *
 * The following compound checks are also available:
 *
 *    • checkNonNegativeInt32
 *    • checkNonNegativeInt16
 *    • checkNonNegativeInt8
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
 *       ◇ Similar to strnlen(), but MaxLen must be an mcMath expression.
 *    • strlen32(CString)
 *       ◇ Equivalent to: checkInt32(strlen(CString))
 *    • strnlen32(CString, MaxLen)
 *       ◇ Equivalent to: checkInt32(strnlen(CString, MaxLen))
 *    • clearFlags(Bits, V)
 *       ◇ Unset the mc_integer_flags 'Bits' from the value 'V'
 *    • setFlags(Bits, V)
 *       ◇ Set the mc_integer_flags 'Bits' on the value 'V'
 *    • assertNot(Bits, V)
 *       ◇ Assert that none of mc_integer_flags 'Bits' are set on 'V'
 */
#define mcMath(...) _mcMathEval16(_mcMath(__VA_ARGS__))
#define _mcMath(Expression) _mcMath1 _mcMathNothing()(Expression)
#define _mcMath1(Expression) _mcMathPaste(_mcMathSubExpr_, Expression)
#define _mcMathEval(...) _mcMath _mcMathNothing()(__VA_ARGS__)

// clang-format off
/// Expand to an opening call to the given function, leaving parens open:
#define _mcMathCallOpen(Item, Func, _ignore2) \
   Func(_mcMathEval(Item),
/// Expand to a comma, argument, and closing parenthesis:
#define _mcMathCallArgAndCloseParen(_ignore0, _ignore1, _ignore2) \
   )
/// Create a chain of binary function calls for each argument, left-associative.
/// The "Identity" is passed as the left-most argument
#define _mcMathChainBinop(Func, Identity, ...) \
   /* Expand an opening call to _math_add for each argument: */ \
   NEO_MAP _mcMathNothing () \
      (_mcMathCallOpen, Func, __VA_ARGS__) \
   /* Ent with the identity argument: */ \
   Identity \
   /* Closing paren for each argument: */ \
   NEO_MAP _mcMathNothing () \
      (_mcMathCallArgAndCloseParen, ~, __VA_ARGS__)
// Four basic operations:
#define _mcMathSubExpr_add(A, B) _mc_math_add(_mcMathEval(A), _mcMathEval(B))
#define _mcMathSubExpr_sub(A, B) _mc_math_sub(_mcMathEval(A), _mcMathEval(B))
#define _mcMathSubExpr_mul(A, B) _mc_math_mul(_mcMathEval(A), _mcMathEval(B))
#define _mcMathSubExpr_div(a, b) _mc_math_div(_mcMathEval(a), _mcMathEval(b))
//  Negate the given argument (subtract from zero)
#define _mcMathSubExpr_neg(n) _mcMathEval (sub (0, n))
// clang-format on

#define _mcMathSubExpr_fromInt(x) _mc_math_from_i64(x)
#define _mcMathSubExpr_I(x) _mc_math_from_i64(x)
#define _mcMathSubExpr_fromUnsigned(x) _mc_math_from_u64(x)
#define _mcMathSubExpr_U(x) _mc_math_from_u64(x)
#define _mcMathSubExpr_value(x) x
#define _mcMathSubExpr_V(x) x

#define _mcMathSubExpr_1024 _mcMathEval _mcMathNothing()(fromInt(1024))
#define _mcMathSubExpr_512 _mcMathEval _mcMathNothing()(fromInt(512))
#define _mcMathSubExpr_256 _mcMathEval _mcMathNothing()(fromInt(256))
#define _mcMathSubExpr_128 _mcMathEval _mcMathNothing()(fromInt(128))
#define _mcMathSubExpr_64 _mcMathEval _mcMathNothing()(fromInt(64))
#define _mcMathSubExpr_32 _mcMathEval _mcMathNothing()(fromInt(32))
#define _mcMathSubExpr_16 _mcMathEval _mcMathNothing()(fromInt(16))
#define _mcMathSubExpr_8 _mcMathEval _mcMathNothing()(fromInt(8))
#define _mcMathSubExpr_5 _mcMathEval _mcMathNothing()(fromInt(5))
#define _mcMathSubExpr_4 _mcMathEval _mcMathNothing()(fromInt(4))
#define _mcMathSubExpr_3 _mcMathEval _mcMathNothing()(fromInt(3))
#define _mcMathSubExpr_2 _mcMathEval _mcMathNothing()(fromInt(2))
#define _mcMathSubExpr_1 _mcMathEval _mcMathNothing()(fromInt(1))
#define _mcMathSubExpr_0 _mcMathEval _mcMathNothing()(fromInt(0))

#define _mcMathSubExpr_checkBounds(Min, Max, Value)                                                \
    _mc_math_check_bounds(_mcMathEval(Min), _mcMathEval(Max), _mcMathEval(Value))

#define _mcMathSubExpr_checkMin(Min, V) _mcMathEval(checkBounds(Min, fromInt(INT64_MAX), V))
#define _mcMathSubExpr_checkMax(Max, V) _mcMathEval(checkBounds(fromInt(INT64_MIN), Max, V))

#define _mcMathSubExpr_checkInt32(Value)                                                           \
    _mcMathSubExpr_checkBounds(fromInt(INT32_MIN), fromInt(INT32_MAX), Value)
#define _mcMathSubExpr_checkNonNegativeInt32(Value)                                                \
    _mcMathSubExpr_checkBounds(0, fromInt(INT32_MAX), Value)
#define _mcMathSubExpr_checkPositiveInt32(Value)                                                   \
    _mcMathSubExpr_checkBounds(1, fromInt(INT32_MAX), Value)

#define _mcMathSubExpr_checkInt16(Value)                                                           \
    _mcMathSubExpr_checkBounds(fromInt(INT16_MIN), fromInt(INT16_MAX), Value)
#define _mcMathSubExpr_checkNonNegativeInt16(Value)                                                \
    _mcMathSubExpr_checkBounds(0, fromInt(INT64_MAX), Value)
#define _mcMathSubExpr_checkPositiveInt16(Value)                                                   \
    _mcMathSubExpr_checkBounds(1, fromInt(INT64_MAX), Value)

#define _mcMathSubExpr_checkInt8(Value)                                                            \
    _mcMathSubExpr_checkBounds(fromInt(INT8_MIN), fromInt(INT8_MAX), Value)
#define _mcMathSubExpr_checkNonNegativeInt8(Value)                                                 \
    _mcMathSubExpr_checkBounds(0, fromInt(INT8_MAX), Value)
#define _mcMathSubExpr_checkPositiveInt8(Value)                                                    \
    _mcMathSubExpr_checkBounds(1, fromInt(INT8_MAX), Value)

#define _mcMathSubExpr_checkNonNegative(Value)                                                     \
    _mcMathSubExpr_checkBounds(0, fromInt(INT64_MAX), Value)
#define _mcMathSubExpr_checkNonPositive(Value)                                                     \
    _mcMathSubExpr_checkBounds(fromInt(INT64_MIN), 0, Value)
#define _mcMathSubExpr_checkPositive(Value) _mcMathSubExpr_checkBounds(1, fromInt(INT64_MAX), Value)

#define _mcMathSubExpr_strlen(CString) _mcMathEval(fromUnsigned(strlen(CString)))

#define _mcMathSubExpr_strlen32(CString) _mcMathEval(checkInt32(strlen(CString)))

#define _mcMathSubExpr_strnlen(CString, MaxLen) _mc_math_strnlen(CString, _mcMathEval(MaxLen))

#define _mcMathSubExpr_strnlen32(CString, MaxLen) _mcMathEval(checkInt32(strnlen(CString, MaxLen)))

#define _mcMathSubExpr_clearFlags(Bits, V) _mc_math_clear_flags(Bits, _mcMathEval(V))

#define _mcMathSubExpr_assertNot(Bits, V)                                                          \
    _mc_math_assert_not_flags(Bits, NEO_STR(Bits), _mcMathEval(V), NEO_STR(V), __FILE__, __LINE__)

enum mcd_integer_flags {
    MC_INTEGER_OKAY          = 0,
    MC_INTEGER_ADD_OVERFLOW  = 1 << 0,
    MC_INTEGER_SUB_OVERFLOW  = 1 << 1,
    MC_INTEGER_MUL_OVERFLOW  = 1 << 2,
    MC_INTEGER_DIV_OVERFLOW  = 1 << 3,
    MC_INTEGER_OVERFLOW_BITS = MC_INTEGER_ADD_OVERFLOW | MC_INTEGER_SUB_OVERFLOW
        | MC_INTEGER_MUL_OVERFLOW |  //
        MC_INTEGER_DIV_OVERFLOW,
    MC_INTEGER_BOUNDS  = 1 << 4,
    MC_INTEGER_ZERODIV = 1 << 5,
    MC_INTEGER_ALLBITS = 0xff,
};

typedef struct mcd_integer {
    int64_t i64;
    uint8_t flags;
} mcd_integer;

inline mcd_integer _mc_math_make_signed(mcd_integer val) {
    mcd_integer r = {0};
    return r;
}

inline mcd_integer _mc_math_from_i64(int64_t val) { return (mcd_integer){val, 0}; }

inline mcd_integer _mc_math_from_u64(uint64_t val) {
    mcd_integer v = _mc_math_from_i64((int64_t)val);
    if (val > INT64_MAX) {
        v.flags |= MC_INTEGER_BOUNDS;
    }
    return v;
}

inline mcd_integer _mc_math_add(mcd_integer l, mcd_integer r) {
    l.flags |= r.flags;
    if (_mcd_i64_add_would_overflow(l.i64, r.i64)) {
        l.flags |= MC_INTEGER_SUB_OVERFLOW;
    }
    uint64_t ret = (uint64_t)l.i64 + (uint64_t)r.i64;
    l.i64        = (int64_t)ret;
    return l;
}

inline mcd_integer _mc_math_sub(mcd_integer l, mcd_integer r) {
    l.flags |= r.flags;
    if (_mcd_i64_sub_would_overflow(l.i64, r.i64)) {
        l.flags |= MC_INTEGER_SUB_OVERFLOW;
    }
    uint64_t ret = (uint64_t)l.i64 - (uint64_t)r.i64;
    l.i64        = (int64_t)ret;
    return l;
}

inline mcd_integer _mc_math_mul(mcd_integer l, mcd_integer r) {
    l.flags |= r.flags;
    if (_mcd_i64_mul_would_overflow(l.i64, r.i64)) {
        l.flags |= MC_INTEGER_MUL_OVERFLOW;
    }
    uint64_t ret = (uint64_t)l.i64 * (uint64_t)r.i64;
    l.i64        = (int64_t)ret;
    return l;
}

inline mcd_integer _mc_math_check_bounds(mcd_integer min, mcd_integer max, mcd_integer value) {
    value.flags |= min.flags | max.flags;
    if (value.i64 < min.i64) {
        value.flags |= MC_INTEGER_BOUNDS;
        value.i64 = min.i64;
    } else if (value.i64 > max.i64) {
        value.flags |= MC_INTEGER_BOUNDS;
        value.i64 = max.i64;
    }
    return value;
}

inline mcd_integer _mc_math_div(mcd_integer num, mcd_integer den) {
    num.flags |= den.flags;
    if (den.i64 == 0) {
        num.flags |= MC_INTEGER_ZERODIV;
        num.i64 = INT64_MAX;
    } else if (num.i64 == INT64_MIN && den.i64 == -1) {
        num.flags |= MC_INTEGER_DIV_OVERFLOW;
        num.i64 = 0;
    } else {
        num.i64 /= den.i64;
    }
    return num;
}

inline mcd_integer _mc_math_strnlen(const char* string, mcd_integer maxlen) {
    if (maxlen.flags) {
        // It is not safe to strlen() the string, since 'maxlen' may have a bogus
        // value.
        mcd_integer r = {0, maxlen.flags};
        return r;
    }
    if (maxlen.i64 < 0) {
        mcd_integer r = {0, MC_INTEGER_BOUNDS};
        return r;
    }
    int64_t ret = 0;
    while (string[ret] && ret < maxlen.i64) {
        ++ret;
    }
    mcd_integer r = {ret, 0};
    return r;
}

inline mcd_integer _mc_math_clear_flags(enum mcd_integer_flags flags, mcd_integer v) {
    v.flags &= (uint8_t)~flags;
    return v;
}

inline mcd_integer _mc_math_set_flags(enum mcd_integer_flags flags, mcd_integer v) {
    v.flags |= (uint8_t)flags;
    return v;
}

inline mcd_integer _mc_math_assert_not_flags(enum mcd_integer_flags flags,
                                             const char*            bits_str,
                                             mcd_integer            v,
                                             const char* const      expr_str,
                                             const char*            file,
                                             int                    line) {
    if (flags & v.flags) {
        fprintf(stderr,
                "           mcMath: assertNot FAILED\n"
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

struct mc_math_fail_info {
    int64_t     i64;
    uint8_t     flags;
    const char* file;
    int         line;
};

inline mcd_integer _mc_math_check_or_jump(jmp_buf*                           jmp,
                                          volatile struct mc_math_fail_info* info,
                                          mcd_integer                        v,
                                          const char*                        file,
                                          int                                line) {
    if (!v.flags) {
        return v;
    }
    info->i64   = v.i64;
    info->flags = v.flags;
    info->file  = file;
    info->line  = line;
    longjmp(*jmp, 1);
}

inline struct mc_math_fail_info
_mc_math_fail_unvolatile(const volatile struct mc_math_fail_info* f) {
    struct mc_math_fail_info ret = {f->i64, f->flags, f->file, f->line};
    return ret;
}

#define _mcMathSubExpr_checkOrJump(V)                                                              \
    _mc_math_check_or_jump(&_mcMathOnFailJmpBuf,                                                   \
                           &_mcMathFailInfo,                                                       \
                           _mcMathEval(V),                                                         \
                           __FILE__,                                                               \
                           __LINE__)

#define _mcMathSubExpr_cast(Type, V) (Type) _mcMathEval(checkOrJump(V)).i64
#define _mcMathSubExpr_castInt32(V) _mcMathEval(cast(int32_t, checkInt32(V)))
#define _mcMathSubExpr_castUInt32(V) _mcMathEval(cast(uint32_t, checkNonNegativeInt32(V)))
#define _mcMathSubExpr_castPositiveInt32(V) _mcMathEval(castInt32(checkPositiveInt32(V)))
#define _mcMathSubExpr_castPositiveUInt32(V) _mcMathEval(castUInt32(checkPositive(V)))
#define _mcMathSubExpr_castNonNegativeInt32(V) _mcMathEval(castInt32(checkNonNegative(V)))

#define mcMathInt32(X) mcMath(castInt32(X))
#define mcMathUInt32(X) mcMath(castUInt32(X))
#define mcMathPositiveUInt32(X) mcMath(castPositiveUInt32(X))
#define mcMathPositiveInt32(X) mcMath(castPositiveInt32(X))
#define mcMathNonNegativeInt32(X) mcMath(castNonNegativeInt32(X))

#define mcMathOnFail(VarName)                                                                      \
    jmp_buf                           _mcMathOnFailJmpBuf = {0};                                   \
    volatile struct mc_math_fail_info _mcMathFailInfo     = {0};                                   \
    if (setjmp(_mcMathOnFailJmpBuf) == 0) {                                                        \
        /* Do nothing */                                                                           \
    } else                                                                                         \
        for (bool once = true; once; once = false)                                                 \
            for (const struct mc_math_fail_info VarName                                            \
                 = _mc_math_fail_unvolatile(&_mcMathFailInfo);                                     \
                 once;                                                                             \
                 once = false)

#endif  // MCD_INTEGER_H_INCLUDED
