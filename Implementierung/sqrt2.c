#include <stdlib.h>

#include "operations.h"
#include "sqrt2.h"

/*
 * Implementation of the polynomial p(n) = 2n - 1
 */
struct bignum p(size_t n)
{
	struct bignum num;
	// Performs 2n by storing the highest bit of n in overflow and then right shifting n by one
	uint8_t overflow = n >> 63;

	bignumInit(&num, n << 1);

	// If there is indeed an overflow a new 32 bit block in num gets created
	if (overflow != 0) {
		num.numbers[num.length] = overflow;
		num.length++;
	}
	// Performs - 1 by calling bignumDec()
	bignumDec(&num);
	return num;
}

/*
 * Recursive definition of P(n1, n2); returns the result, every other bignum used gets freed
 */
struct bignum P(size_t n1, size_t n2)
{
	size_t nm = (n1 + n2) / 2;
	struct bignum res;

	if (n1 == n2 - 1) {
		res = p(n1);
	} else {
		struct bignum num0 = P(n1, nm);
		struct bignum num1 = P(nm, n2);
		res = karazMult(&num0, &num1);

		bignumFree(&num0);
		bignumFree(&num1);
	}
	return res;
}

/*
 * Implementation of the polynomial q(n) = 4n; similar to p(size_t n), just shifts by two and doesn't subtract one
 */
struct bignum q(size_t n)
{
	struct bignum num;
	uint8_t overflow = n >> 62;

	bignumInit(&num, n << 2);

	if (overflow != 0) {
		num.numbers[num.length] = overflow;
		num.length++;
	}
	return num;
}

/*
 * Recursive definition of Q(n1, n2); similar to P(size_t n1, size_t n2), just uses q(size_t n) instead
 */
struct bignum Q(size_t n1, size_t n2)
{
	size_t nm = (n1 + n2) / 2;
	struct bignum res;

	if (n1 == n2 - 1) {
		res = q(n1);
	} else {
		struct bignum num0 = Q(n1, nm);
		struct bignum num1 = Q(nm, n2);
		res = karazMult(&num0, &num1);

		bignumFree(&num0);
		bignumFree(&num1);
	}
	return res;
}

/*
 * Recursive definition of T(n1, n2); also similar to P(size_t n1, size_t n2), but more complex operations have to be performed
 * Every instance of a(n), b(n), A(n1, n2), B(n1, n2) have been omitted since they always evaluate to one
 */
struct bignum T(size_t n1, size_t n2)
{
	size_t nm = (n1 + n2) / 2;
	struct bignum res;

	if (n1 == n2 - 1) {
		res = p(n1);
	} else {
		struct bignum num0;
		struct bignum num1;
		struct bignum num2 = Q(nm, n2);
		struct bignum num3 = T(n1, nm);

		num0 = karazMult(&num2, &num3);

		bignumFree(&num2);
		bignumFree(&num3);

		num2 = P(n1, nm);
		num3 = T(nm, n2);

		num1 = karazMult(&num2, &num3);

		bignumFree(&num2);
		bignumFree(&num3);

		res = bignumAdd(&num0, &num1);

		bignumFree(&num0);
		bignumFree(&num1);
	}
	return res;
}

/*
 * Returns the approximation of sqrt2 with precision of s binary subone places by computing 1 + T(1, n) / Q(1, n)
 */
struct bignum sqrt2(size_t n, size_t s)
{
	struct bignum res;

	if (s == 0) {
		bignumInit(&res, 1);
		return res;
	}

	struct bignum N = T(1, n);
	struct bignum D = Q(1, n);

	res = newtonDiv(&N, &D, s);

	bignumFree(&N);
	bignumFree(&D);

	// NewtonDiv already returns a result cut to the right amount of blocks, so only the unprecise places have to be cut here
	s %= 32;
	if (s != 0) {
		res.numbers[0] = (res.numbers[0] >> (32 - s)) << (32 - s);
	}

	res.numbers = realloc(res.numbers, (res.length + 1) * sizeof(uint32_t));
	res.numbers[res.length] = 1;
	res.length++;

	return res;
}

/*
 * Computes sqrt2 without binary Splitting
 */
struct bignum sqrt2_V1(size_t n, size_t s)
{
	if (s == 0) {
		struct bignum res;
		bignumInit(&res, 1);
		return res;
	}

	size_t cons_blocks = s / 32;
	if (s % 32 != 0) {
		cons_blocks++;
	}

	struct bignum temp0;
	struct bignum temp1;
	struct bignum temp2;
	bignumInit(&temp2, 0);

	for (size_t i = 1; i <= n; i++) {
		bignumInit(&temp1, 1);
		for (size_t k = 1; k <= i; k++) {
			struct bignum N = p(k);
			struct bignum D = q(k);

			temp0 = newtonDiv(&N, &D, s);
			bignumFree(&N);
			N = karazMult(&temp0, &temp1);
			// Cutting to desired precision + 1 block for potential carry
			cutToSize(&N, cons_blocks + 1);
			bignumFree(&temp1);
			bignumFree(&temp0);
			temp1 = N;
			bignumFree(&D);
		}	
		temp0 = bignumAdd(&temp1, &temp2);
		bignumFree(&temp1);
		bignumFree(&temp2);
		temp2 = temp0;
		cutToSize(&temp2, cons_blocks);
	}

	temp2.numbers[temp2.length - 1] = 1;
	return temp2;
}
