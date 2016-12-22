#include <stdint.h>
#include "fixed_point.h"

#define POSITIVE 0
#define NEGATIVE 1
#define FRACTION_LEN 14

int32_t set_sign_bit(int32_t fixed, int32_t bit);
int32_t get_sign_bit(int32_t fixed);
int32_t abs_fixed(int32_t x);
int32_t abs_integer(int32_t x);
int32_t flip_sign_fixed(int32_t fixed);

int32_t get_as_fixed(int32_t integer)
{
    int32_t sign_bit = POSITIVE;
    if (integer < 0) {//integer is negative
        sign_bit = NEGATIVE;
    }
    integer = abs_integer(integer);
    integer = integer << FRACTION_LEN;
    //set sign bit to the given value
    integer = set_sign_bit(integer, sign_bit);
    return integer;
}

int32_t round_to_int(int32_t fixed)
{
    int32_t sign_bit = get_sign_bit(fixed);
    //clear sign bit
    fixed = abs_fixed(fixed);
    //truncate fraction
    fixed = fixed >> FRACTION_LEN;
    if (sign_bit == NEGATIVE) {//number is originally negative
        fixed *= -1;
    }
    return fixed;
}

int32_t mul_fixed_point(int32_t fixed1, int32_t fixed2)
{
    /* getsign bits */
    int32_t sign1 = get_sign_bit(fixed1);
    int32_t sign2 = get_sign_bit(fixed2);

    /* deal with positive values */
    fixed1 = abs_fixed(fixed1);
    fixed2 = abs_fixed(fixed2);
    //temporarily hold the result that is shifted
    int64_t product = (int64_t)fixed1 * (int64_t)fixed2;
    //restore its state
    product = (product >> FRACTION_LEN) & 0x0003FFFFFFFFFFFFLL;

    return set_sign_bit((int32_t)product, sign1 ^ sign2);
}

int32_t mul_integer(int32_t fixed, int32_t integer)
{
    //convert to fixed point
    int32_t temp_fixed = get_as_fixed(integer);
    return mul_fixed_point(fixed, temp_fixed);
}

int32_t add_fixed_point(int32_t fixed1, int32_t fixed2)
{
    /* get sign bits */
    int32_t sign1 = get_sign_bit(fixed1);
    int32_t sign2 = get_sign_bit(fixed2);

    /* deal with positive values */
    fixed1 = abs_fixed(fixed1);
    fixed2 = abs_fixed(fixed2);

    //apply signs
    fixed1 = sign1 == NEGATIVE ? (fixed1 * -1) : fixed1;
    fixed2 = sign2 == NEGATIVE ? (fixed2 * -1) : fixed2;

    //add
    int32_t temp = fixed1 + fixed2;

    //convert to fixed point
    int is_negative = temp < 0;
    temp = abs_integer(temp);
    return set_sign_bit(temp, is_negative);
}

int32_t add_integer(int32_t fixed, int32_t integer)
{
    //convert to fixed point
    int32_t temp = get_as_fixed(integer);
    return add_fixed_point(fixed, temp);
}

int32_t sub_fixed_point(int32_t fixed1, int32_t fixed2)
{
    //add negative the second number;
    return add_fixed_point(fixed1, flip_sign_fixed(fixed2));
}

int32_t sub_integer(int32_t fixed, int32_t integer)
{
    //convert to fixed point
    int32_t temp = get_as_fixed(integer);
    return sub_fixed_point(fixed, temp);
}

int32_t div_fixed_point(int32_t fixed1, int32_t fixed2)
{
    /* getsign bits */
    int32_t sign1 = get_sign_bit(fixed1);
    int32_t sign2 = get_sign_bit(fixed2);

    /* deal with positive values */
    fixed1 = abs_fixed(fixed1);
    fixed2 = abs_fixed(fixed2);

    /*
      doubling the fraction part, as it vanished after division.
      it is required to double the precision to int64_t to avoid
      overflow. */
    int64_t large_precision = (int64_t)fixed1 << FRACTION_LEN;

    large_precision /= fixed2;

    return set_sign_bit((int32_t)large_precision, sign1 ^ sign2);
}

int32_t div_integer(int32_t fixed, int32_t integer)
{
    //convert to fixed point
    int32_t temp = get_as_fixed(integer);
    return div_fixed_point(fixed, temp);
}

int32_t set_sign_bit(int32_t fixed, int32_t bit)
{
    //clear sign bit
    fixed = fixed & 0x7FFFFFFF;
    //set sign bit to the required value
    fixed = fixed | (bit << 31);
    return fixed;
}

/* returns the sign bit value */
int32_t get_sign_bit(int32_t fixed)
{
    return (fixed >> 31) != 0;
}

/* absolute value as fixed point */
int32_t abs_fixed(int32_t x)
{
    return (x & 0x7FFFFFFF);
}

/* absolute value as integer */
int32_t abs_integer(int32_t x)
{
    return (x < 0 ? (x * -1) : x);
}

/* flips the sign of a fixed point */
int32_t flip_sign_fixed(int32_t fixed)
{
    int32_t sign = get_sign_bit(fixed);
    return set_sign_bit(fixed, !sign);
}


