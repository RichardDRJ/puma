#ifndef __PUMALIST__BITMASK_H__
#define __PUMALIST__BITMASK_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

struct pumaBitmask
{
	uint64_t* buckets;
	size_t numElements;
};

struct pumaBitmask* createPumaBitmask(const size_t numElements,
		const uint8_t initialValue);
void destroyPumaBitmask(struct pumaBitmask* bitmask);
void pumaBitmaskSet(struct pumaBitmask* bitmask, const size_t index,
		uint8_t value);
uint8_t pumaBitmaskGet(const struct pumaBitmask* const bitmask,
		const size_t index);
size_t pumaFirstIndexOfValue(const struct pumaBitmask* const bitmask,
		int8_t value, bool* const found);
size_t pumaLastIndexOfValue(const struct pumaBitmask* const bitmask,
		int8_t value, bool* const found);
void pumaBitmaskToString(const struct pumaBitmask* const bitmask,
		char* buf, const size_t bufLen);

static const unsigned char MASKFREE = 1;
static const unsigned char MASKNOTFREE = 0;

#endif // __PUMALIST__BITMASK_H__