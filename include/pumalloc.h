#ifndef __PUMALIST__PUMALLOC_H__
#define __PUMALIST__PUMALLOC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "pumalist.h"

#include <stdlib.h>

void* pumalloc(struct pumaList* list);
void* pumallocManualBalancing(struct pumaList* list, void* balData);
void* pumallocAutoBalancing(struct pumaList* list, int* allocatedThread);
void* pumallocOnThread(struct pumaList* list, size_t thread);
void pufree(void* element);

#ifdef __cplusplus
}
#endif

#endif // __PUMALIST__PUMALLOC_H__
