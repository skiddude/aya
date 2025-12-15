

#pragma once

#include "Humanoid/Balancing.hpp"
#include "Utility/Name.hpp"

namespace Aya
{
namespace HUMAN
{

// Flying occurs when there's no ground below you. You have the ability
// to turn around the y-axis, but not much else.
extern const char* const sRagdoll;

class Ragdoll : public Named<HumanoidState, sRagdoll>
{
private:
    typedef Named<HumanoidState, sRagdoll> Super;
    /*override*/ StateType getStateType() const
    {
        return RAGDOLL;
    }
    /*override*/ void onStepImpl();
    /*override*/ void onComputeForceImpl() {}
    /*override*/ bool enableAutoJump() const
    {
        return false;
    }

public:
    Ragdoll(Humanoid* humanoid, StateType priorState);
    ~Ragdoll();
};

} // namespace HUMAN
} // namespace Aya
