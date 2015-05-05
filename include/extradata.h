

#ifndef _PUMALIST__EXTRADATA_H_
#define _PUMALIST__EXTRADATA_H_

#ifdef __cplusplus
extern "C" {
#endif

struct pumaListExtraKernelData
{
	void* (*extraDataConstructor)(void* constructorData);
	void* constructorData;
	void (*extraDataDestructor)(void* data);
	void (*extraDataThreadReduce)(void* data);
	void (*extraDataReduce)(void* retValue, void* data[],
			unsigned int nThreads);
	void* retValue;
};

extern struct pumaListExtraKernelData emptyKernelData;

#ifdef __cplusplus
}
#endif

#endif // _PUMALIST__EXTRADATA_H_
