#include "minunit.h"

#include "puma.h"
#include "internal/test_pumautil.h"
#include "internal/test_bitmask.h"
#include <unistd.h>

int tests_run = 0;
int tests_failed = 0;
extern size_t pumaPageSize;

int main(void)
{
	if(pumaPageSize == 0)
		pumaPageSize = (size_t)sysconf(_SC_PAGESIZE);
	mu_run_test(test_constructorFillsWithInitialValue);
	mu_run_test(test_getAndSetAreConsistent);
	mu_run_test(test_getFirstIndexOfValueIsCorrect);
	mu_run_test(test_getLastIndexOfValueIsCorrect);
	mu_run_test(test_getLastElementGetsLastElement);
	mu_run_test(test_getIndexOfElementAndGetElementAreEquivalent);

	return (tests_failed != 0);
}
