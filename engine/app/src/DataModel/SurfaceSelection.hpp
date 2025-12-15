

#pragma once

#include "DataModel/Adornment.hpp"
#include "Base/IAdornable.hpp"

namespace Aya
{
extern const char* const sSurfaceSelection;

class SurfaceSelection : public DescribedCreatable<SurfaceSelection, PartAdornment, sSurfaceSelection>
{
public:
    SurfaceSelection();

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // IAdornable
    /*override*/ void render3dAdorn(Adorn* adorn);

    NormalId getSurface() const
    {
        return surfaceId;
    }
    void setSurface(NormalId value);

private:
    NormalId surfaceId;
};

} // namespace Aya
