

#pragma once

#include "Utility/G3DCore.hpp"
#include "Utility/NormalId.hpp"
#include "HandleType.hpp"

namespace Aya
{

class Extents;

class HandleHitTest
{
public:
    static bool hitTestHandleLocal(const Extents& localExtents, const CoordinateFrame& location, HandleType handleType, const Ray& gridRay,
        Vector3& hitPointWorld, NormalId& localNormalId, const int normalIdMask = NORM_ALL_MASK);

    static bool hitTestHandleWorld(const Extents& worldExtents, HandleType handleType, const Ray& gridRay, Vector3& hitPointWorld,
        NormalId& worldNormalId, const int normalIdMask = NORM_ALL_MASK);

    static bool hitTestMoveHandleWorld(
        const Extents& worldExtents, const RbxRay& gridRay, Vector3& hitPointWorld, NormalId& worldNormalId, const int normalIdMask = NORM_ALL_MASK);
};
} // namespace Aya