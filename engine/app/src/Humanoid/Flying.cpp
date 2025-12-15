


#include "Humanoid/Flying.hpp"

#include "Humanoid/Humanoid.hpp"
// #include "World/Controller.hpp"
#include "Kernel/Body.hpp"

namespace Aya
{

namespace HUMAN
{


const char* const sFlying = "Flying";

Flying::Flying(Humanoid* humanoid, StateType priorState)
    : Named<Balancing, sFlying>(humanoid, priorState)
{
    setBalanceP(5000.0f);
}


void Flying::onSimulatorStepImpl(float stepDt)
{
    // to implement
}

void Flying::onComputeForceImpl()
{
    // to implement
}

} // namespace HUMAN
} // namespace Aya