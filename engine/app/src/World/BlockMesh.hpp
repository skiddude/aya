#pragma once

/*
        Utility class - holds Block Meshes of same size for use by Geometry Pool.
*/

#include "World/Mesh.hpp"
#include "Memory.hpp"

namespace Aya
{

namespace POLY
{

class BlockMesh : public Allocator<BlockMesh>
{
    Mesh mesh;

public:
    BlockMesh(const Vector3& size)
    {
        mesh.makeBlock(size);
    }
    const Mesh* getMesh() const
    {
        return &mesh;
    }
};

} // namespace POLY
} // namespace Aya
