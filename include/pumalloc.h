#ifndef __PUMALIST__PUMALLOC_H__
#define __PUMALIST__PUMALLOC_H__

#include "pumalist.h"

#include <stdlib.h>

void* pumalloc(struct pumaList* list);
void* pumallocManualBalancing(struct pumaList* list, void* balData);
void* pumallocAutoBalancing(struct pumaList* list, int* allocatedThread);
void* pumallocOnThread(struct pumaList* list, size_t thread);
void pufree(void* element);

#endif // __PUMALIST__PUMALLOC_H__
