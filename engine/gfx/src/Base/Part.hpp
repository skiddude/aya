

#pragma once

#include "Utility/SurfaceType.hpp"
#include "Utility/Vector6.hpp"
#include "Vector3.hpp"
#include "Color4.hpp"
#include "CoordinateFrame.hpp"

// Simple description of a part suitable for drawing, etc.  Build Instance on top of this.
// Low level.

namespace Aya
{

enum PartType
{
    BALL_PART = 0,
    BLOCK_PART,
    CYLINDER_PART,
    TRUSS_PART,
    WEDGE_PART,
    PRISM_PART,
    PYRAMID_PART,
    PARALLELRAMP_PART,
    RIGHTANGLERAMP_PART,
    CORNERWEDGE_PART,
    MEGACLUSTER_PART,
    OPERATION_PART,
    MESH_PART
};

class Part
{
public:
    // alpha order for simplification on dialogs

    PartType type; // hash code hashes this block of data
    G3D::Vector3 gridSize;
    G3D::Color4 color;
    Vector6<SurfaceType> surfaceType;
    G3D::CoordinateFrame coordinateFrame;

    Part() {}

    Part(PartType _type, const G3D::Vector3& _gridSize, const G3D::Color4 _color, const G3D::CoordinateFrame& c)
        : type(_type)
        , gridSize(_gridSize)
        , color(_color)
        , surfaceType(NO_SURFACE)
        , coordinateFrame(c)
    {
    }

    Part(PartType type, const G3D::Vector3& gridSize, const G3D::Color4 color, const Vector6<SurfaceType>& surfaceType, const G3D::CoordinateFrame& c)
        : type(type)
        , gridSize(gridSize)
        , color(color)
        , surfaceType(surfaceType)
        , coordinateFrame(c)
    {
    }
};

} // namespace Aya
