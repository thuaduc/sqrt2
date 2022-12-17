#ifndef OPERATIONS_H
#define OPERATIONS_H


#include <stddef.h>
#include <stdint.h>

/*
 * Representation of big numbers used in computations, subone stores the amount of blocks used to represent subone places
 * Numbers are stored in little-endian format
 */
struct bignum {
	uint32_t *numbers;
	size_t length;
	size_t subone;
};

void bignumInit(struct bignum *num, size_t n);

void bignumFree(struct bignum *num);

void bignumPrint(const struct bignum *num);

void printResultHex(const struct bignum *num, size_t prec);

void cutToSize(struct bignum *num, size_t prec);

void bignumDec(struct bignum *x);

struct bignum rShift(struct bignum *x, size_t n);

struct bignum bignumSub(struct bignum *x, struct bignum *y);

struct bignum bignumAdd(struct bignum *x, struct bignum *y);

struct bignum karazMult(struct bignum *x, struct bignum *y);

struct bignum newtonDiv(struct bignum *x, struct bignum *y, size_t prec);

struct bignum sqrt2(size_t n, size_t s);


#endif
