


#include "Humanoid/GettingUp.hpp"
#include "Humanoid/Humanoid.hpp"
#include "World/World.hpp"


namespace Aya
{
namespace HUMAN
{

const char* const sGettingUp = "GettingUp";

GettingUp::GettingUp(Humanoid* humanoid, StateType priorState)
    : Named<Balancing, sGettingUp>(humanoid, priorState, 5000.0f, 300.0f)
{
    if (humanoid && humanoid->getWorld()->getUsingPGSSolver())
    {
        setBalanceP(2250.0f);
        setBalanceD(50.0f);
    }
}


} // namespace HUMAN
} // namespace Aya