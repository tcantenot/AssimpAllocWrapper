#include <AssimpWrapper.h>

#include <assimp/cimport.h>

const aiScene * AssimpImportFile(const char *pFile, unsigned int pFlags)
{
	return aiImportFile(pFile, pFlags);
}

void AssimpReleaseImport(const aiScene * pScene)
{
	aiReleaseImport(pScene);
}
