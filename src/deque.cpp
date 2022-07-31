#include <jx/deque.h>
#include <bx/allocator.h>

#define DEQUE_CONFIG_ITEM_CAPACITY_DELTA 256
#define DEQUE_CONFIG_ITEMS_PER_BUFFER    64

namespace jx
{
struct Deque
{
	bx::AllocatorI* m_Allocator;
	uint8_t** m_ItemBuffers;
	uint32_t m_NumItemBuffers;
	uint32_t m_ItemBufferNextFreeSlotID;
	uint32_t* m_Items;
	uint32_t m_ItemSize;
	uint32_t m_NumItems;
	uint32_t m_ItemCapacity;
};

static void deque_insertItem(Deque* dq, const void* item, uint32_t pos);
static void deque_removeItem(Deque* dq, uint32_t pos, void* item);
static uint32_t deque_getItemID(Deque* dq, uint32_t pos);
static const void* deque_getItem(Deque* dq, uint32_t itemID);

Deque* createDeque(bx::AllocatorI* allocator, uint32_t itemSize)
{
	Deque* dq = (Deque*)BX_ALLOC(allocator, sizeof(Deque));
	if (!dq) {
		return nullptr;
	}

	bx::memSet(dq, 0, sizeof(Deque));
	dq->m_Allocator = allocator;
	dq->m_ItemSize = itemSize;

	return dq;
}

void destroyDeque(Deque* dq)
{
	bx::AllocatorI* allocator = dq->m_Allocator;

	// Free all buffers
	const uint32_t numItemBuffers = dq->m_NumItemBuffers;
	for (uint32_t i = 0; i < numItemBuffers; ++i) {
		BX_FREE(allocator, dq->m_ItemBuffers[i]);
		dq->m_ItemBuffers[i] = nullptr;
	}
	BX_FREE(allocator, dq->m_ItemBuffers);
	dq->m_ItemBuffers = nullptr;
	dq->m_NumItemBuffers = 0;
	dq->m_ItemBufferNextFreeSlotID = UINT32_MAX;

	// Free item list
	BX_FREE(allocator, dq->m_Items);

	BX_FREE(allocator, dq);
}

void dequePushFront(Deque* dq, const void* item)
{
	deque_insertItem(dq, item, 0);
}

void dequePushBack(Deque* dq, const void* item)
{
	deque_insertItem(dq, item, dequeGetSize(dq));
}

void dequePopFront(Deque* dq, void* item)
{
	deque_removeItem(dq, 0, item);
}

void dequePopBack(Deque* dq, void* item)
{
	deque_removeItem(dq, dequeGetSize(dq) - 1, item);
}

void dequePeekFront(Deque* dq, void* item)
{
	const void* srcItem = deque_getItem(dq, deque_getItemID(dq, 0));
	bx::memCopy(item, srcItem, dq->m_ItemSize);
}

void dequePeekBack(Deque* dq, void* item)
{
	const void* srcItem = deque_getItem(dq, deque_getItemID(dq, dequeGetSize(dq) - 1));
	bx::memCopy(item, srcItem, dq->m_ItemSize);
}

bool dequeIsEmpty(const Deque* dq)
{
	return dequeGetSize(dq) == 0;
}

void dequeClear(Deque* dq)
{
	dq->m_NumItems = 0;

	// Free all buffers
	const uint32_t numItemBuffers = dq->m_NumItemBuffers;
	for (uint32_t i = 0; i < numItemBuffers; ++i) {
		BX_FREE(dq->m_Allocator, dq->m_ItemBuffers[i]);
		dq->m_ItemBuffers[i] = nullptr;
	}
	BX_FREE(dq->m_Allocator, dq->m_ItemBuffers);
	dq->m_ItemBuffers = nullptr;
	dq->m_NumItemBuffers = 0;
	dq->m_ItemBufferNextFreeSlotID = UINT32_MAX;
}

uint32_t dequeGetSize(const Deque* dq)
{
	return dq->m_NumItems;
}

static uint32_t deque_allocItem(Deque* dq, const void* item)
{
	if (dq->m_ItemBufferNextFreeSlotID == UINT32_MAX) {
		// No free slot. Allocate a new buffer.
		uint8_t* buffer = (uint8_t*)BX_ALLOC(dq->m_Allocator, dq->m_ItemSize * DEQUE_CONFIG_ITEMS_PER_BUFFER);
		if (!buffer) {
			return UINT32_MAX;
		}

		// Initialize all new items as free items by pointing the free slot index to the next item.
		const uint32_t nextFreeSlotID = dq->m_NumItemBuffers * DEQUE_CONFIG_ITEMS_PER_BUFFER;
		for (uint32_t i = 0; i < DEQUE_CONFIG_ITEMS_PER_BUFFER; ++i) {
			uint8_t* ptr = buffer + i * dq->m_ItemSize;
			*(uint32_t*)ptr = nextFreeSlotID + i + 1;
		}

		// Terminate the free list
		{
			uint8_t* ptr = buffer + (DEQUE_CONFIG_ITEMS_PER_BUFFER - 1) * dq->m_ItemSize;
			*(uint32_t*)ptr = UINT32_MAX;
		}

		// Point the free list to the first element of the new buffer
		dq->m_ItemBufferNextFreeSlotID = nextFreeSlotID;

		// Insert the new buffer into the buffer list
		dq->m_ItemBuffers = (uint8_t**)BX_REALLOC(dq->m_Allocator, dq->m_ItemBuffers, sizeof(uint8_t*) * (dq->m_NumItemBuffers + 1));
		dq->m_ItemBuffers[dq->m_NumItemBuffers] = buffer;
		dq->m_NumItemBuffers++;
	}

	const uint32_t freeSlot = dq->m_ItemBufferNextFreeSlotID;
	const uint32_t bufferID = (freeSlot % DEQUE_CONFIG_ITEMS_PER_BUFFER);
	const uint32_t slotID = freeSlot - (bufferID * DEQUE_CONFIG_ITEMS_PER_BUFFER);
	uint8_t* buffer = dq->m_ItemBuffers[bufferID];
	uint8_t* ptr = buffer + slotID * dq->m_ItemSize;
	
	// Update next free slot
	dq->m_ItemBufferNextFreeSlotID = *(uint32_t*)ptr;

	bx::memCopy(ptr, item, dq->m_ItemSize);

	return freeSlot;
}

static void deque_freeItem(Deque* dq, uint32_t itemID)
{
	const uint32_t bufferID = (itemID % DEQUE_CONFIG_ITEMS_PER_BUFFER);
	const uint32_t slotID = itemID - (bufferID * DEQUE_CONFIG_ITEMS_PER_BUFFER);
	uint8_t* buffer = dq->m_ItemBuffers[bufferID];
	uint8_t* ptr = buffer + slotID * dq->m_ItemSize;

	*(uint32_t*)ptr = dq->m_ItemBufferNextFreeSlotID;
	dq->m_ItemBufferNextFreeSlotID = itemID;
}

static bool deque_insertItemID(Deque* dq, uint32_t pos, uint32_t itemID)
{
	if (dq->m_NumItems == dq->m_ItemCapacity) {
		const uint32_t oldCapacity = dq->m_ItemCapacity;
		const uint32_t newCapacity = oldCapacity + DEQUE_CONFIG_ITEM_CAPACITY_DELTA;

		const uint32_t totalMemory = sizeof(uint32_t) * newCapacity;
		uint8_t* buffer = (uint8_t*)BX_ALLOC(dq->m_Allocator, totalMemory);
		if (!buffer) {
			return false;
		}

		uint8_t* ptr = buffer;
		uint32_t* newItems = (uint32_t*)buffer; ptr += sizeof(uint32_t) * newCapacity;

		bx::memCopy(newItems, dq->m_Items, sizeof(uint32_t) * oldCapacity);
		bx::memSet(&newItems[oldCapacity], 0xFF, sizeof(uint32_t) * (newCapacity - oldCapacity));

		BX_FREE(dq->m_Allocator, dq->m_Items);
		dq->m_Items = newItems;
		dq->m_ItemCapacity = newCapacity;
	}

	dq->m_NumItems++;
	bx::memMove(&dq->m_Items[pos + 1], &dq->m_Items[pos], sizeof(uint32_t) * (dq->m_NumItems - pos - 1));
	dq->m_Items[pos] = itemID;

	return true;
}

static void deque_removeItemID(Deque* dq, uint32_t pos)
{
	bx::memMove(&dq->m_Items[pos], &dq->m_Items[pos + 1], sizeof(uint32_t) * (dq->m_NumItems - pos - 1));
	dq->m_NumItems--;
}

static uint32_t deque_getItemID(Deque* dq, uint32_t pos)
{
	return dq->m_Items[pos];
}

static const void* deque_getItem(Deque* dq, uint32_t itemID)
{
	const uint32_t bufferID = (itemID % DEQUE_CONFIG_ITEMS_PER_BUFFER);
	const uint32_t slotID = itemID - (bufferID * DEQUE_CONFIG_ITEMS_PER_BUFFER);
	uint8_t* buffer = dq->m_ItemBuffers[bufferID];
	return buffer + slotID * dq->m_ItemSize;
}

static void deque_insertItem(Deque* dq, const void* item, uint32_t pos)
{
	const uint32_t itemID = deque_allocItem(dq, item);
	if (itemID != UINT32_MAX) {
		deque_insertItemID(dq, pos, itemID);
	}
}

static void deque_removeItem(Deque* dq, uint32_t pos, void* item)
{
	const uint32_t itemID = deque_getItemID(dq, pos);
	if (item) {
		const void* srcItem = deque_getItem(dq, itemID);
		bx::memCopy(item, srcItem, dq->m_ItemSize);
	}
	deque_removeItemID(dq, pos);
	deque_freeItem(dq, itemID);
}
}
