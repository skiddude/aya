


#include "Humanoid/RunningNoPhysics.hpp"
#include "Humanoid/Humanoid.hpp"

DYNAMIC_FASTFLAG(ClampRunSignalMinSpeed)

namespace Aya
{
namespace HUMAN
{

const char* const sRunningNoPhysics = "RunningNoPhysics";


RunningNoPhysics::RunningNoPhysics(Humanoid* humanoid, StateType priorState)
    : Named<MovingNoPhysicsBase, sRunningNoPhysics>(humanoid, priorState)
{
    if (DFFlag::ClampRunSignalMinSpeed)
    {
        fireMovementSignal(humanoid->runningSignal, getRelativeMovementVelocity().xz().length());
    }
    else
    {
        humanoid->runningSignal(getRelativeMovementVelocity().xz().length());
    }
}



} // namespace HUMAN
} // namespace Aya