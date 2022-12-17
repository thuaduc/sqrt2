#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "operations.h"

/*
 * Initializes the given bignum with the value n; allocates an additional 32 bit block for further operations
 */
void bignumInit(struct bignum *num, size_t n)
{
	num->numbers = calloc(3, sizeof(uint32_t));
	if (num->numbers == NULL) {
		fprintf(stderr, "Error while allocation memory!");
		exit(EXIT_FAILURE);
	}
	num->numbers[0] = n % 0x100000000;
	num->length = 1;
	if (n > UINT32_MAX) {
		num->numbers[1] = n >> 32;
		num->length++;
	}
	num->subone = 0;
}

/*
 *  Frees up all allocated memory of the given bignum
 */
void bignumFree(struct bignum *num)
{
	free(num->numbers);
}

/*
 *  Prints bignum, meant for debugging
 */
void bignumPrint(const struct bignum *num)
{
	for (int i = num->length - 1; i >= 0; i--) {
		if (num->subone - 1 == (size_t) i) {
			printf(",");
		}
		printf("%08x", num->numbers[i]);
	}
	printf("\n");
}

/*
 * Prints bignum as hexadecimal number up to desired precision
 * Terminates the program immeadiatly if the given precision is higher than the amount of subone places of the number
 */
void printResultHex(const struct bignum *num, size_t prec) 
{
	size_t whole_blocks = prec / 8;
	prec %= 8;

	if (whole_blocks > num->subone || (whole_blocks == num->subone && prec != 0)) {
		fprintf(stderr, "DEBUG: printResult cannot print more precise than the number actually is!");
		exit(EXIT_FAILURE);
	}

	bool comma_set = false;

	for (int i = num->length - 1; i >= (int) num->subone - (int) whole_blocks; i--) {
		// Printing the highest significant places without leading zeroes, if they are not subone
		if ((size_t) i == num->length - 1 && num->subone < num->length) {
			printf("%x", num->numbers[num->length - 1]);
		} else {
			if (num->subone - 1 == (size_t) i) {
				printf(",");
				comma_set = true;
			}
			printf("%08x", num->numbers[i]);
		}
	}
	
	if (prec != 0) {
		if (!comma_set) {
			printf(",");
		}
		printf("%x", num->numbers[num->subone - whole_blocks - 1] >> (32 - prec * 4));
	}
	printf("\n");
}

/*
 * Decreases the given bignum by one, flips to UINT32_MAX if number stored in the bignum is 0
 * Disregards values <1 since it is only meant as an integer operation and will terminate the program if a value <1 is given
 */
void bignumDec(struct bignum *x)
{
	bool done = false;
	if (x->subone > 0) {
		fprintf(stderr, "DEBUG: bignumDec is not meant for non integer values!\n");
		exit(EXIT_FAILURE);
	}

	// Subtracts one until the first block that isn't one is reached
	for (size_t i = 0; i < x->length && !done; i++) {
		if (x->numbers[i] != 0) {
			done = true;
		}
		x->numbers[i]--;
	}
}

/*
 *  Adds two given bignums; returns the result
 *  Removes unnecassary sub one blocks
 */
struct bignum bignumAdd(struct bignum *x, struct bignum *y)
{
	// Stores the bigger length/subone value of the two arguments, used later to make sure that the result can hold the new number
	size_t greater_length = x->length > y->length ? x->length : y->length;
	size_t greater_subone = x->subone > y->subone ? x->subone : y->subone;

	struct bignum res;

	/*
	 * Allocates enough memory for the values even if one block has more values smaller than one + one additional block for potential carry
	 * Will allocate more memory than needed in some cases, but assures that there is enough
	 */
	res.numbers = calloc(greater_length + greater_subone + 1, sizeof(uint32_t));
	if (res.numbers == NULL) {
		fprintf(stderr, "Error while allocation memory!");
		exit(EXIT_FAILURE);
	}
	res.length = 0;


	/* The result of the individual 32 bit block addittions gets stored in the variable sum,
	 * sum has 64 bit in order to be able to detect overflows
	 */
	uint64_t sum;

	// offset is used to count the number of unnecassary sub one blocks
	size_t offset = 0;

	// least_significant indicates if there has been a value that is not zero stored in a sub one block
	bool least_significant = true;
	bool carry = false;

	size_t x_index = 0;
	size_t y_index = 0;

	for (size_t i = 0; x_index < x->length || y_index < y->length; i++) {
		// If the end of one of the numbers is reached and the carry flag is false the other number gets copied into result
		if (x_index >= x->length) {
			sum = (uint64_t) y->numbers[y_index];
			y_index++;
		} else if (y_index >= y->length) {
			sum = (uint64_t) x->numbers[x_index];
			x_index++;
		} else {
			// Makes sure that only blocks of the same significance get added
			if (x->subone > y->subone && x->subone > y->subone + x_index) {
				sum = (uint64_t) x->numbers[x_index];
				x_index++;
			} else if (x->subone < y->subone && y->subone > x->subone + y_index) {
				sum = (uint64_t) y->numbers[y_index];
				y_index++;
			} else {
				sum = (uint64_t) x->numbers[x_index] + (uint64_t) y->numbers[y_index];
				x_index++;
				y_index++;
			}
		}

		if (carry) {
			sum++;
			carry = false;

		}

		if (sum > UINT32_MAX) {
			carry = true;
		}

		// If the sum of two sub one block is zero and no block before was not zero the value of the new block gets omitted
		if (least_significant && i < greater_subone && sum % 0x100000000 == 0) {
			offset++;
			continue;
		} else {
			least_significant = false;
		}

		// The value in sum gets cut down to 32 bits, any overflow is 'stored' in the carry flag
		res.numbers[i - offset] = sum % 0x100000000;
		res.length++;
	}

	// If the carry flag is set after the addition is over an additional block is added and set to one
	if (carry) {
		res.numbers[res.length] = 1;
		res.length++;
	}

	// The number of sub one blocks is equal to that of the addend with more sub one places minus the blocks that are omitted
	res.subone = greater_subone - offset;

	return res;
}

/*
 * Subtracts bignum in y from bignum in x; returns result
 * x should always be greater than y, since creating an underflow for a potentially infinite number is not possible
 */
struct bignum bignumSub(struct bignum *x, struct bignum *y)
{
	// Stores the bigger length/subone value of the two arguments, used later to make sure that the result can hold the new number
	size_t greater_length = x->length > y->length ? x->length : y->length;
	size_t greater_subone = x->subone > y->subone ? x->subone : y->subone;

	struct bignum res;

	/*
	 * Allocates enough memory for the values even if one block has more values smaller than one
	 * Will allocate more memory than needed in some cases, but assures that there is enough
	 */
	res.numbers = calloc(greater_length + greater_subone, sizeof(uint32_t));
	if (res.numbers == NULL) {
		fprintf(stderr, "Error while allocation memory!");
		exit(EXIT_FAILURE);
	}

	// least_significant indicates if there has been a value that is not zero stored in a sub one block
	bool least_significant = true;
	bool carry = false;
	//y_more_prec tracks the carry status if y has more subone blocks than x
	bool y_more_prec = false;

	// offset is used to count the number of unnecassary sub one blocks
	size_t offset = 0;

	uint32_t diff;

	// final_length tracks the highest block that isn't zero after the subtraction; if every block is zero the variable is one to represent the lowest possible length
	size_t final_length = 1;

	size_t x_index = 0;
	size_t y_index = 0;

	for (size_t i = 0; x_index < x->length || y_index < y->length; i++) {
		if (x_index >= x->length) {
			diff = 0 - y->numbers[y_index];
			y_index++;
		} else if (y_index >= y->length) {
			diff = x->numbers[x_index];

			x_index++;
		} else {
			// Makes sure that only blocks of the same significance get subtracted
			if (x->subone > y->subone && x->subone > y->subone + x_index) {
				diff = x->numbers[x_index];
				
				x_index++;
			} else if (x->subone < y->subone && y->subone > x->subone + y_index) {
				y_more_prec = true;

				diff = 0 - y->numbers[y_index];

				y_index++;
			} else {
				diff = x->numbers[x_index] - y->numbers[y_index];

				x_index++;
				y_index++;
			}
		}

		if (carry) {
			diff--;
			carry = false;
		}

		// Carry gets set if there is an underflow or if y_more_prec is set
		if ((x_index > 0 && diff > x->numbers[x_index - 1]) || y_more_prec) {
			carry = true;
			y_more_prec = false;
		}

		// If the difference of two sub one blocks is zero and no block before was not zero the value of the new block gets omitted
		if (least_significant && i < greater_subone && diff % 0x100000000 == 0) {
			offset++;
			continue;
		} else {
			least_significant = false;
		}


		res.numbers[i - offset] = diff;

		// Updates the length everytime a new block that is not zero is written in the result bignum
		if (diff != 0) {
			final_length = i + 1;
		}
	}

	// The length of the result is the same as the highest non zero block
	res.length = final_length > offset ? final_length - offset : 1;
	res.subone = greater_subone - offset;

	return res;
}

/*
 * Shifts the given integer number to the right by n bits; returns the result 
 * rShift is meant to be an integer operation and will terminate the program immeadiatly if x has subone places
 */
struct bignum rShift(struct bignum *x, size_t n) 
{	
	if (x->subone > 0) {
		fprintf(stderr, "DEBUG: rShift is not meant for non integer values!\n");
	}
	
	// Indicates the amount of whole blocks shifted
	size_t blocks_shifted = n/32;
	n %= 32;

	struct bignum res;

	// First sets the amount of subone places to the value of whole blocks shifted
	res.subone = blocks_shifted;
	res.length = 0;

	// Determines if there is an additional subone block needed to hold any values shifted over a block boundry
	if (n != 0 && x->numbers[0] << (32 - n) != 0) {
		res.subone++;
		res.length++;
	}

	// Used to track and ommit blocks that are zero and at the currently least significant position
	size_t offset = 0;
	bool least_significant = true;

	// Determines if there are additional subone blocks needed
	size_t overfill = blocks_shifted > x->length ? blocks_shifted - x->length : 0;
	
	res.numbers = calloc((x->length + overfill + 1), sizeof(uint32_t));
	if (res.numbers == NULL) {
		fprintf(stderr, "Error while allocation memory!");
		exit(EXIT_FAILURE);
	}
		
	uint32_t val = 0;
	
	// Tracks the position of the next block to be set in res
	size_t index = 0;

	// Fills in the additional block of the value shifted over the least significant block boundry if needed
	if (n != 0 && x->numbers[0] << (32 - n) != 0) {
		res.numbers[0] = x->numbers[0] << (32 - n);
		index++;
	}

	for (size_t i = 0; i < x->length; i++) {
		// buffer is equal to the bits lost in a block by shifting left, buffer gets added to the next block to restore the value
		uint32_t buffer = 0;
		if (n != 0) {
			// Checks next block in x for bits shifted over boundry
			if (i < x->length - 1) {
				buffer = x->numbers[i + 1] << (32 - n);
			}

			val = x->numbers[i] >> n;

			val += buffer;
		} else {
			val = x->numbers[i];
		}

		if (val == 0 && index < res.subone && least_significant) {
			offset++;
			continue;
		} else {
			least_significant = false;
		}
		
		res.numbers[index - offset] = val;
		index++;
	}
	// Sets the amount of blocks determined by overfill to zero
	for (size_t i = index; i < x->length + overfill; i++) {
		res.numbers[index - offset] = 0;
	}

	res.subone -= offset;
	res.length += x->length + overfill - offset;

	if (res.subone < res.length && res.numbers[res.length - 1] == 0) {
		res.length--;
	}
	return res;
}

/*
 * Shifts the numbers of the given bignum to the left by n blocks
 */
void lShift32(struct bignum *x, size_t n)
{
	if (x->subone > 0) {
		size_t subone = x->subone;
		x->subone = subone > n ? subone - n : 0;
		n = x->subone > n ? 0 : n - subone;
	}

	x->numbers = realloc(x->numbers, (x->length + n) * sizeof(uint32_t));
	if (x->numbers == NULL) {
		fprintf(stderr, "Error while allocation memory!");
		exit(EXIT_FAILURE);
	}

	if (n > 0) {
		// Shifts blocks to the left
		for (size_t i = x->length; i > 0; i--) {
			x->numbers[n + i - 1] = x->numbers[i - 1];
		}
		// Sets new blocks to zero
		for (size_t i = 0; i < n; i++) {
			x->numbers[i] = 0;
		}
	}
	x->numbers[x->length + n - 1] != 0 ? x->length += n : 1;
}

/*
 * Copies an array x to the y starting at begin up to end(inclusive).
 * Adapts subnum accordingly.
 */
void copy(struct bignum *x, struct bignum *y, int begin, int end)
{
	y->length = 0;
	y->subone = 0;

	y->numbers = calloc(end - begin + 2, sizeof(uint32_t));
	if (y->numbers == NULL) {
		fprintf(stderr, "Error while allocation memory!");
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i <= end - begin; i++) {
		y->numbers[i] = x->numbers[begin + i];
		y->length++;		
	}
}

/*
 * Cuts the bignum down to desired precision in blocks
 */
void cutToSize(struct bignum *num, size_t prec) 
{
	if (num->subone > prec) {
		size_t offset = num->subone - prec;

		for (size_t i = 0; i < num->length - offset; i++) {
			num->numbers[i] = num->numbers[i + offset];
		}

		num->length -= offset;
		num->subone -= offset;
		
	}
}

/*
 * Aligns two number arrays to same length by 0-extending the smaller one.
 * Assures that both arrays are of an even length using 0-extension.
 */
void karazMultPreparation(struct bignum *x, struct bignum *y)
{
	// aligning to the same precision
	int32_t subone_diff = x->subone - y->subone;

	x->subone = 0;
	y->subone = 0;

	// checking if x->subone is greater that y->subone
	// the number with the smaller precision gets shifted left until there is a 'zero extention' to the right end of the other number
    	if (subone_diff > 0) {
        	lShift32(y, subone_diff);
    	} else if (subone_diff < 0) {
        	subone_diff *= -1;
        	lShift32(x, subone_diff);
    	}

	// aligning to the same length
	int32_t offset = x->length - y->length;

	// checking if x->length is greater than y->length
	if (offset > 0) {
		// y->numbers has to be 0-extended
		// allocating memory for the 0's
		y->numbers = (uint32_t *) realloc(y->numbers, (y->length + offset + 1) * sizeof(uint32_t));
		if (y->numbers == NULL) {
			fprintf(stderr, "Error while allocation memory!");
			exit(EXIT_FAILURE);
		}
		while(offset > 0){
			// extending and setting length
			y->numbers[y->length] = 0x00000000;
			y->length++;
			offset--;
		}
	} else if (offset < 0) {
		// x->numbers has to be 0-extended
		// allocating memory for the 0's
		x->numbers = (uint32_t *) realloc(x->numbers, ((int32_t) x->length - offset + 1) * sizeof(uint32_t));
		if (x->numbers == NULL) {
			fprintf(stderr, "Error while allocation memory!");
			exit(EXIT_FAILURE);
		}
		while(offset < 0){
			x->numbers[x->length] = 0x00000000;
			x->length++;
			offset++;
		}
	}

	// aligning to the even length
	if ((x->length % 2) == 1) {
		x->length++;

		// extending and setting length
		x->numbers[x->length - 1] = 0x00000000;

	}
	
	if ((y->length % 2) == 1) {
		y->length++;

		y->numbers[y->length - 1] = 0x00000000;
	}
}

/*
 * Multiplies two bignums with karazuba multiplication
 * Recursively splits the bignums in two until there are only one 32 bit blocks left, that can be multiplied regularly,
 * if one bignum uses more blocks, has greater precision or has an odd number of blocks the bignum gets zero extended until both have an equal and even amount of blocks
 */
struct bignum karazMult(struct bignum *x, struct bignum *y)
{
    // recursion base case: multiplication when both lengths are 1
        if ((x->length == 1) && (y->length == 1)) {
                struct bignum res;

                if (x->numbers[0] == 0 || y->numbers[0] == 0) {
		bignumInit(&res, 0);
                return res;
                }

                uint64_t product = ((uint64_t)x->numbers[0])*((uint64_t)y->numbers[0]);
                bignumInit(&res, product);
                res.subone = x->subone + y->subone;
                return res;
        }

	struct bignum x_cpy, y_cpy;

	copy(x, &x_cpy, 0, x->length - 1);
	copy(y, &y_cpy, 0, y->length - 1);

	x_cpy.subone = x->subone;
	y_cpy.subone = y->subone;

        // calling function to assure, that both bignum structs have the same even length
        karazMultPreparation(&x_cpy, &y_cpy);

        struct bignum x0, x1, y0, y1;

        copy(&x_cpy, &x1, x_cpy.length/2, x_cpy.length - 1);
        copy(&x_cpy, &x0, 0,(x_cpy.length/2) - 1);
        copy(&y_cpy, &y1, y_cpy.length/2, y_cpy.length - 1);
        copy(&y_cpy, &y0, 0, (y_cpy.length/2) - 1);

	bignumFree(&x_cpy);
	bignumFree(&y_cpy);

        struct bignum mx, my;
        mx = bignumAdd(&x1, &x0);
        my = bignumAdd(&y1, &y0);

        // Stores the amount of blocks needed to calculate number * 2^2 by shifting number left
        size_t powerOfTwo = x1.length;
        // Stores the amount of blocks needed to calculate number * 2^2m by shifting number left
        size_t powerOfTwoMulTwo = 2 * x1.length;

	// Used to temporally store the result of the operations in order to retain the pointers of the operands so they can be freed if need be
	struct bignum temp;

        // Multiplying mx*my = (x0 + x1)*(y0*y1) and storing the result in mx
        temp = karazMult(&mx, &my);
	bignumFree(&mx);
	mx = temp;

	// x0 * y0
        temp = karazMult(&y0, &x0);
	bignumFree(&x0);
	x0 = temp;

	// x1 * y1
        temp = karazMult(&x1, &y1);
	bignumFree(&x1);
	x1 = temp;
	
	// (x0 + x1)*(y0*y1) - x0*y0 - x1*y1
        temp = bignumSub(&mx, &x0);
	bignumFree(&mx);
	mx = temp;

        temp = bignumSub(&mx, &x1);
	bignumFree(&mx);
	mx = temp;

	// 2^m * ((x0 + x1)*(y0*y1) - x0*y0 - x1*y1)
        lShift32(&mx, powerOfTwo);
	// x1*y1 * 2^2m
        lShift32(&x1, powerOfTwoMulTwo);

	// x0*y0 + 2^m * ((x0 + x1)*(y0*y1) - x0*y0 - x1*y1)
        temp = bignumAdd(&mx, &x0);
	bignumFree(&mx);
	mx = temp;

	// x0*y0 + 2^m * ((x0 + x1)*(y0*y1) - x0*y0 - x1*y1) + 2^2m x1*y1
        temp = bignumAdd(&mx, &x1);
	bignumFree(&mx);
	mx = temp;

        mx.subone = x->subone > y->subone ? 2 * x->subone : 2 * y->subone;
	if (mx.subone > mx.length) {
		mx.numbers = realloc(mx.numbers, mx.subone * sizeof(uint32_t));
		if (mx.numbers == NULL) {
			fprintf(stderr, "Error while allocation memory!");
			exit(EXIT_FAILURE);
		}

		for (size_t i = mx.length; i < mx.subone; i++) {
			mx.numbers[i] = 0;
		}
		mx.length = mx.subone;
	}

        bignumFree(&x0);
        bignumFree(&x1);
        bignumFree(&y0);
        bignumFree(&y1);
        bignumFree(&my);

        return mx;
}

/*
 * Reduces the given number to 1 >= number >= 0.5 to be used in Newton-Raphson-Division, removes unnecessary sub one blocks, stores result in dest
 * Only meant as an integer operation and will terminate the program immediatly if a value with sub one places is given
 */
size_t reduce(struct bignum *x, struct bignum *dest)
{
	if (x->subone > 0) {
		fprintf(stderr, "DEBUG: reduce is not meant for non integer values!\n");
		exit(EXIT_FAILURE);
	}

	// Sets all blocks in bignum to subone, then shifts left until the most significant sub one bit is set resulting in a number between 0.5 and 1
	// n counts the required left shifts
	size_t n = 0;
	if (!(x->length == 1 && x->numbers[0] == 0))  {
		while ((x->numbers[x->length - 1] << n & 0x80000000) == 0) {
			n++;
		}
		// Offset is the number of unnecessary blocks omitted
		size_t offset = 0;

		dest->numbers = calloc(x->length, sizeof(uint32_t));
		if (dest->numbers == NULL) {
			fprintf(stderr, "Error while allocation memory!");
			exit(EXIT_FAILURE);
		}

		// Buffer is equal to the bits lost in a block by shifting left, buffer gets added to the next block to restore the value
		uint32_t buffer = 0;

		uint32_t val;

		// least_significant indicates if there has been a value that is not zero stored in a sub one block
		bool least_significant = true;

		// Copies the blocks of x in dest adjusted by the required left shift
		for (size_t i = 0; i < x->length; i++) {
			if (n != 0) {
				uint32_t temp = x->numbers[i] >> (32 - n);
				val = x->numbers[i] << n;
				val += buffer;
				buffer = temp;
			} else {
				val = x->numbers[i];
			}

			// Block gets ommitted if it is 0 and least_significant
			if (val == 0 && least_significant) {
				offset++;
				continue;
			} else {
				least_significant = false;
			}

			if (i >= offset) {
				dest->numbers[i - offset] = val;
			}
		}
		dest->length = x->length - offset;
	}

	// n gets set to the number of right shifts needed to equal the operations done by reduce
	n = (x->length - x->subone) * 32 - n;
	dest->subone = dest->length;
	return n;
}

/*
 * Calculates quotient D/N with the precison of prec using Newton-Raphson division
 */
struct bignum newtonDiv(struct bignum *N, struct bignum *D, size_t prec)
{
	// Number of considered blocks needed to assure the required precision
	size_t cons_blocks = prec/32;
	if (prec % 32 != 0) {
		cons_blocks++;
	}
	
	// Reduces the denominator to be between 0.5 and 1 then right shifts the Numerator by the same amount needed for reduce
	struct bignum D_reduced;
	int n = reduce(D, &D_reduced);
	struct bignum N_reduced = rShift(N, n);

	cutToSize(&D_reduced, cons_blocks * 2);
	cutToSize(&N_reduced, cons_blocks * 2);

	// magic0 is an aproximation for 48/17, magic1 for 32/17
	struct bignum magic0;
	magic0.length = D_reduced.subone + 1;
	magic0.subone = magic0.length - 1;

	struct bignum magic1;
	magic1.length = D_reduced.subone + 1;
	magic1.subone = magic1.length - 1;

	magic0.numbers = calloc(magic0.length, sizeof(uint32_t));
	magic1.numbers = calloc(magic1.length, sizeof(uint32_t));
	if (magic0.numbers == NULL || magic1.numbers == NULL) {
		fprintf(stderr, "Error while allocation memory!");
		exit(EXIT_FAILURE);
	}

	// Fills the magic up to the same precision as D_reduced
	for (size_t i = 0; i < magic0.length - 1; i++) {
		magic0.numbers[i] = 0xd2d2d2d2;
		magic1.numbers[i] = 0xe1e1e1e1;
	}
	magic0.numbers[magic0.length - 1] = 0x2;
	magic1.numbers[magic1.length - 1] = 0x1;

	// Stores the aproximation of the reciprocal
	struct bignum x;

	// Initial value for x wich is an aproximation of 48/17 - 32/17 * D_reducedD_reduced
	x = karazMult(&D_reduced, &magic1);
	cutToSize(&x, cons_blocks * 2);
	bignumFree(&magic1);

	magic1 = bignumSub(&magic0, &x);
	bignumFree(&x);
	bignumFree(&magic0);

	x = magic1;

	// Required steps to achieve the required precision
	int steps = ceil(log((prec + 1)/ (log(17)/ log(2))) / log(2));

	struct bignum two;
	bignumInit(&two, 2);

	// x = x * (2 - x * D_reducedD_reduced)
	for (int i = 0; i < steps; i++) {
		struct bignum temp0 = karazMult(&D_reduced, &x);
		cutToSize(&temp0, cons_blocks * 2);
		struct bignum temp1 = bignumSub(&two, &temp0);
		bignumFree(&temp0);

		temp0 = karazMult(&x, &temp1);
		cutToSize(&temp0, cons_blocks * 2);

		bignumFree(&temp1);
		bignumFree(&x);

		x = temp0;

	}
	bignumFree(&two);

	// The actual quotient is then computed with N * x
	struct bignum quotient = karazMult(&N_reduced, &x);
	cutToSize(&quotient, cons_blocks);
	bignumFree(&x);
	bignumFree(&D_reduced);
	bignumFree(&N_reduced);
	return quotient;
}
