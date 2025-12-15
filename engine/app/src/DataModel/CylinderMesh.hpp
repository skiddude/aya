

#pragma once

#include "BevelMesh.hpp"

namespace Aya
{
extern const char* const sCylinderMesh;
class CylinderMesh : public DescribedCreatable<CylinderMesh, BevelMesh, sCylinderMesh>
{
public:
    CylinderMesh() {}
};
} // namespace Aya