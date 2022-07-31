#ifndef JX_THREAD_H
#define JX_THREAD_H

#include <stdint.h>
#include <bx/thread.h>

namespace jx
{
#define JX_THREAD_MESSAGE_BUFFER_SIZE 60

struct ThreadMessage
{
	uint32_t m_MsgID;
	uint8_t m_Data[JX_THREAD_MESSAGE_BUFFER_SIZE];
};
BX_STATIC_ASSERT(sizeof(ThreadMessage) == 64, "Invalid ThreadMessage size");

struct Thread;

typedef int32_t (*ThreadFn)(Thread* self, void* userData);

Thread* createThread(bx::AllocatorI* allocator, ThreadFn func, void* userData = nullptr, uint32_t stackSize = 0u, const char* name = nullptr);
void destroyThread(Thread* thread);

ThreadMessage* threadInQueuePop(Thread* thread, int32_t timeout_msec);
bool threadInQueuePush(Thread* thread, uint32_t msgID, const void* data, uint32_t sz);
ThreadMessage* threadOutQueuePop(Thread* thread, int32_t timeout_msec);
bool threadOutQueuePush(Thread* thread, uint32_t msgID, const void* data, uint32_t sz);
void threadReleaseMessage(Thread* thread, ThreadMessage* msg);
}

#endif
