#ifndef _PUMALIST__PUMA_H_
#define _PUMALIST__PUMA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "pumalist.h"
#include "pumastaticvar.h"
#include "pumathreadpool.h"
#include "pumakernel.h"
#include "extradata.h"
#include "pumalloc.h"

void initPuma(int numThreads);

#ifdef __cplusplus
}
#endif

#endif // _PUMALIST__PUMA_H_
