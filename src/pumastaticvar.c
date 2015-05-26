#include "pumastaticvar.h"
#include "internal/numa.h"
#include "internal/pumadomain.h"
#include "internal/pumautil.h"

#include <pthread.h>
#include <stdbool.h>
#include <string.h>

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

static struct pumaStaticNode* _appendStaticNode(struct pumaStaticNode* prev,
		const size_t nodeSize)
{
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

static inline bool _tailHasSpace(const size_t size)
{
	struct pumaStaticNode* tail = pthread_getspecific(_staticTailKey);
	return tail->blockSize - tail->used >= size;
}

static void _initialiseStaticNodes(void)
{
	pthread_key_create(&_staticHeadKey, NULL);
	pthread_key_create(&_staticTailKey, NULL);
}

void* pumallocStaticLocal(const size_t size)
{
	void* ret;
	(void)pthread_once(&_initialiseOnce, &_initialiseStaticNodes);

	if(pthread_getspecific(_staticHeadKey) == NULL)
	{
		struct pumaStaticNode* newHead = _appendStaticNode(NULL,
				_getSmallestContainingPages(size));

		pthread_setspecific(_staticHeadKey, newHead);
		pthread_setspecific(_staticTailKey, newHead);
	}

	struct pumaStaticNode* tail = pthread_getspecific(_staticTailKey);

	if(!_tailHasSpace(size))
		tail = _appendStaticNode(tail, _getSmallestContainingPages(size));

	ret = tail->nextFree;
	tail->nextFree += size;
	tail->used += size;

	return ret;
}

void pumaDeleteStaticData(void)
{
	(void)pthread_once(&_initialiseOnce, &_initialiseStaticNodes);

	struct pumaStaticNode* head = pthread_getspecific(_staticHeadKey);

	while(head != NULL)
	{
		struct pumaStaticNode* next = head->next;
		nufree(head, head->blockSize);
		head = next;
	}

	pthread_once_t _dummyOnce = PTHREAD_ONCE_INIT;
	memcpy(&_initialiseOnce, &_dummyOnce, sizeof(pthread_once_t));
	pthread_key_delete(_staticHeadKey);
	pthread_key_delete(_staticTailKey);
}
