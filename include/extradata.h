#ifndef _PUMALIST__EXTRADATA_H_
#define _PUMALIST__EXTRADATA_H_

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

#endif // _PUMALIST__EXTRADATA_H_
