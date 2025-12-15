#pragma once

#include "Utility/G3DCore.hpp"
#include "Utility/ExtentsInt32.hpp"
#include "Utility/Extents.hpp"
#include "Debug.hpp"

// #define _AYA_DEBUGGING_SPATIAL_HASH

#ifdef _AYA_DEBUGGING_SPATIAL_HASH
#define AYAASSERT_SPATIAL_HASH(expr) AYAASSERT(expr)
const bool assertingSpatialHash = true;
#else
#define AYAASSERT_SPATIAL_HASH(expr) ((void)0)
const bool assertingSpatialHash = false;
#endif

namespace Aya
{

/** use the SpatialHash with classes that contain these members:
 * basic Primitive must implement:
 */
class BasicSpatialHashPrimitive
{
private:
    ExtentsInt32 oldSpatialExtents;
    int spatialNodeLevel;

#ifdef _AYA_DEBUGGING_SPATIAL_HASH
    void* spatialNodes;
    int spatialNodeCount;
#endif

public:
    BasicSpatialHashPrimitive()
        : spatialNodeLevel(-1)
#ifdef _AYA_DEBUGGING_SPATIAL_HASH
        , spatialNodes(0)
        , spatialNodeCount(0)
#endif
    {};

    ~BasicSpatialHashPrimitive()
    {
        AYAASSERT(spatialNodeLevel == -1); //
        AYAASSERT_SPATIAL_HASH(spatialNodes == NULL);
        AYAASSERT_SPATIAL_HASH(spatialNodeCount == 0);
        spatialNodeLevel = -2;
    }

    bool IsInSpatialHash()
    {
        return spatialNodeLevel > -1;
    }

    // The remaining functions are used by the SpatialHash<> implementation
    int getSpatialNodeLevel() const
    {
        AYAASSERT(spatialNodeLevel >= -1);
        return spatialNodeLevel;
    }
    void setSpatialNodeLevel(int value)
    {
        spatialNodeLevel = value;
    }

    const ExtentsInt32& getOldSpatialExtents() const
    {
        return oldSpatialExtents;
    }
    void setOldSpatialExtents(const ExtentsInt32& value)
    {
        oldSpatialExtents = value;
    }

    const Vector3int32& getOldSpatialMin()
    {
        return oldSpatialExtents.low;
    }
    const Vector3int32& getOldSpatialMax()
    {
        return oldSpatialExtents.high;
    }
};

} // namespace Aya
