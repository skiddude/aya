

#pragma once

#include "Tree/Instance.hpp"
#include "Utility/RunStateOwner.hpp"
#include "Base/IAdornable.hpp"
#include "Base/Adorn.hpp"
#include "DrawAdorn.hpp"
#include "DataModel/Effect.hpp"

namespace Aya
{

class PartInstance;

static const float largeSize = 1.1f;

extern const char* const sForceField;
class ForceField
    : public DescribedCreatable<ForceField, Instance, sForceField>
    , public IAdornable
    , public Effect
{
private:
    // typedef DescribedCreatable<ForceField, Instance, sForceField> Super;

    Time startTime;
    int cycle;
    int invertCycle;
    shared_ptr<Instance> torso;
    bool visible;

    // Instance
    /*override*/ bool askSetParent(const Instance* instance) const;

    // IAdornable
    /*override*/ bool shouldRender2d() const
    {
        return visible;
    }
    /*override*/ bool shouldRender3dAdorn() const
    {
        return visible;
    }
    /*override*/ bool shouldRender3d() const
    {
        return visible;
    }
    /*override*/ bool shouldRender3dSortedAdorn() const
    {
        return visible;
    }
    /*override*/ void render3dAdorn(Adorn* adorn);

public:
    ForceField();
    virtual ~ForceField() {}
    static Reflection::PropDescriptor<ForceField, bool> prop_Visible;

    static bool partInForceField(PartInstance* part);
    static int cycles()
    {
        return 60;
    }

    bool getVisible() const
    {
        return visible;
    }
    void setVisible(bool value);
};
} // namespace Aya