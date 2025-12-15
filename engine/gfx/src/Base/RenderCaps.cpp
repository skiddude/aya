#include "Base/RenderCaps.hpp"
#include "FastLog.hpp"

namespace Aya
{

RenderCaps::RenderCaps(std::string gfxCardName, size_t vidMemSize)
    : gfxCardName(gfxCardName)
    , vidMemSize(vidMemSize)
    , texturePowerOf2Only(false)
    , supportsGBuffer(false)
    , skinningBoneCount(0)
{
}

} // namespace Aya
