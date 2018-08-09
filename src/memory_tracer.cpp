#include "memory_tracer.h"
#include <jx/sys.h>
#include <jx/object_pool.h>
#include <bx/allocator.h>
#include <bx/mutex.h>
#include <bx/debug.h>

#if BX_PLATFORM_WINDOWS
#include <Windows.h>
#include <dbghelp.h>

typedef BOOL(__stdcall *tSC)(IN HANDLE hProcess);
typedef PVOID(__stdcall *tSFTA)(IN HANDLE hProcess, IN DWORD64 AddrBase);
typedef BOOL(__stdcall *tSGLFA)(IN HANDLE hProcess, IN DWORD64 dwAddr, OUT PDWORD pdwDisplacement, OUT PIMAGEHLP_LINE64 Line);
typedef DWORD64(__stdcall *tSGMB)(IN HANDLE hProcess, IN DWORD64 dwAddr);
typedef BOOL(__stdcall *tSI)(IN HANDLE hProcess, IN PCSTR UserSearchPath, IN BOOL fInvadeProcess);
typedef DWORD(__stdcall *tSSO)(IN DWORD SymOptions);
typedef BOOL(__stdcall *tSW)(DWORD MachineType, HANDLE hProcess, HANDLE hThread, LPSTACKFRAME64 StackFrame, PVOID ContextRecord, PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine, PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine, PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine, PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress);
#endif

namespace jx
{
#define TRACER_CONFIG_MAX_STACK_FRAMES 8

struct AllocationInfo
{
	AllocationInfo* m_Next;
	AllocationInfo* m_Prev;
	const void* m_Ptr;
	const char* m_Filename;
	uint32_t m_Line;
	size_t m_Size;
#if BX_PLATFORM_WINDOWS
	uint64_t m_StackFrameAddr[TRACER_CONFIG_MAX_STACK_FRAMES];
#endif
};

struct AllocatorInfo
{
	char m_Name[64];
	size_t m_TotalAllocatedMemory;
	AllocationInfo* m_Allocations;
	uint32_t m_TotalAllocations;
	uint32_t m_ActiveAllocations;
	bool m_IsAlive;
};

struct MemTracer
{
	bx::AllocatorI* m_Allocator;
	AllocatorInfo* m_Allocators;
	ObjectPool* m_AllocInfoPool;
#if BX_CONFIG_SUPPORTS_THREADING
	bx::Mutex* m_Mutex;
#endif
	uint16_t m_NumAllocators;
	char m_Name[64];

#if BX_PLATFORM_WINDOWS
	tSC SymCleanup;
	tSFTA SymFunctionTableAccess64;
	tSGLFA SymGetLineFromAddr64;
	tSGMB SymGetModuleBase64;
	tSI SymInitialize;
	tSSO SymSetOptions;
	tSW StackWalk64;
	HMODULE m_hDbgHelpDll;
#endif
};

static MemTracer* s_MemTracer = nullptr;

static AllocationInfo* findAllocation(AllocatorInfo* ai, const void* ptr);
static void getStackTrace(uint64_t* stack, uint32_t size);

bool memTracerInit(bx::AllocatorI* allocator)
{
	MemTracer* mt = (MemTracer*)BX_ALLOC(allocator, sizeof(MemTracer));
	if (!mt) {
		return false;
	}

	bx::memSet(mt, 0, sizeof(MemTracer));

	mt->m_Allocator = allocator;
#if BX_CONFIG_SUPPORTS_THREADING
	mt->m_Mutex = BX_NEW(allocator, bx::Mutex)();
#endif
	mt->m_AllocInfoPool = createObjectPool(sizeof(AllocationInfo), 2048, allocator);

#if BX_PLATFORM_WINDOWS
	char filename[256];
	GetModuleFileNameA(NULL, &filename[0], BX_COUNTOF(filename));
	char* lastSlash = strrchr(filename, '\\');
	*lastSlash = NULL;

	char dbgHelpFilename[256];
	bx::snprintf(dbgHelpFilename, BX_COUNTOF(dbgHelpFilename), "%s\\dbghelp.dll", filename);
	mt->m_hDbgHelpDll = LoadLibraryA(dbgHelpFilename);

	mt->SymCleanup = (tSC)GetProcAddress(mt->m_hDbgHelpDll, "SymCleanup");
	mt->SymFunctionTableAccess64 = (tSFTA)GetProcAddress(mt->m_hDbgHelpDll, "SymFunctionTableAccess64");
	mt->SymGetLineFromAddr64 = (tSGLFA)GetProcAddress(mt->m_hDbgHelpDll, "SymGetLineFromAddr64");
	mt->SymGetModuleBase64 = (tSGMB)GetProcAddress(mt->m_hDbgHelpDll, "SymGetModuleBase64");
	mt->SymInitialize = (tSI)GetProcAddress(mt->m_hDbgHelpDll, "SymInitialize");
	mt->SymSetOptions = (tSSO)GetProcAddress(mt->m_hDbgHelpDll, "SymSetOptions");
	mt->StackWalk64 = (tSW)GetProcAddress(mt->m_hDbgHelpDll, "StackWalk64");

	mt->SymInitialize(GetCurrentProcess(), NULL, TRUE);

	mt->SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_ANYTHING | SYMOPT_LOAD_LINES);
#endif

	s_MemTracer = mt;

	return true;
}

void memTracerShutdown()
{
	JX_CHECK(s_MemTracer != nullptr, "Memory tracer hasn't been initialized or already shutdown");

	bx::AllocatorI* allocator = s_MemTracer->m_Allocator;

	const uint32_t numAllocators = s_MemTracer->m_NumAllocators;
	for (uint32_t i = 0; i < numAllocators; ++i) {
		AllocatorInfo* ai = &s_MemTracer->m_Allocators[i];
		if (!ai->m_IsAlive) {
			continue;
		}

		JX_WARN(false, "Allocator %s is still alive", ai->m_Name);

		memTracerDestroyAllocator((uint16_t)i);
	}

	destroyObjectPool(s_MemTracer->m_AllocInfoPool);
#if BX_CONFIG_SUPPORTS_THREADING
	BX_DELETE(allocator, s_MemTracer->m_Mutex);
#endif
	
	BX_FREE(allocator, s_MemTracer);
	s_MemTracer = nullptr;
}

uint16_t memTracerCreateAllocator(const char* name)
{
	JX_CHECK(s_MemTracer != nullptr, "Memory tracer hasn't been initialized");

	MemTracer* ctx = s_MemTracer;

	ctx->m_Allocators = (AllocatorInfo*)BX_REALLOC(ctx->m_Allocator, ctx->m_Allocators, sizeof(AllocatorInfo) * (ctx->m_NumAllocators + 1));

	AllocatorInfo* ai = &ctx->m_Allocators[ctx->m_NumAllocators++];
	bx::memSet(ai, 0, sizeof(AllocatorInfo));
	bx::snprintf(ai->m_Name, BX_COUNTOF(ai->m_Name), "%s", name);
	ai->m_IsAlive = true;

	return ctx->m_NumAllocators - 1;
}

void memTracerDestroyAllocator(uint16_t allocatorID)
{
	JX_CHECK(s_MemTracer != nullptr, "Memory tracer hasn't been initialized");
	JX_CHECK(allocatorID < s_MemTracer->m_NumAllocators, "Invalid allocator handle");

	MemTracer* ctx = s_MemTracer;

	AllocatorInfo* ai = &ctx->m_Allocators[allocatorID];
	JX_WARN(ai->m_IsAlive, "memTracerDestroyAllocator(): Allocator is already destroyed");

	bx::debugPrintf("Allocator %s\n", ai->m_Name);
	bx::debugPrintf("- # Active allocations : %u\n", ai->m_ActiveAllocations);
	bx::debugPrintf("- Allocated memory (kb): %u\n", ai->m_TotalAllocatedMemory >> 10);

	AllocationInfo* alloc = ai->m_Allocations;
	if (alloc) {
		bx::debugPrintf("Active allocations\n");
		while (alloc) {
			AllocationInfo* nextAlloc = alloc->m_Next;

			// Dump allocation info.
			bx::debugPrintf("- %d bytes @ %s:%d\n", alloc->m_Size, alloc->m_Filename, alloc->m_Line);

#if BX_PLATFORM_WINDOWS
			bx::debugPrintf("- Stack trace:\n");

			for (uint32_t iFrame = 0; iFrame < TRACER_CONFIG_MAX_STACK_FRAMES; ++iFrame) {
				IMAGEHLP_LINE64 line;
				line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

				DWORD offset_ln = 0;
				if (ctx->SymGetLineFromAddr64(GetCurrentProcess(), alloc->m_StackFrameAddr[iFrame], &offset_ln, &line)) {
					bx::debugPrintf("%s(%d):\n", line.FileName, line.LineNumber);
				} else {
					bx::debugPrintf("(null)(0):\n");
				}
			}
#endif

			objPoolFree(ctx->m_AllocInfoPool, alloc);

			alloc = nextAlloc;
		}
	}

	ai->m_Allocations = nullptr;
	ai->m_ActiveAllocations = 0;
	ai->m_TotalAllocatedMemory = 0;
	ai->m_TotalAllocations = 0;
	ai->m_IsAlive = false;
}

void memTracerOnRealloc(uint16_t allocatorID, void* ptr, void* ptrNew, size_t sizeNew, const char* file, uint32_t line)
{
	JX_CHECK(s_MemTracer != nullptr, "Memory tracer hasn't been initialized");

	MemTracer* ctx = s_MemTracer;

	JX_CHECK(allocatorID < ctx->m_NumAllocators, "Invalid allocator handle");

#if BX_CONFIG_SUPPORTS_THREADING
	bx::MutexScope ms(*ctx->m_Mutex);
#endif

	AllocatorInfo* ai = &ctx->m_Allocators[allocatorID];
	JX_CHECK(ai->m_IsAlive, "Allocator has been destroyed");

	if (ptr) {
		if (!sizeNew) {
			// free
			JX_CHECK(ptrNew == nullptr, "Freeing with new ptr");

			AllocationInfo* alloc = findAllocation(ai, ptr);
			JX_CHECK(alloc != nullptr, "Allocation not found");
			JX_CHECK(alloc->m_Ptr == ptr, "Allocation pointer mismatch");

			if (alloc->m_Prev) {
				alloc->m_Prev->m_Next = alloc->m_Next;
			}
			if (alloc->m_Next) {
				alloc->m_Next->m_Prev = alloc->m_Prev;
			}
			if (ai->m_Allocations == alloc) {
				ai->m_Allocations = alloc->m_Next;
			}

			JX_CHECK(ai->m_ActiveAllocations > 0, "Allocation counter undeflow");
			ai->m_ActiveAllocations--;

			JX_CHECK(ai->m_TotalAllocatedMemory >= alloc->m_Size, "Allocated memory underflow");
			ai->m_TotalAllocatedMemory -= alloc->m_Size;

			objPoolFree(ctx->m_AllocInfoPool, alloc);
		} else {
			// realloc
			JX_CHECK(ptrNew != nullptr, "Realloc without old pointer");

			AllocationInfo* alloc = findAllocation(ai, ptr);
			JX_CHECK(alloc != nullptr, "Allocation not found");
			JX_CHECK(alloc->m_Ptr == ptr, "Allocation pointer mismatch");

			alloc->m_Filename = file;
			alloc->m_Line = line;
			alloc->m_Ptr = ptrNew;

			JX_CHECK(ai->m_TotalAllocatedMemory >= alloc->m_Size, "Allocated memory underflow");
			ai->m_TotalAllocatedMemory -= alloc->m_Size;
			ai->m_TotalAllocatedMemory += sizeNew;

			alloc->m_Size = sizeNew;
		}
	} else {
		// alloc
		if (ptrNew == nullptr) {
			JX_CHECK(sizeNew == 0, "Allocation without new pointer");
		} else {
			JX_CHECK(sizeNew != 0, "Allocation with zero size");

			AllocationInfo* alloc = (AllocationInfo*)objPoolAlloc(ctx->m_AllocInfoPool);
			alloc->m_Filename = file;
			alloc->m_Line = line;

#if BX_PLATFORM_WINDOWS
			getStackTrace(&alloc->m_StackFrameAddr[0], TRACER_CONFIG_MAX_STACK_FRAMES);
#endif

			alloc->m_Next = ai->m_Allocations;
			alloc->m_Prev = nullptr;
			alloc->m_Ptr = ptrNew;
			alloc->m_Size = sizeNew;

			if (ai->m_Allocations) {
				ai->m_Allocations->m_Prev = alloc;
			}
			ai->m_Allocations = alloc;
			ai->m_ActiveAllocations++;
			ai->m_TotalAllocations++;
			ai->m_TotalAllocatedMemory += sizeNew;
		}
	}
}

static AllocationInfo* findAllocation(AllocatorInfo* ai, const void* ptr)
{
	JX_CHECK(ptr != nullptr, "Invalid allocator");

	AllocationInfo* alloc = ai->m_Allocations;
	while (alloc) {
		if (alloc->m_Ptr == ptr) {
			return alloc;
		}

		alloc = alloc->m_Next;
	}

	return nullptr;
}

#if BX_PLATFORM_WINDOWS
void getStackTrace(uint64_t* stack, uint32_t size)
{
	JX_CHECK(s_MemTracer != nullptr, "Memory tracer hasn't been initialized");
	RtlCaptureStackBackTrace(2, size, (PVOID*)stack, NULL);
}
#endif // BX_PLATFORM_WINDOWS

#if JX_CONFIG_TRACE_ALLOCATIONS
uint32_t memTracerGetNumAllocators()
{
	JX_CHECK(s_MemTracer != nullptr, "Memory tracer hasn't been initialized");
	return s_MemTracer->m_NumAllocators;
}

const char* memTracerGetAllocatorName(uint32_t id)
{
	JX_CHECK(s_MemTracer != nullptr, "Memory tracer hasn't been initialized");
	return s_MemTracer->m_Allocators[id].m_Name;
}

size_t memTracerGetTotalAllocatedMemory(uint32_t id)
{
	JX_CHECK(s_MemTracer != nullptr, "Memory tracer hasn't been initialized");
	return s_MemTracer->m_Allocators[id].m_TotalAllocatedMemory;
}
#endif // JX_CONFIG_TRACE_ALLOCATIONS
}
