#ifndef JX_PROFILER_H
#define JX_PROFILER_H

#include <jx/logger.h>
#include <jx/sys.h>
#include <bx/timer.h>

#define JX_PROFILE_START(id) int64_t __profileStart_##id = bx::getHPCounter();
#define JX_PROFILE_END(id, text) \
    int64_t __profileDelta_##id = bx::getHPCounter() - __profileStart_##id; \
    JX_LOG_DEBUG("%s took %.2f msec\n", text, 1000.0 * (double)__profileDelta_##id / (double)bx::getHPFrequency())

#endif
