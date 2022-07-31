#ifndef JX_DEQUEUE_H
#define JX_DEQUEUE_H

#include <stdint.h>

namespace bx
{
struct AllocatorI;
}

namespace jx
{
struct Deque;

Deque* createDeque(bx::AllocatorI* allocator, uint32_t elemSize);
void destroyDeque(Deque* dq);

void dequePushFront(Deque* dq, const void* item);
void dequePushBack(Deque* dq, const void* item);
void dequePopFront(Deque* dq, void* item);
void dequePopBack(Deque* dq, void* item);
void dequePeekFront(Deque* dq, void* item);
void dequePeekBack(Deque* dq, void* item);
bool dequeIsEmpty(const Deque* dq);
void dequeClear(Deque* dq);
uint32_t dequeGetSize(const Deque* dq);
}

#endif
