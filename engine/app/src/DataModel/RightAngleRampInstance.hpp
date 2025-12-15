

#pragma once

#include "DataModel/PartInstance.hpp"
#include "DataModel/BasicPartInstance.hpp"
#include "Reflection/Reflection.hpp"

namespace Aya
{

extern const char* const sRightAngleRamp;

class RightAngleRampInstance : public DescribedNonCreatable<RightAngleRampInstance, PartInstance, sRightAngleRamp>
{
public:
    RightAngleRampInstance();
    ~RightAngleRampInstance();

    /*override*/ virtual PartType getPartType() const
    {
        return RIGHTANGLERAMP_PART;
    }
};

} // namespace Aya
