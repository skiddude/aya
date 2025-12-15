

#pragma once

#include "BevelMesh.hpp"

namespace Aya
{
extern const char* const sBlockMesh;
class BlockMesh : public DescribedCreatable<BlockMesh, BevelMesh, sBlockMesh>
{
public:
    BlockMesh() {}
};
} // namespace Aya