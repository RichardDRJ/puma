#include "minunit.h"

#include "internal/test_bitmask.h"

int tests_run = 0;
int tests_failed = 0;

int main(void)
{
	mu_run_test(test_constructorFillsWithInitialValue);
	mu_run_test(test_getAndSetAreConsistent);
	mu_run_test(test_getFirstIndexOfValueIsCorrect);
	mu_run_test(test_getLastIndexOfValueIsCorrect);

	return (tests_failed != 0);
}