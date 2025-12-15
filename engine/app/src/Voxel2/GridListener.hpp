#pragma once

namespace Aya
{
namespace Voxel2
{

class Region;

class GridListener
{
public:
    virtual ~GridListener() {}

    virtual void onTerrainRegionChanged(const Region& region) = 0;
};

} // namespace Voxel2
} // namespace Aya
