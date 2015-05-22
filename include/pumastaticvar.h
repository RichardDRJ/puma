#ifndef _PUMALIST__PUMASTATICVAR_H_
#define _PUMALIST__PUMASTATICVAR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

void* pumallocStaticLocal(size_t size);
void pumaDeleteStaticData(void);

#ifdef __cplusplus
}
#endif

#endif // _PUMALIST__PUMASTATICVAR_H_
