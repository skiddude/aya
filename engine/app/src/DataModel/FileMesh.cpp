


#include "DataModel/FileMesh.hpp"

using namespace Aya;

const char* const Aya::sFileMesh = "FileMesh";


static Reflection::PropDescriptor<FileMesh, MeshId> desc_meshId("MeshId", category_Data, &FileMesh::getMeshId, &FileMesh::setMeshId);
static Reflection::PropDescriptor<FileMesh, TextureId> desc_textureId("TextureId", category_Data, &FileMesh::getTextureId, &FileMesh::setTextureId);
REFLECTION_END();

FileMesh::FileMesh() {}


void FileMesh::setMeshId(const MeshId& value)
{
    if (meshId != value)
    {
        meshId = value;
        raisePropertyChanged(desc_meshId);
    }
}

void FileMesh::setTextureId(const TextureId& value)
{
    if (textureId != value)
    {
        textureId = value;
        raisePropertyChanged(desc_textureId);
    }
}