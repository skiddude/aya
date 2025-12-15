

#pragma once

#include "DataModel/PartInstance.hpp"
#include "DataModel/BasicPartInstance.hpp"
#include "Reflection/Reflection.hpp"

namespace Aya
{

extern const char* const sParallelRamp;

class ParallelRampInstance : public DescribedNonCreatable<ParallelRampInstance, PartInstance, sParallelRamp>
{
public:
    ParallelRampInstance();
    ~ParallelRampInstance();

    /*override*/ virtual PartType getPartType() const
    {
        return PARALLELRAMP_PART;
    }
};

} // namespace Aya
