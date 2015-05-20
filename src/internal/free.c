#include "internal/free.h"

#include "internal/pumautil.h"
#include "internal/pumanode.h"
#include "internal/pumathreadlist.h"
#include "internal/valgrind.h"

#include <assert.h>
#include <unistd.h>

void _freeElementOnNode(void* element, struct pumaNode* node)
{
	node->dirty = true;

	size_t index = _getIndexOfElement(element);

	assert(pumaBitmaskGet(&node->freeMask, index) == MASKNOTFREE);
	pumaBitmaskSet(&node->freeMask, index, MASKFREE);
	assert(pumaBitmaskGet(&node->freeMask, index) == MASKFREE);

	size_t elementSize = node->elementSize;

	VALGRIND_FREELIKE_BLOCK(element, 0);
	VALGRIND_MAKE_MEM_NOACCESS(element, elementSize);

	--node->numElements;
	VALGRIND_MAKE_MEM_DEFINED(node->threadList, sizeof(struct pumaThreadList));
	--node->threadList->numElements;
	VALGRIND_MAKE_MEM_NOACCESS(node->threadList, sizeof(struct pumaThreadList));

	if(node->numElements < node->capacity / 2
			&& node->next != NULL && !node->next->active)
	{
		_freePumaNode(node->next);
		node->next = NULL;
	}
}
