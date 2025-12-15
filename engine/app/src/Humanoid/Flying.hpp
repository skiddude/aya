

#pragma once

#include "Humanoid/Balancing.hpp"
#include "Utility/Name.hpp"

namespace Aya
{
namespace HUMAN
{

// Flying occurs when there's no ground below you. You have the ability
// to turn around the y-axis, but not much else.
extern const char* const sFlying;

class Flying : public Named<Balancing, sFlying>
{
private:
    /*override*/ StateType getStateType() const
    {
        return FLYING;
    }

protected:
    // Humanoid::State
    /*override*/ void onSimulatorStepImpl(float stepDt);
    /*override*/ void onComputeForceImpl();
    /*override*/ bool enableAutoJump() const
    {
        return false;
    }

public:
    Flying(Humanoid* humanoid, StateType priorState);
};

} // namespace HUMAN
} // namespace Aya
