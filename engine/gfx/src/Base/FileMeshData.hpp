#pragma once

#include "MeshFileStructs.hpp"

#include "Utility/Object.hpp"
#include "Utility/G3DCore.hpp"

#include <vector>

namespace Aya
{
struct FileMeshData
{
    std::vector<FileMeshVertexNormalTexture3d> vnts;
    std::vector<FileMeshFace> faces;
    AABox aabb;
};

shared_ptr<FileMeshData> ReadFileMesh(const std::string& data);

// writes the newest version always.
// remember: set ostream to binary!
void WriteFileMesh(std::ostream& f, const FileMeshData& mesh);
} // namespace Aya