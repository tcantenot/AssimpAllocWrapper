#include <AssimpWrapper.h>

#include <new> // std::bad_alloc
#include <cstdlib>
#include <cstdint>

// Note: could be thread_local if needed
static AssimpAllocFunctions sAllocFunctions;

void AssimpSetAllocFunctions(AssimpAllocFunctions allocFunctions)
{
	sAllocFunctions = allocFunctions;
}

static inline bool AssimpValidAllocFunctions()
{
	return sAllocFunctions.malloc && sAllocFunctions.free;
}

namespace PtrUtils {

inline void * AlignForward(void * ptr, uintptr_t alignment)
{
	return(void *)((reinterpret_cast<uintptr_t>(ptr) +(alignment - 1)) & ~(alignment - 1));
}

inline void * Add(void * ptr, ptrdiff_t offset)
{
	return(void *)(reinterpret_cast<uintptr_t>(ptr) + offset);
}

inline void * Subtract(void * ptr, ptrdiff_t offset)
{
	return(void *)(reinterpret_cast<uintptr_t>(ptr) - offset);
}

}

#define K_DEFAULT_ALIGNMENT 16
#define K_ALLOC_IDENTIFIER  0xA0B1C2D3u

struct AllocHeader
{
	uint32_t identifier; // Identifier used to detect the allocations performed by the "default" hook
	uint32_t alignment;  // Note: can be used to implement realloc(not used here), otherwise used as padding
	void * allocPtr;
};

inline AllocHeader * GetAllocHeader(void * ptr)
{
	return reinterpret_cast<AllocHeader*>(PtrUtils::Subtract(ptr, sizeof(AllocHeader)));
}

#define K_ENABLE_DEBUG_LOG 0

#if K_ENABLE_DEBUG_LOG
	#include <stdarg.h>
	#include <stdio.h>

	#if defined(_WIN64)
	#define _AMD64_
	#elif defined(_WIN32)
	#define _X86_
	#endif
	#include <debugapi.h> // OutputDebugStringA

	inline void DebugLog(const char * fmt, ...)
	{
		char buffer[256];

		va_list args;
		va_start(args, fmt);
		vsnprintf(buffer, sizeof(buffer), fmt, args);
		va_end(args);

		printf(buffer);
		OutputDebugStringA(buffer);
	}

	#define K_DEBUG_LOG(...) DebugLog(__VA_ARGS__)
#else
	// https://twitter.com/molecularmusing/status/1628709653759635459?t=8wIpgwb2u0X_5Kp7noIM7A&s=09
	#define K_NOOP(...) (void)sizeof(0, __VA_ARGS__)
	#define K_DEBUG_LOG(...) K_NOOP(__VA_ARGS__)
#endif

static inline void * DefaultMallocAligned(size_t size, size_t alignment)
{
	const size_t extraSize = sizeof(AllocHeader) + alignment - 1; // Allocate space for header and alignment
	const size_t allocSize = size + extraSize;
	if(void * allocPtr = malloc(allocSize))
	{
		void * alignedPtr = PtrUtils::AlignForward(PtrUtils::Add(allocPtr, sizeof(AllocHeader)), alignment);

		AllocHeader * header = GetAllocHeader(alignedPtr);
		header->allocPtr = allocPtr;
		header->alignment =(uint32_t)alignment;
		header->identifier = K_ALLOC_IDENTIFIER;

		K_DEBUG_LOG("Non-hooked allocation: 0x%p (block: 0x%p, size: %llu, alignment: %llu)\n", alignedPtr, allocPtr, size, alignment);

		return alignedPtr;
	}
	return nullptr;
}

static inline bool DefaultFreeAligned(void * ptr)
{
	if(!ptr)
		return true;

	AllocHeader * header = GetAllocHeader(ptr);
	if(header->identifier == K_ALLOC_IDENTIFIER)
	{
		K_DEBUG_LOG("Non-hooked deallocation: 0x%p (block: 0x%p)\n", ptr, header->allocPtr);
		free(header->allocPtr);
		return true;
	}
	else
	{
		// Hooked alloc, return false to signal we did not process it
		return false;
	}
}

static inline void * AssimpWrapperMalloc(std::size_t size, std::size_t alignment = K_DEFAULT_ALIGNMENT)
{
	if(AssimpValidAllocFunctions())
		return sAllocFunctions.malloc(size, alignment, sAllocFunctions.userdata);
	return DefaultMallocAligned(size, alignment);
}

static inline void AssimpWrapperFree(void * ptr)
{
	if(!DefaultFreeAligned(ptr))
	{
		if(AssimpValidAllocFunctions())
			sAllocFunctions.free(ptr, sAllocFunctions.userdata);
	}
}


// Note: here we assume at least C++17 for the new/delete operators
// https://en.cppreference.com/w/cpp/memory/new/operator_new
// https://en.cppreference.com/w/cpp/memory/new/operator_delete
// https://stackoverflow.com/questions/5802005/does-dll-new-delete-override-the-user-code-new-delete

// Throwing new
void * operator new(std::size_t size)
{
	if(void * ptr = AssimpWrapperMalloc(size))
		return ptr;
	throw std::bad_alloc();
}

// Throwing new[]
void * operator new[](std::size_t size)
{
	if(void * ptr = AssimpWrapperMalloc(size))
		return ptr;
	throw std::bad_alloc();
}

// Throwing new aligned
void * operator new(std::size_t size, std::align_val_t alignment)
{
	if(void * ptr = AssimpWrapperMalloc(size, static_cast<std::size_t>(alignment)))
		return ptr;
	throw std::bad_alloc();
}

// Throwing new[] aligned
void * operator new[](std::size_t size, std::align_val_t alignment)
{
	if(void * ptr = AssimpWrapperMalloc(size, static_cast<std::size_t>(alignment)))
		return ptr;
	throw std::bad_alloc();
}

// Non-throwing new
void * operator new(std::size_t size, const std::nothrow_t&) noexcept
{
	return AssimpWrapperMalloc(size);
}

// Non-throwing new[]
void * operator new[](std::size_t size, const std::nothrow_t&) noexcept
{
	return AssimpWrapperMalloc(size);
}

// Non-throwing new aligned
void * operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
	return AssimpWrapperMalloc(size, static_cast<std::size_t>(alignment));
}

// Non-throwing new[] aligned
void * operator new[](std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
	return AssimpWrapperMalloc(size, static_cast<std::size_t>(alignment));
}

// "Throwing" delete
void operator delete(void * ptr) noexcept
{
	AssimpWrapperFree(ptr);
}

// "Throwing" delete[]
void operator delete[](void * ptr) noexcept
{
	AssimpWrapperFree(ptr);
}

// "Throwing" delete aligned
void operator delete(void * ptr, std::align_val_t) noexcept
{
	AssimpWrapperFree(ptr);
}

// "Throwing" delete[] aligned
void operator delete[](void * ptr, std::align_val_t) noexcept
{
	AssimpWrapperFree(ptr);
}

// "Throwing" delete with size
void operator delete(void * ptr, std::size_t) noexcept
{
	AssimpWrapperFree(ptr);
}

// "Throwing" delete[] with size
void operator delete[](void * ptr, std::size_t) noexcept
{
	AssimpWrapperFree(ptr);
}

// "Throwing" delete aligned with size
void operator delete(void * ptr, std::size_t, std::align_val_t) noexcept
{
	AssimpWrapperFree(ptr);
}

// "Throwing" delete[] aligned with size
void operator delete[](void * ptr, std::size_t, std::align_val_t) noexcept
{
	AssimpWrapperFree(ptr);
}

// Non-throwing delete
void operator delete(void * ptr, const std::nothrow_t&) noexcept
{
	AssimpWrapperFree(ptr);
}

// Non-throwing delete[]
void operator delete[](void * ptr, const std::nothrow_t&) noexcept
{
	AssimpWrapperFree(ptr);
}

// Non-throwing delete aligned
void operator delete(void * ptr, std::align_val_t, const std::nothrow_t&) noexcept
{
	AssimpWrapperFree(ptr);
}

// Non-throwing delete[] aligned
void operator delete[](void * ptr, std::align_val_t, const std::nothrow_t&) noexcept
{
	AssimpWrapperFree(ptr);
}
