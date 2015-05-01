#include "internal/alloc.h"

#include "internal/valgrind.h"
#include "internal/bitmask.h"

#include "internal/pumautil.h"
#include "internal/pumanode.h"

#include <assert.h>

void _allocElementOnNode(struct pumaNode* node, void* element)
{
	assert(node == _getNodeForElement(element));

	// node->dirty = true;

	assert(node->active);

	size_t index = _getIndexOfElementOnNode(element, node);

	assert(pumaBitmaskGet(node->freeMask, index) == MASKFREE);
	pumaBitmaskSet(node->freeMask, index, MASKNOTFREE);
	assert(pumaBitmaskGet(node->freeMask, index) == MASKNOTFREE);

	VALGRIND_MAKE_MEM_UNDEFINED(element, node->elementSize);
	VALGRIND_MALLOCLIKE_BLOCK(element, node->elementSize, 0, true);

	++node->numElements;
	VALGRIND_MAKE_MEM_DEFINED(node->threadList, sizeof(struct pumaThreadList));
	++node->threadList->numElements;
	VALGRIND_MAKE_MEM_NOACCESS(node->threadList, sizeof(struct pumaThreadList));
}