#include <jx/handlealloc32.h>
#include <jx/sys.h>
#include <bx/allocator.h>
#include <bx/uint32_t.h>

namespace jx
{
#define SLOT_CAPACITY_DELTA 32

struct FreeListSlot
{
	uint32_t m_FirstHandleID;
	uint32_t m_NumFreeHandles;
};

struct HandleAlloc32
{
	bx::AllocatorI* m_Allocator;
	FreeListSlot* m_Slots;
	uint32_t m_NumSlots;
	uint32_t m_SlotCapacity;
	uint32_t m_HandleCapacity;
	uint32_t m_HandleCapacityDelta;
};

static FreeListSlot* allocSlot(HandleAlloc32* ha, uint32_t firstHandleID, uint32_t numFreeHandles);
static void freeSlot(HandleAlloc32* ha, uint32_t slotID);

HandleAlloc32* createHandleAlloc32(bx::AllocatorI* allocator, uint32_t capacityDelta)
{
	HandleAlloc32* ha = (HandleAlloc32*)BX_ALLOC(allocator, sizeof(HandleAlloc32));
	if (!ha) {
		return nullptr;
	}

	bx::memSet(ha, 0, sizeof(HandleAlloc32));
	ha->m_Allocator = allocator;
	ha->m_HandleCapacityDelta = capacityDelta;

	return ha;
}

void destroyHandleAlloc32(HandleAlloc32* ha)
{
	bx::AllocatorI* allocator = ha->m_Allocator;

	BX_ALIGNED_FREE(allocator, ha->m_Slots, 16);
	BX_FREE(allocator, ha);
}

uint32_t ha32AllocHandles(HandleAlloc32* ha, uint32_t n)
{
	JX_CHECK(n < ha->m_HandleCapacityDelta, "Very large number of handles requested. Increase handle capacity delta");

	// Find first free list slot which can hold n handles.
	const uint32_t numSlots = ha->m_NumSlots;
	for (uint32_t i = 0; i < numSlots; ++i) {
		FreeListSlot* slot = &ha->m_Slots[i];
		if (slot->m_NumFreeHandles >= n) {
			const uint32_t firstHandleID = slot->m_FirstHandleID;

			if (slot->m_NumFreeHandles == n) {
				slot->m_NumFreeHandles = 0; // Just in case something goes wrong.
				freeSlot(ha, i);
			} else {
				slot->m_NumFreeHandles -= n;
				slot->m_FirstHandleID += n;
			}

			return firstHandleID;
		}
	}

	// No free slot found to hold n handles.
	const uint32_t firstHandleID = ha->m_HandleCapacity;

	FreeListSlot* slot = allocSlot(ha, firstHandleID, ha->m_HandleCapacityDelta);
	if (!slot) {
		return UINT32_MAX;
	}

	slot->m_FirstHandleID += n;
	slot->m_NumFreeHandles -= n;
	ha->m_HandleCapacity += ha->m_HandleCapacityDelta;

	return firstHandleID;
}

void ha32FreeHandles(HandleAlloc32* ha, uint32_t firstHandle, uint32_t n)
{
	// Check if we can merge these handles with an existing free list node.
	const uint32_t endHandleID = firstHandle + n;

	const uint32_t numSlots = ha->m_NumSlots;
	for (uint32_t i = 0; i < numSlots; ++i) {
		FreeListSlot* slot = &ha->m_Slots[i];
		if (slot->m_FirstHandleID == endHandleID) {
			// This slot starts at the point the specified range ends. Merge the range
			// to the existing slot.
			slot->m_FirstHandleID = firstHandle;
			slot->m_NumFreeHandles += n;

			// Check if this new slot can be merged with the previous slot.
			// NOTE: We check only the previous slot because if this slot (before the expansion) touched the
			// the next slot, it would have been already merged.
			if (i != 0) {
				FreeListSlot* prevSlot = &ha->m_Slots[i - 1];
				if (prevSlot->m_FirstHandleID + prevSlot->m_NumFreeHandles == firstHandle) {
					prevSlot->m_NumFreeHandles += slot->m_NumFreeHandles;
					slot->m_NumFreeHandles = 0; // Just in case something goes wrong.
					freeSlot(ha, i);
				}
			}
			return;
		} else if (slot->m_FirstHandleID + slot->m_NumFreeHandles == firstHandle) {
			// This slot ends at the point the specified range starts. Merge the range 
			// to the existing slot.
			slot->m_NumFreeHandles += n;

			// Check if this new slot can be merged with the next slot.
			// NOTE: We check only the next slot because if this slot (before the expansion) touched the
			// previous slot, it would have been already merged.
			if (i != numSlots - 1) {
				FreeListSlot* nextSlot = &ha->m_Slots[i + 1];
				if (slot->m_FirstHandleID + slot->m_NumFreeHandles == nextSlot->m_FirstHandleID) {
					slot->m_NumFreeHandles += nextSlot->m_NumFreeHandles;
					nextSlot->m_NumFreeHandles = 0; // Just in case something goes wrong.
					freeSlot(ha, i + 1);
				}
			}
			return;
		}
	}

	// Cannot merge these handles with an existing free list node. Create a new node
	// and insert it to the list.
	allocSlot(ha, firstHandle, n);
}

uint32_t ha32GetCapacity(HandleAlloc32* ha)
{
	return ha->m_HandleCapacity;
}

bool ha32IsValid(HandleAlloc32* ha, uint32_t handle)
{
	if (handle >= ha->m_HandleCapacity) {
		return false;
	}

	const uint32_t numSlots = ha->m_NumSlots;
	for (uint32_t i = 0; i < numSlots; ++i) {
		FreeListSlot* slot = &ha->m_Slots[i];
		if (handle >= slot->m_FirstHandleID && handle < slot->m_FirstHandleID + slot->m_NumFreeHandles) {
			return false; // Handle is free so it's not valid
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
// Internal
//
static FreeListSlot* allocSlot(HandleAlloc32* ha, uint32_t firstHandleID, uint32_t numFreeHandles)
{
	// Make sure we have enough room for an extra slot.
	const uint32_t numSlots = ha->m_NumSlots;
	if (numSlots == ha->m_SlotCapacity) {
		const uint32_t oldCapacity = ha->m_SlotCapacity;
		const uint32_t newCapacity = oldCapacity + SLOT_CAPACITY_DELTA;
		FreeListSlot* newSlots = (FreeListSlot*)BX_ALIGNED_ALLOC(ha->m_Allocator, sizeof(FreeListSlot) * newCapacity, 16);
		if (!newSlots) {
			return nullptr;
		}

		bx::memCopy(newSlots, ha->m_Slots, sizeof(FreeListSlot) * oldCapacity);
		bx::memSet(&newSlots[oldCapacity], 0, sizeof(FreeListSlot) * SLOT_CAPACITY_DELTA);

		BX_ALIGNED_FREE(ha->m_Allocator, ha->m_Slots, 16);
		ha->m_Slots = newSlots;
		ha->m_SlotCapacity = newCapacity;
	}

	// Find the correct position for the new slot (NOTE: Slots are sorted based on their first handle ID).
	uint32_t pos = ha->m_NumSlots++; // Initially assume to be at the end of the list.
	for (uint32_t i = 0; i < numSlots; ++i) {
		FreeListSlot* slot = &ha->m_Slots[i];
		if (slot->m_FirstHandleID > firstHandleID) {
			pos = i;
			break;
		}
	}

	if (pos != numSlots) {
		bx::memMove(&ha->m_Slots[pos + 1], &ha->m_Slots[pos], sizeof(FreeListSlot) * (numSlots - pos));
	}

	FreeListSlot* slot = &ha->m_Slots[pos];
	slot->m_FirstHandleID = firstHandleID;
	slot->m_NumFreeHandles = numFreeHandles;

	return slot;
}

static void freeSlot(HandleAlloc32* ha, uint32_t slotID)
{
	const uint32_t numSlots = ha->m_NumSlots;
	JX_CHECK(slotID < numSlots, "Invalid slot id");

	bx::memMove(&ha->m_Slots[slotID], &ha->m_Slots[slotID + 1], sizeof(FreeListSlot) * (numSlots - slotID - 1));
	--ha->m_NumSlots;
}
}
