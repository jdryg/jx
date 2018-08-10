#include <jx/histogram.h>
#include <jx/sys.h>
#include <bx/allocator.h>

namespace jx
{
#define SLOT_CAPACITY_DELTA 32

struct HistogramSlot
{
	int m_Value;
	uint32_t m_Freq;
};

struct Histogram
{
	bx::AllocatorI* m_Allocator;
	HistogramSlot* m_Slots;
	uint32_t m_NumSlots;
	uint32_t m_SlotCapacity;

	uint32_t* m_BucketFreq;
	int m_FirstBucketValue;
	uint32_t m_NumBuckets;
	uint32_t m_BucketSize;
};

static HistogramSlot* allocSlot(Histogram* h, int value);
static HistogramSlot* findSlot(Histogram* h, int value);

Histogram* createHistogram(bx::AllocatorI* allocator)
{
	Histogram* h = (Histogram*)BX_ALLOC(allocator, sizeof(Histogram));
	if (!h) {
		return nullptr;
	}

	bx::memSet(h, 0, sizeof(Histogram));
	h->m_Allocator = allocator;

	return h;
}

void destroyHistogram(Histogram* h)
{
	bx::AllocatorI* allocator = h->m_Allocator;

	BX_FREE(allocator, h->m_Slots);
	BX_FREE(allocator, h);
}

bool histogramAddValue(Histogram* h, int value, uint32_t freq)
{
	HistogramSlot* slot = allocSlot(h, value);
	if (!slot) {
		return false;
	}

	slot->m_Freq += freq;

	return true;
}

int histogramGetMinValue(const Histogram* histogram)
{
	const uint32_t numSlots = histogram->m_NumSlots;
	if (numSlots == 0) {
		return 0;
	}

	return histogram->m_Slots[0].m_Value;
}

int histogramGetMaxValue(const Histogram* histogram)
{
	const uint32_t numSlots = histogram->m_NumSlots;
	if (numSlots == 0) {
		return 0;
	}

	return histogram->m_Slots[numSlots - 1].m_Value;
}

uint32_t histogramGetNumBuckets(const Histogram* histogram)
{
	return histogram->m_NumBuckets != 0 ? histogram->m_NumBuckets : histogram->m_NumSlots;
}

uint32_t histogramGetBucketSize(const Histogram* histogram)
{
	return histogram->m_NumBuckets != 0 ? histogram->m_BucketSize : 1;
}

void histogramGetBucket(const Histogram* histogram, uint32_t bucketID, int* value, uint32_t* freq)
{
	if (histogram->m_NumBuckets == 0) {
		const HistogramSlot* slot = &histogram->m_Slots[bucketID];
		if (value) {
			*value = slot->m_Value;
		}
		if (freq) {
			*freq = slot->m_Freq;
		}
	} else {
		if (value) {
			*value = histogram->m_FirstBucketValue + bucketID * histogram->m_BucketSize;
		}
		if (freq) {
			*freq = histogram->m_BucketFreq[bucketID];
		}
	}
}

uint32_t histogramGetLargestBucketFreq(const Histogram* histogram)
{
	const uint32_t numBuckets = histogram->m_NumBuckets;
	if (numBuckets == 0) {
		const uint32_t numSlots = histogram->m_NumSlots;
		if (numSlots == 0) {
			return 1;
		}

		uint32_t maxFreq = histogram->m_Slots[0].m_Freq;
		for (uint32_t i = 1; i < numSlots; ++i) {
			maxFreq = bx::max<uint32_t>(maxFreq, histogram->m_Slots[i].m_Freq);
		}

		return maxFreq;
	}

	uint32_t maxFreq = histogram->m_BucketFreq[0];
	for (uint32_t i = 1; i < numBuckets; ++i) {
		maxFreq = bx::max<uint32_t>(maxFreq, histogram->m_BucketFreq[i]);
	}

	return maxFreq;
}

void histogramRecalcBuckets(Histogram* histogram, int firstBucketMinValue, uint32_t bucketSize)
{
	const int minValue = histogramGetMinValue(histogram);
	const int maxValue = histogramGetMaxValue(histogram);
	JX_CHECK(firstBucketMinValue <= minValue, "Invalid first bucket value");

	histogram->m_FirstBucketValue = firstBucketMinValue;
	histogram->m_BucketSize = bucketSize;

	const uint32_t numBuckets = ((maxValue - firstBucketMinValue) / bucketSize) + 1;
	if (numBuckets != histogram->m_NumBuckets) {
		histogram->m_BucketFreq = (uint32_t*)BX_REALLOC(histogram->m_Allocator, histogram->m_BucketFreq, sizeof(uint32_t) * numBuckets);
		JX_CHECK(histogram->m_BucketFreq != nullptr, "Memory allocation failed");
		histogram->m_NumBuckets = numBuckets;
	}

	bx::memSet(histogram->m_BucketFreq, 0, sizeof(uint32_t) * numBuckets);

	const uint32_t numSlots = histogram->m_NumSlots;
	for (uint32_t i = 0; i < numSlots; ++i) {
		const HistogramSlot* slot = &histogram->m_Slots[i];

		const uint32_t bucketID = (slot->m_Value - firstBucketMinValue) / bucketSize;
		JX_CHECK(bucketID < numBuckets, "Invalid bucket id");
		histogram->m_BucketFreq[bucketID] += slot->m_Freq;
	}
}

//////////////////////////////////////////////////////////////////////////
// Internal functions
//
static HistogramSlot* allocSlot(Histogram* h, int value)
{
	// Check if the value is already in the histogram
	HistogramSlot* existingSlot = findSlot(h, value);
	if (existingSlot) {
		return existingSlot;
	}

	// Make room for 1 more slot.
	if (h->m_NumSlots == h->m_SlotCapacity) {
		const uint32_t oldCapacity = h->m_SlotCapacity;
		const uint32_t newCapacity = oldCapacity + SLOT_CAPACITY_DELTA;

		HistogramSlot* newSlots = (HistogramSlot*)BX_ALLOC(h->m_Allocator, sizeof(HistogramSlot) * newCapacity);
		if (!newSlots) {
			return nullptr;
		}

		bx::memCopy(newSlots, h->m_Slots, sizeof(HistogramSlot) * oldCapacity);
		bx::memSet(&newSlots[oldCapacity], 0, sizeof(HistogramSlot) * SLOT_CAPACITY_DELTA);

		BX_FREE(h->m_Allocator, h->m_Slots);
		h->m_Slots = newSlots;
		h->m_SlotCapacity = newCapacity;
	}

	// Find the correct place for the new value
	// NOTE: Slots are sorted based on their value.
	const uint32_t numSlots = h->m_NumSlots;
	uint32_t pos = numSlots;
	for (uint32_t i = 0; i < numSlots; ++i) {
		const HistogramSlot* slot = &h->m_Slots[i];
		if (slot->m_Value > value) {
			pos = i;
			break;
		}
	}

	bx::memMove(&h->m_Slots[pos + 1], &h->m_Slots[pos], sizeof(HistogramSlot) * (numSlots - pos));
	++h->m_NumSlots;

	HistogramSlot* slot = &h->m_Slots[pos];
	slot->m_Value = value;
	slot->m_Freq = 0;

	return slot;
}

static HistogramSlot* findSlot(Histogram* h, int value)
{
	const uint32_t numSlots = h->m_NumSlots;
	if (!numSlots) {
		return nullptr;
	}

	HistogramSlot* slots = h->m_Slots;

	if (value < slots[0].m_Value || value >= slots[numSlots - 1].m_Value) {
		return nullptr;
	}

	// Binary search
	uint32_t l = 0;
	uint32_t r = h->m_NumSlots - 1;
	do {
		const uint32_t m = (l + r) >> 1;
		const int mval = slots[m].m_Value;
		if (mval == value) {
			return &slots[m];
		} else if (mval < value) {
			l = m;
		} else if (mval > value) {
			r = m;
		}
	} while (l != r);

	return slots[l].m_Value == value ? &slots[l] : nullptr;
}
}
