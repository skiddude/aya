


#include "DataModel/SurfaceSelection.hpp"
#include "DataModel/PartInstance.hpp"
#include "DrawAdorn.hpp"
#include "Draw.hpp"

namespace Aya
{

const char* const sSurfaceSelection = "SurfaceSelection";


static const Reflection::EnumPropDescriptor<SurfaceSelection, NormalId> prop_Surface(
    "TargetSurface", category_Data, &SurfaceSelection::getSurface, &SurfaceSelection::setSurface);
REFLECTION_END();

SurfaceSelection::SurfaceSelection()
    : DescribedCreatable<SurfaceSelection, PartAdornment, sSurfaceSelection>("SurfaceSelection")
    , surfaceId(NORM_X)
{
}

void SurfaceSelection::setSurface(NormalId value)
{
    if (surfaceId != value)
    {
        surfaceId = value;
        raisePropertyChanged(prop_Surface);
    }
}

void SurfaceSelection::render3dAdorn(Adorn* adorn)
{
    if (getVisible())
    {
        if (shared_ptr<Aya::PartInstance> partInstance = adornee.lock())
        {
            DrawAdorn::partSurface(partInstance->getPart(), surfaceId, adorn, color);
        }
    }
}

} // namespace Aya

// Randomized Locations for hackflags
namespace Aya
{
namespace Security
{
unsigned int hackFlag0 = 0;
};
}; // namespace Aya
