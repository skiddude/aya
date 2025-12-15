

#pragma once

#include "Tree/Instance.hpp"
#include "Utility/ContentId.hpp"
#include "Utility/MeshId.hpp"
#include "Utility/TextureId.hpp"
#include "Utility/G3DCore.hpp"
#include "FileMesh.hpp"

namespace Aya
{
extern const char* const sSpecialShape;
class SpecialShape : public DescribedCreatable<SpecialShape, FileMesh, sSpecialShape>
{
private:
    typedef DescribedCreatable<SpecialShape, FileMesh, sSpecialShape> Super;

public:
    // Warning - these values determine XML read/write.  Only append, never change
    typedef enum
    {
        HEAD_MESH = 0,
        TORSO_MESH = 1,
        WEDGE_MESH = 2,
        SPHERE_MESH = 3,
        CYLINDER_MESH = 4,
        FILE_MESH = 5,
        BRICK_MESH = 6,
        PRISM_MESH = 7,
        PYRAMID_MESH = 8,
        PARALLELRAMP_MESH = 9,
        RIGHTANGLERAMP_MESH = 10,
        CORNERWEDGE_MESH = 11
    } MeshType;

private:
    MeshType meshType;

public:
    SpecialShape();

    MeshType getMeshType() const
    {
        return meshType;
    }
    void setMeshType(MeshType value);

    /*** Override ***/ void setMeshId(const MeshId& value);
    /*** Override ***/ void setTextureId(const TextureId& value);
};
} // namespace Aya
