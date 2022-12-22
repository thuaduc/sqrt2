#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <time.h>

#include "sqrt2.h"
#include "operations.h"

const char* usage_msg =
	"Usage: %s [options]	Approximates the square root of 2\n"
	"   or: %s -h	   	Show help message and exit\n"
	"   or: %s --help   	Show help message and exit\n";

const char* help_msg = 
	"Optional arguments:\n"
	"  -V<int>	Defines version of the programm to be run (default: 0)\n"
	"  -B<int>	Gives runtime of the function, additional value <int> defines the number of reruns (default: 10)\n"
	"  -d<int>	Gives <int> numbers of decimal places after comma (default: 5)\n"
	"  -h<int>	Gives <int> number of hexadecimal places after comma (default: 5)\n"
	"  -T<int>	Tests speed of multiplication for number of <int> blocks, number is initialized consecuantly with 0x00000001 and Multiplied with itself, reruns can be set with -B (default size: 5)\n"
	"  --help	 Shows help message (this text) and exit\n"
	"  -h		 Shows help message (this text) and exit\n"
	"Examples:\n"
       	"  ./sqrt2 -B 		Shows 5 hexadecimal places and runtime for 10 reruns\n"
	"  ./sqrt2 -h15	 	Shows 15 hexadecimal places\n"
	"  ./sqrt2 -T100 -B15	Tests speed of multiplication for number of 100 blocks with 15 reruns\n";

void print_usage(const char* progname) 
{
	fprintf(stderr, usage_msg, progname, progname, progname);
}

void print_help(const char* progname) 
{
	print_usage(progname);
	fprintf(stderr, "\n%s", help_msg);
}

struct bignum mainImplementation(size_t n, size_t s)
{
	return sqrt2(n, s);
}

struct bignum secondImplementation(size_t n, size_t s)
{
	return sqrt2_V1(n, s);
}

int main(int argc, char** argv)
{
	// optionals flags	
	bool result_in_hex = true;
	bool test_mult = false;
	size_t number_of_blocks = 5;
	size_t number_of_decimal_places = 5;
	size_t runtime_reruns = 10;
	uint8_t version = 0;

	size_t s;

	const char* progname = argv[0];

	if (argc == 1) {
		print_usage(progname);
		return EXIT_FAILURE;
	}

	static struct option long_options[] = {
		{"help",	  no_argument,	   0,  'h' }
	};

	int opt;
	int long_index = 0;

	bool benchmarking = false;

	while ((opt = getopt_long(argc, argv, "V:B::d::h::T::", long_options, &long_index)) != -1){
		switch(opt){
			case 'h':
				if (optarg == 0){
					print_help(progname);
					return EXIT_SUCCESS;
				} else {
					result_in_hex = true;
					number_of_decimal_places = strtol(optarg, NULL, 10);
					if (number_of_decimal_places > 1000000) {
						printf("Desired amount of decimal places is too great!\nStay within 0 and 1000000 (both inclusive).\n");
						return EXIT_FAILURE;
					}
					break;
				}
			case 'd':
				result_in_hex = false;
				number_of_decimal_places = optarg ? strtol(optarg, NULL, 10) : 5;
				break;
			case 'V':
				version = strtol(optarg, NULL, 10);
				break;
			case 'B':
				if (optarg == 0) {
					benchmarking = true;
					break;
				} else {
					runtime_reruns = strtol(optarg, NULL, 10);
					if (runtime_reruns == 0 || runtime_reruns > 1000000) {
						printf("Desired amount of reruns invalid!\nStay within 1 and 1000000 (both inclusive).\n");
						return EXIT_FAILURE;
					}
					benchmarking = true;
					break;
				}
			case 'T':
				if (optarg == 0) {
					test_mult = true;
					break;
				} else {
					number_of_blocks = strtol(optarg, NULL, 10);
					if (number_of_blocks == 0 || number_of_blocks > 1000000) {
						printf("Desired amount of blocks for testing multiplication invalid\nStay within 1 and 1000000 (both inclusive).\n");
						return EXIT_FAILURE;
					}
					test_mult = true;
					break;
				}

			default:
				print_help(progname);
				return EXIT_SUCCESS;			 
		}
	}

	struct bignum result;

	// Testing multiplication if flag is set
	if (test_mult) {
		// Setting up number
		struct bignum operand;
		operand.length = number_of_blocks;
		operand.subone = 0;
		operand.numbers = calloc(number_of_blocks, sizeof(uint32_t));
		if (operand.numbers == NULL) {
			fprintf(stderr, "Error while allocation memory!");
			return EXIT_FAILURE;
		}
		for (size_t i = 0; i < number_of_blocks; i++) {
			operand.numbers[i] = 1;
		}

		printf("Displaying runtimes of multiplication %ld blocks and %ld reruns:\n", number_of_blocks, runtime_reruns);
		printf("Operand number: ");
		printResultHex(&operand, 0);
		// Workaround to avoid free on uninitialized
		bignumInit(&result, 1);
		// temp is used to temporally store the result
		struct bignum temp;

		struct timespec start;
		clock_gettime(CLOCK_MONOTONIC, &start);
		for (size_t i = 0; i < runtime_reruns; i++) {
			temp = karazMult(&operand, &operand);
			bignumFree(&result);
			result = temp;
		}
		struct timespec end;
		clock_gettime(CLOCK_MONOTONIC, &end);

		double time = end.tv_sec - start.tv_sec + 1e-9 * (end.tv_nsec - start.tv_nsec);
		double avg_time = time/runtime_reruns;

		printf("done after %f seconds, average time is %f seconds\n", time, avg_time);
		printf("Result: ");
		printResultHex(&result, 0);
		bignumFree(&result);
		bignumFree(&operand);
		return EXIT_SUCCESS;
	}

	switch(version){
		case 0:
			if (result_in_hex) {
				s = number_of_decimal_places * 4;
				if (benchmarking) {
					printf("Displaying runtime of computing %ld places with %ld reruns:\n", number_of_decimal_places, runtime_reruns);
					// Workatound to avoid free on uninitialized
					bignumInit(&result, 1);
					// temp is used to temporally store the result
					struct bignum temp;
					struct timespec start;
					clock_gettime(CLOCK_MONOTONIC, &start);
					for (size_t i = 0; i < runtime_reruns; i++) {
						temp = mainImplementation(s + 1, s);
						// Workaround to avoid memory leaks
						bignumFree(&result);
						result = temp;
					}
					struct timespec end;
					clock_gettime(CLOCK_MONOTONIC, &end);
					double time = end.tv_sec - start.tv_sec + 1e-9 * (end.tv_nsec - start.tv_nsec);
					double avg_time = time/runtime_reruns;
					printf("done after %f seconds, average time is %f seconds\n", time, avg_time);
				} else {
					printf("Printing %ld hexadecimal places after comma...\n", number_of_decimal_places);
					result = mainImplementation(s + 1, s);
				}
				printf("Result: ");
				printResultHex(&result, number_of_decimal_places);
				bignumFree(&result);
			} else {
				s = number_of_decimal_places;
				if (benchmarking) {
					printf("Displaying runtime of computing %ld places with %ld reruns:\n", number_of_decimal_places, runtime_reruns);
					// Workatound to avoid free on uninitialized
					bignumInit(&result, 1);
					// temp is used to temporally store the result
					struct bignum temp;
					struct timespec start;
					clock_gettime(CLOCK_MONOTONIC, &start);
					for (size_t i = 0; i < runtime_reruns; i++) {
						temp = mainImplementation(s + 1, s);
						// Workaround to avoid memory leaks
						bignumFree(&result);
						result = temp;
					}
					struct timespec end;
					clock_gettime(CLOCK_MONOTONIC, &end);
					double time = end.tv_sec - start.tv_sec + 1e-9 * (end.tv_nsec - start.tv_nsec);
					double avg_time = time/runtime_reruns;
					printf("done after %f seconds, average time is %f seconds\n", time, avg_time);
				} else {
					printf("Printing %ld hexadecimal places after comma...\n", number_of_decimal_places);
					result = mainImplementation(s + 1, s);
				}
				printf("Result: ");
				bignumPrintDec(&result, number_of_decimal_places);
				bignumFree(&result);
			}
			break;
		case 1: 
			if (result_in_hex) {
				s = number_of_decimal_places * 4;
				if (benchmarking) {
					printf("Displaying runtime of computing %ld places with %ld reruns:\n", number_of_decimal_places, runtime_reruns);
					// Workatound to avoid free on uninitialized
					bignumInit(&result, 1);
					// temp is used to temporally store the result
					struct bignum temp;
					struct timespec start;
					clock_gettime(CLOCK_MONOTONIC, &start);
					for (size_t i = 0; i < runtime_reruns; i++) {
						temp = secondImplementation(s + 1, s);
						// Workaround to avoid memory leaks
						bignumFree(&result);
						result = temp;
					}
					struct timespec end;
					clock_gettime(CLOCK_MONOTONIC, &end);
					double time = end.tv_sec - start.tv_sec + 1e-9 * (end.tv_nsec - start.tv_nsec);
					double avg_time = time/runtime_reruns;
					printf("done after %f seconds, average time is %f seconds.\n", time, avg_time);
				} else {
					printf("Printing %ld hexadecimal places after comma...\n", number_of_decimal_places);
					result = secondImplementation(s + 1, s);
				}
				printf("Result: ");
				printResultHex(&result, number_of_decimal_places);
				bignumFree(&result);
			} else {
				printf("Feature not yet implemented!\n");
			}
			break;
		default:
			printf("Unsupported version number!\n");
			return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
