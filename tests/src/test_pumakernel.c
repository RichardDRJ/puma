#include "minunit.h"
#include <stdlib.h>
#include <string.h>
#include "puma.h"

static const size_t NUM_ELEMENTS = 10000;

struct testElem
{
	bool touched;
};

static void _touchKernel(void* vElem, void* extraData)
{
	(void)extraData;
	struct testElem* elem = (struct testElem*)vElem;
	elem->touched = true;
}

static void _unTouchKernel(void* vElem, void* extraData)
{
	(void)extraData;
	struct testElem* elem = (struct testElem*)vElem;
	elem->touched = false;
}

char* test_runKernelIteratesOverAll(void)
{
	for(size_t elementSize = sizeof(struct testElem);
			elementSize < 100 + sizeof(struct testElem); ++elementSize)
	{
		struct pumaList* list = createPumaList(elementSize, 1, NULL);
		struct testElem** elements =
				(struct testElem**)malloc(sizeof(struct testElem*) * NUM_ELEMENTS);

		for(size_t i = 0; i < NUM_ELEMENTS; ++i)
			elements[i] = pumalloc(list);

		runKernel(list, &_unTouchKernel, &emptyKernelData);

		for(size_t i = 0; i < NUM_ELEMENTS; ++i)
			mu_assert(!elements[i]->touched, "Element touched when it shouldn't be!");

		runKernel(list, &_touchKernel, &emptyKernelData);

		for(size_t i = 0; i < NUM_ELEMENTS; ++i)
			mu_assert(elements[i]->touched, "Element untouched when it should be!");

		destroyPumaList(list);
	}

	return NULL;
}
