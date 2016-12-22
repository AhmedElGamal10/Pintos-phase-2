#ifndef FIXED_POINT_H_INCLUDED
#define FIXED_POINT_H_INCLUDED

#include <stdint.h>

/** returns fixed point representation of an integer */
int32_t get_as_fixed(int32_t integer);

/** truncates fractional part */
int32_t round_to_int(int32_t fixed);

/** multiplies two fixed point numbers */
int32_t mul_fixed_point(int32_t fixed1, int32_t fixed2);

/** multiplies a fixed point by an integer */
int32_t mul_integer(int32_t fixed, int32_t integer);

/** adds two fixed point numbers */
int32_t add_fixed_point(int32_t fixed1, int32_t fixed2);

/** adds fixed point to an integer*/
int32_t add_integer(int32_t fixed, int32_t integer);

/** subtracts two fixed point numbers */
int32_t sub_fixed_point(int32_t fixed1, int32_t fixed2);

/** subtracts an integer from a fixed point*/
int32_t sub_integer(int32_t fixed, int32_t integer);

/** divides a fixed point by another */
int32_t div_fixed_point(int32_t fixed1, int32_t fixed2);

/** divides a fixed point by an integer */
int32_t div_integer(int32_t fixed, int32_t integer);


#endif // FIXED_POINT_H_INCLUDED

