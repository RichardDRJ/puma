#include "internal/bitmask.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void pumaBitmaskSet(struct pumaBitmask* bitmask, const size_t index,
		uint8_t value)
{
	size_t bucket = index / 64;
	uint8_t lOffset = 63 - (uint8_t)(index % 64);

	value &= 1;
	bitmask->buckets[bucket] |= ((uint64_t)value << lOffset);
	bitmask->buckets[bucket] &= ~((uint64_t)(1 - value) << lOffset);
}

uint8_t pumaBitmaskGet(const struct pumaBitmask* const bitmask, const size_t index)
{
	size_t bucket = index / 64;
	uint8_t lOffset = 63 - (uint8_t)(index % 64);

	uint64_t bucketValue = bitmask->buckets[bucket];
	bucketValue >>= lOffset;
	return (uint8_t)(bucketValue & 1);
}

void pumaBitmaskToString(const struct pumaBitmask* const bitmask,
		char* buf, const size_t bufLen)
{
	size_t i;
	for(i = 0; i < bitmask->numElements && i < bufLen - 1; ++i)
		snprintf(&buf[i], 2, "%c", '0' + pumaBitmaskGet(bitmask, i));
	buf[i] = 0;
}

size_t pumaFirstIndexOfValue(const struct pumaBitmask* const bitmask,
		int8_t value, bool* const found)
{
	size_t bucket = 0;

	int8_t maskValue = value & 1;
	maskValue -= 1;
	uint64_t currentBucket = bitmask->buckets[0];

	if(found)
		*found = true;

	for(bucket = 0; bucket * 64 < bitmask->numElements; ++bucket)
	{
		size_t numFromBucketOn = bitmask->numElements - bucket * 64;
		currentBucket = bitmask->buckets[bucket];
		uint64_t bucketValueMask = (uint64_t)-1;

		if(numFromBucketOn < 64)
			bucketValueMask <<= (64 - numFromBucketOn);
		else
			numFromBucketOn = 64;

		if(((currentBucket ^ maskValue) & bucketValueMask) != 0)
		{
			if(bucket * 64 < bitmask->numElements)
			{
				for(uint8_t i = 0; i < numFromBucketOn; ++i)
				{
					if(((currentBucket >> (63 - i)) & 1) == value)
						return bucket * 64 + i;
				}
			}
		}
	}

	if(found)
		*found = false;
	return -1;
}

size_t pumaLastIndexOfValue(const struct pumaBitmask* const bitmask,
		int8_t value, bool* const found)
{
	// size_t bufLen = bitmask->numElements + 64;
	// char buf[bufLen];

	// pumaBitmaskToString(bitmask, &buf[0], bufLen);
	// printf("finding last index of value %d\tfreeMask 0b%s\n", value, buf);

	size_t bucket = (bitmask->numElements + 63) / 64;

	int8_t maskValue = value & 1;
	maskValue -= 1;

	if(found)
		*found = true;

	while(bucket-- > 0)
	{
		size_t numFromBucketOn = bitmask->numElements - bucket * 64;
		uint64_t bucketValueMask = (uint64_t)-1;

		if(numFromBucketOn < 64)
			bucketValueMask <<= (64 - numFromBucketOn);
		else
			numFromBucketOn = 64;

		if(((bitmask->buckets[bucket] ^ maskValue) & bucketValueMask) != 0)
		{
			for(uint8_t i = (64 - numFromBucketOn); i < 64; ++i)
			{
				if(((bitmask->buckets[bucket] >> i) & 1) == value)
					return bucket * 64 + (63 - i);
			}
		}
	}

	if(found)
		*found = false;

	return -1;
}

void createPumaBitmask(struct pumaBitmask* bm, const size_t numElements,
		const uint8_t initialValue)
{
	bm->buckets = (uint64_t*)malloc(((numElements + 63) / 64) * sizeof(uint64_t));
	uint8_t value = initialValue & 1;
	value = ~(value - 1);
	memset(bm->buckets, value, ((numElements + 63) / 64) * sizeof(uint64_t));
	bm->numElements = numElements;
}

void destroyPumaBitmask(struct pumaBitmask* bitmask)
{
	free(bitmask->buckets);
}
