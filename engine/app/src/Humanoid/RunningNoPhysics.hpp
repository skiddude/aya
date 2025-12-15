

#pragma once

#include "Humanoid/MovingNoPhysicsBase.hpp"

namespace Aya
{
class Clump;

namespace HUMAN
{

extern const char* const sRunningNoPhysics;

class RunningNoPhysics : public Named<MovingNoPhysicsBase, sRunningNoPhysics>
{
private:
    /*override*/ StateType getStateType() const
    {
        return RUNNING_NO_PHYS;
    }

public:
    RunningNoPhysics(Humanoid* humanoid, StateType priorState);
};

} // namespace HUMAN
} // namespace Aya
