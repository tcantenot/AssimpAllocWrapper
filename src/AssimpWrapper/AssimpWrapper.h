#pragma once

#ifdef _WIN32
#    ifdef ASSIMP_WRAPPER_EXPORT
#        define ASSIMP_WRAPPER_API __declspec(dllexport)
#    else
#        define ASSIMP_WRAPPER_API __declspec(dllimport)
#    endif
#elif
#    define ASSIMP_WRAPPER_API
#endif

struct AssimpAllocFunctions
{
	void*(*malloc)(size_t size, size_t alignment, void * userdata);
	void (*free)(void * ptr, void * userdata);
	void * userdata;
};

ASSIMP_WRAPPER_API void AssimpSetAllocFunctions(AssimpAllocFunctions allocFunctions);

// For the PoC, only aiImportFile and aiReleaseImport are wrapped but all API functions that can allocate/free should be added
struct aiScene;
ASSIMP_WRAPPER_API const aiScene * AssimpImportFile(const char *pFile, unsigned int pFlags);
ASSIMP_WRAPPER_API void AssimpReleaseImport(const aiScene * pScene);
