#include <jx/thread.h>
#include <jx/object_pool.h>
#include <jx/sys.h>

namespace jx
{
static const uint32_t kDefaultMessagePoolBlockSize = 128;

struct Thread
{
	bx::AllocatorI* m_Allocator;
	jx::ObjectPool* m_MsgPool;
	bx::Thread m_bxThread;
	bx::SpScBlockingUnboundedQueue m_InMsgQueue;
	bx::SpScBlockingUnboundedQueue m_OutMsgQueue;
	bx::Mutex m_MsgPoolMutex;
	ThreadFn m_Func;
	void* m_UserData;
};

static ThreadMessage* threadPopMessage(Thread* thread, bx::SpScBlockingUnboundedQueue* queue, int32_t timeout_msec);
static bool threadPushMessage(Thread* thread, bx::SpScBlockingUnboundedQueue* queue, uint32_t msgID, const void* data, uint32_t sz);
static int32_t threadFunc(bx::Thread* self, void* userData);

Thread* createThread(bx::AllocatorI* allocator, ThreadFn func, void* userData, uint32_t stackSize, const char* name)
{
	Thread* thread = (Thread*)BX_ALLOC(allocator, sizeof(Thread));
	if (!thread) {
		return nullptr;
	}

	bx::memSet(thread, 0, sizeof(Thread));
	thread->m_Allocator = allocator;
	thread->m_Func = func;
	thread->m_UserData = userData;

	BX_PLACEMENT_NEW(&thread->m_bxThread, bx::Thread)();
	BX_PLACEMENT_NEW(&thread->m_InMsgQueue, bx::SpScBlockingUnboundedQueue)(allocator);
	BX_PLACEMENT_NEW(&thread->m_OutMsgQueue, bx::SpScBlockingUnboundedQueue)(allocator);
	BX_PLACEMENT_NEW(&thread->m_MsgPoolMutex, bx::Mutex)();

	thread->m_MsgPool = jx::createObjectPool(sizeof(ThreadMessage), kDefaultMessagePoolBlockSize, allocator);
	if (!thread->m_MsgPool) {
		destroyThread(thread);
		return nullptr;
	}

	if (!thread->m_bxThread.init(threadFunc, thread, stackSize, name)) {
		destroyThread(thread);
		return nullptr;
	}

	return thread;
}

void destroyThread(Thread* thread)
{
	if (thread->m_bxThread.isRunning()) {
		thread->m_bxThread.shutdown();
	}

	if (thread->m_MsgPool) {
		jx::destroyObjectPool(thread->m_MsgPool);
		thread->m_MsgPool = nullptr;
	}

	thread->m_InMsgQueue.~SpScBlockingUnboundedQueue();
	thread->m_OutMsgQueue.~SpScBlockingUnboundedQueue();
	thread->m_MsgPoolMutex.~Mutex();
	thread->m_bxThread.~Thread();

	BX_FREE(thread->m_Allocator, thread);
}

ThreadMessage* threadInQueuePop(Thread* thread, int32_t timeout_msec)
{
	return threadPopMessage(thread, &thread->m_InMsgQueue, timeout_msec);
}

bool threadInQueuePush(Thread* thread, uint32_t msgID, const void* data, uint32_t sz)
{
	return threadPushMessage(thread, &thread->m_InMsgQueue, msgID, data, sz);
}

ThreadMessage* threadOutQueuePop(Thread* thread, int32_t timeout_msec)
{
	return threadPopMessage(thread, &thread->m_OutMsgQueue, timeout_msec);
}

bool threadOutQueuePush(Thread* thread, uint32_t msgID, const void* data, uint32_t sz)
{
	return threadPushMessage(thread, &thread->m_OutMsgQueue, msgID, data, sz);
}

void threadReleaseMessage(Thread* thread, ThreadMessage* msg)
{
	bx::MutexScope ms(thread->m_MsgPoolMutex);
	jx::objPoolFree(thread->m_MsgPool, msg);
}

static ThreadMessage* threadPopMessage(Thread* thread, bx::SpScBlockingUnboundedQueue* queue, int32_t timeout_msec)
{
	BX_UNUSED(thread);
	return (timeout_msec == 0 && queue->peek() == nullptr)
		? nullptr
		: (ThreadMessage*)queue->pop(timeout_msec)
		;
}

static bool threadPushMessage(Thread* thread, bx::SpScBlockingUnboundedQueue* queue, uint32_t msgID, const void* data, uint32_t sz)
{
	if (sz > JX_THREAD_MESSAGE_BUFFER_SIZE) {
		JX_CHECK(false, "Thread message data too large!");
		return false;
	}

	bx::MutexScope ms(thread->m_MsgPoolMutex);
	ThreadMessage* msg = (ThreadMessage*)jx::objPoolAlloc(thread->m_MsgPool);
	if (!msg) {
		return false;
	}

	msg->m_MsgID = msgID;
	bx::memCopy(msg->m_Data, data, sz);

	queue->push(msg);

	return true;
}

static int32_t threadFunc(bx::Thread* self, void* userData)
{
	Thread* thread = (Thread*)userData;

	bool done = false;
	while (!done) {
		const int32_t retCode = thread->m_Func(thread, thread->m_UserData);
		if (retCode == 0) {
			done = true;
		}
	}

	return 0;
}
}
