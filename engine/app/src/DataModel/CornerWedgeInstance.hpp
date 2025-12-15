

#pragma once

#include "DataModel/PartInstance.hpp"
#include "DataModel/BasicPartInstance.hpp"
#include "Reflection/Reflection.hpp"

namespace Aya
{

extern const char* const sCornerWedge;

class CornerWedgeInstance : public DescribedCreatable<CornerWedgeInstance, PartInstance, sCornerWedge>
{
public:
    CornerWedgeInstance();
    ~CornerWedgeInstance();

    /*override*/ virtual PartType getPartType() const
    {
        return CORNERWEDGE_PART;
    }
};

} // namespace Aya