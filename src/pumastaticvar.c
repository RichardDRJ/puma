#include "pumastaticvar.h"
#include "internal/numa.h"
#include "internal/pumadomain.h"

#include <pthread.h>
#include <stdbool.h>

#ifndef PUMA_STATICPAGES
#define PUMA_STATICPAGES 1
#endif

extern size_t pumaPageSize;

struct pumaStaticNode
{
	void* nextFree;

	size_t used;
	size_t blockSize;

	struct pumaStaticNode* next;
	struct pumaStaticNode* prev;
};

static pthread_key_t _staticHeadKey;
static pthread_key_t _staticTailKey;
static pthread_once_t _initialiseOnce = PTHREAD_ONCE_INIT;

static struct pumaStaticNode* _appendStaticNode(struct pumaStaticNode* prev)
{
	size_t nodeSize = PUMA_STATICPAGES * pumaPageSize;
	int domain = _getCurrentNumaDomain();
	struct pumaStaticNode* ret = numalloc_on_node(nodeSize, domain);
	ret->nextFree = (void*)ret + sizeof(struct pumaStaticNode);
	ret->used = 0;
	ret->blockSize = nodeSize;
	ret->next = NULL;
	ret->prev = prev;
	if(prev != NULL)
		prev->next = ret;

	return ret;
}

static bool _tailHasSpace(size_t size)
{
	struct pumaStaticNode* tail = pthread_getspecific(_staticTailKey);
	return tail->blockSize - tail->used >= size;
}

static void _initialiseStaticNodes(void)
{
	pthread_key_create(&_staticHeadKey, NULL);
	pthread_key_create(&_staticTailKey, NULL);
}

void* pumallocStaticLocal(size_t size)
{
	void* ret;
	(void)pthread_once(&_initialiseOnce, &_initialiseStaticNodes);

	if(pthread_getspecific(_staticHeadKey) == NULL)
	{
		struct pumaStaticNode* newHead = _appendStaticNode(NULL);

		pthread_setspecific(_staticHeadKey, newHead);
		pthread_setspecific(_staticTailKey, newHead);
	}

	struct pumaStaticNode* tail = pthread_getspecific(_staticTailKey);

	if(!_tailHasSpace(size))
		tail = _appendStaticNode(tail);

	ret = tail->nextFree;
	tail->nextFree += size;
	tail->used += size;

	return ret;
}
