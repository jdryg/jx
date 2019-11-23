#ifndef JX_MOVING_AVERAGE_H
#define JX_MOVING_AVERAGE_H

#include <stdint.h>

namespace bx
{
struct AllocatorI;
}

namespace jx
{
struct MovingAverage;

MovingAverage* createMovingAverage(bx::AllocatorI* allocator, uint32_t n);
void destroyMovingAverage(MovingAverage* ma);

float movAvgPush(MovingAverage* ma, float val);
float movAvgGetAverage(const MovingAverage* ma);
float movAvgGetStdDev(const MovingAverage* ma);
void movAvgGetBounds(const MovingAverage* ma, float* minVal, float* maxVal);
const float* movAvgGetValues(const MovingAverage* ma);
uint32_t movAvgGetNumValues(const MovingAverage* ma);
}

#endif
