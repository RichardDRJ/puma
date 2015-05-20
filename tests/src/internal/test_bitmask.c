#include "minunit.h"
#include "internal/bitmask.h"
#include "internal/test_bitmask.h"
#include <stdlib.h>

char* test_constructorFillsWithInitialValue(void)
{
	struct pumaBitmask bm;

	createPumaBitmask(&bm, 5000, 0);
	for(size_t i = 0; i < 5000; ++i)
		mu_assert(pumaBitmaskGet(&bm, i) == 0, "Expected 0 in new bitmask, got 1.");
	destroyPumaBitmask(&bm);

	createPumaBitmask(&bm, 5000, 1);
	for(size_t i = 0; i < 5000; ++i)
		mu_assert(pumaBitmaskGet(&bm, i) == 1, "Expected 1 in new bitmask, got 0.");
	destroyPumaBitmask(&bm);

	return NULL;
}

char* test_getAndSetAreConsistent(void)
{
	struct pumaBitmask bm;
	createPumaBitmask(&bm, 5000, 0);
	for(size_t i = 0; i < 5000; ++i)
	{
		pumaBitmaskSet(&bm, i, 1);
		mu_assert(pumaBitmaskGet(&bm, i) == 1, "Expected 1 in bitmask, got 0.");
		pumaBitmaskSet(&bm, i, 0);
		mu_assert(pumaBitmaskGet(&bm, i) == 0, "Expected 0 in bitmask, got 1.");
	}
	destroyPumaBitmask(&bm);

	return NULL;
}

char* test_getFirstIndexOfValueIsCorrect(void)
{
	bool found;
	struct pumaBitmask bm;
	createPumaBitmask(&bm, 5000, 1);
	for(size_t i = 0; i < 4999; ++i)
	{
		mu_assert(pumaFirstIndexOfValue(&bm, 1, &found) == i,
				"Incorrect index for first index with value 1.");
		pumaBitmaskSet(&bm, i, 0);
	}
	pumaBitmaskSet(&bm, 4999, 0);
	pumaFirstIndexOfValue(&bm, 1, &found);
	mu_assert(!found,
			"pumaFirstIndexOfValue should set found to false if none found.");

	destroyPumaBitmask(&bm);

	return NULL;
}

char* test_getLastIndexOfValueIsCorrect(void)
{
	bool found;
	struct pumaBitmask bm;
	createPumaBitmask(&bm, 5000, 1);
	for(size_t i = 4999; i > 0; --i)
	{
		mu_assert(pumaLastIndexOfValue(&bm, 1, &found) == i,
				"Incorrect index for first index with value 1.");
		pumaBitmaskSet(&bm, i, 0);
	}
	pumaBitmaskSet(&bm, 0, 0);
	pumaLastIndexOfValue(&bm, 1, &found);
	mu_assert(!found,
			"pumaFirstIndexOfValue should set found to false if none found.");

	destroyPumaBitmask(&bm);

	return NULL;
}
