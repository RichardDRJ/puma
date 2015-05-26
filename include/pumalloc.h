#ifndef __PUMA__PUMALLOC_H__
#define __PUMA__PUMALLOC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "pumaset.h"

#include <stdlib.h>

void* pumalloc(struct pumaSet* set);
void* pumallocManualBalancing(struct pumaSet* set, void* balData);
void* pumallocAutoBalancing(struct pumaSet* set);
void pufree(void* element);

#ifdef __cplusplus
}
#endif

#endif // __PUMA__PUMALLOC_H__
