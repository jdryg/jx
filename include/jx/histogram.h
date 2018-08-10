#ifndef JX_HISTOGRAM_H
#define JX_HISTOGRAM_H

#include <stdint.h>

namespace bx
{
struct AllocatorI;
}

namespace jx
{
struct Histogram;

Histogram* createHistogram(bx::AllocatorI* allocator);
void destroyHistogram(Histogram* histogram);

bool histogramAddValue(Histogram* histogram, int value, uint32_t freq);
int histogramGetMinValue(const Histogram* histogram);
int histogramGetMaxValue(const Histogram* histogram);

void histogramRecalcBuckets(Histogram* histogram, int firstBucketMinValue, uint32_t bucketSize);
uint32_t histogramGetNumBuckets(const Histogram* histogram);
uint32_t histogramGetBucketSize(const Histogram* histogram);
uint32_t histogramGetLargestBucketFreq(const Histogram* histogram);
void histogramGetBucket(const Histogram* histogram, uint32_t bucketID, int* value, uint32_t* freq);
}

#endif
