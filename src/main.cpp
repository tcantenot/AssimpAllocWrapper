#include <AssimpWrapper.h>

#include <stdlib.h>
#include <stdio.h>

static unsigned int sAllocCount = 0;
static unsigned int sFreeCount  = 0;

void * MyMalloc(size_t size, size_t alignment, void *)
{
	void * ptr = _aligned_malloc(size, alignment);
	printf("#%u - Allocate: 0x%p (%lluB)\n", sAllocCount, ptr, size);
	sAllocCount += 1;
	return ptr;
}

void MyFree(void * ptr, void *)
{
	//printf("#%u - Free: 0x%p\n", sFreeCount, ptr);
	sFreeCount += 1;
	_aligned_free(ptr);
}

#include <assimp/cimport.h>
#include <assimp/postprocess.h>

int main()
{
	AssimpAllocFunctions allocFunctions;
	allocFunctions.malloc = MyMalloc;
	allocFunctions.free = MyFree;
	allocFunctions.userdata = nullptr;
	AssimpSetAllocFunctions(allocFunctions);

	const aiScene* assimpScene = AssimpImportFile("../assets/cube/cube.obj",
		aiProcess_Triangulate            |
		aiProcess_GenSmoothNormals       |
		aiProcess_GenUVCoords            |
		aiProcess_JoinIdenticalVertices  |
		aiProcess_SortByPType
	);

	AssimpReleaseImport(assimpScene);

	return 0;
}
