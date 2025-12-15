

#pragma once

#include "Utility/G3DCore.hpp"

namespace Aya
{

class Part;

class HitTest
{
private:
    // hitTests
    static bool hitTestBox(const Part& part, RbxRay& rayInPartCoords, Vector3& hitPointInPartCoords, float gridToReal);
    static bool hitTestBall(const Part& part, RbxRay& rayInPartCoords, Vector3& hitPointInPartCoords, float gridToReal);
    static bool hitTestCylinder(const Part& part, RbxRay& rayInPartCoords, Vector3& hitPointInPartCoords, float gridToReal);

public:
    static bool hitTest(const Part& part, RbxRay& rayInPartCoords, Vector3& hitPointInPartCoords, float gridToReal);
};

} // namespace Aya
