#include <jx/handlealloc32.h>
#include <jx/sys.h>
#include <bx/allocator.h>

namespace jx
{
struct FreeListNode
{
	FreeListNode* m_Next;
	FreeListNode* m_Prev;
	uint32_t m_FirstHandleID;
	uint32_t m_NumFreeHandles;
};

struct HandleAlloc32
{
	bx::AllocatorI* m_Allocator;
	FreeListNode* m_FreeList;
	uint32_t m_HandleCapacity;
	uint32_t m_CapacityDelta;
};

static void removeFreeListNode(HandleAlloc32* ha, FreeListNode* node);
static void insertFreeListNode(HandleAlloc32* ha, FreeListNode* newNode);

HandleAlloc32* createHandleAlloc32(bx::AllocatorI* allocator, uint32_t capacityDelta)
{
	HandleAlloc32* ha = (HandleAlloc32*)BX_ALLOC(allocator, sizeof(HandleAlloc32));
	if (!ha) {
		return nullptr;
	}

	bx::memSet(ha, 0, sizeof(HandleAlloc32));
	ha->m_Allocator = allocator;
	ha->m_CapacityDelta = capacityDelta;

	return ha;
}

void destroyHandleAlloc32(HandleAlloc32* ha)
{
	bx::AllocatorI* allocator = ha->m_Allocator;

	FreeListNode* node = ha->m_FreeList;
	while (node) {
		FreeListNode* next = node->m_Next;
		BX_FREE(allocator, node);
		node = next;
	}
	ha->m_FreeList = nullptr;

	ha->m_Allocator = nullptr;

	BX_FREE(allocator, ha);
}

uint32_t ha32AllocHandles(HandleAlloc32* ha, uint32_t n)
{
	// Find first free list slot which can hold n handles.
	FreeListNode* node = ha->m_FreeList;
	while (node) {
		if (node->m_NumFreeHandles >= n) {
			// Found. Allocate n handles from this node.
			const uint32_t firstHandleID = node->m_FirstHandleID;

			node->m_NumFreeHandles -= n;
			if (node->m_NumFreeHandles == 0) {
				// No more free handles in the node. Remove it from the free list.
				removeFreeListNode(ha, node);
				BX_FREE(ha->m_Allocator, node);
			} else {
				// There are more handles in this free node. Just increase the first handle index.
				node->m_FirstHandleID += n;
			}

			return firstHandleID;
		}

		node = node->m_Next;
	}

	// No free slot found to hold n handles.
	const uint32_t firstHandleID = ha->m_HandleCapacity;

	FreeListNode* newNode = (FreeListNode*)BX_ALLOC(ha->m_Allocator, sizeof(FreeListNode));
	newNode->m_FirstHandleID = ha->m_HandleCapacity + n;
	newNode->m_NumFreeHandles = ha->m_CapacityDelta - n;
	newNode->m_Next = nullptr;
	newNode->m_Prev = nullptr;
	ha->m_HandleCapacity += ha->m_CapacityDelta;

	insertFreeListNode(ha, newNode);

	return firstHandleID;
}

void ha32FreeHandles(HandleAlloc32* ha, uint32_t firstHandle, uint32_t n)
{
	// Check if we can merge these handles with an existing free list node.
	const uint32_t endHandleID = firstHandle + n;

	FreeListNode* node = ha->m_FreeList;
	while (node) {
		if (node->m_FirstHandleID == endHandleID) {
			// Merge after.
			node->m_FirstHandleID = firstHandle;
			node->m_NumFreeHandles += n;
			return;
		} else if (node->m_FirstHandleID + node->m_NumFreeHandles == firstHandle) {
			// Merge before.
			node->m_NumFreeHandles += n;
			return;
		}

		node = node->m_Next;
	}

	// Cannot merge these handles with an existing free list node. Create a new node
	// and insert it to the list.
	FreeListNode* newNode = (FreeListNode*)BX_ALLOC(ha->m_Allocator, sizeof(FreeListNode));
	newNode->m_FirstHandleID = firstHandle;
	newNode->m_NumFreeHandles = n;
	newNode->m_Next = nullptr;
	newNode->m_Prev = nullptr;

	insertFreeListNode(ha, newNode);
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

	// TODO: Check if handle is free

	return true;
}

//////////////////////////////////////////////////////////////////////////
// Internal
//
static void removeFreeListNode(HandleAlloc32* ha, FreeListNode* node)
{
	if (node->m_Prev) {
		node->m_Prev->m_Next = node->m_Next;
	}

	if (node->m_Next) {
		node->m_Next->m_Prev = node->m_Prev;
	}

	if (node == ha->m_FreeList) {
		ha->m_FreeList = node->m_Next;
	}
}

static void insertFreeListNode(HandleAlloc32* ha, FreeListNode* newNode)
{
	// Find the correct position in the free list to insert the given node.
	const uint32_t newFirstHandleID = newNode->m_FirstHandleID;
	FreeListNode* node = ha->m_FreeList;
	FreeListNode* prevNode = nullptr;
	while (node) {
		if (node->m_FirstHandleID > newFirstHandleID) {
			break;
		}

		prevNode = node;
		node = node->m_Next;
	}

	if (prevNode) {
		newNode->m_Next = prevNode->m_Next;
		newNode->m_Prev = prevNode;
		prevNode->m_Next = newNode;
	} else {
		newNode->m_Next = ha->m_FreeList;
		newNode->m_Prev = nullptr;
		ha->m_FreeList = newNode;
	}
}
}
