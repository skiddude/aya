


#include "Humanoid/Seated.hpp"
#include "Humanoid/Humanoid.hpp"

namespace Aya
{
namespace HUMAN
{

const char* const sSeated = "Seated";

Seated::Seated(Humanoid* humanoid, StateType priorState)
    : Named<HumanoidState, sSeated>(humanoid, priorState)
{
    setCanThrottleState(true);
}

// note - constructor always sets to false

Seated::~Seated()
{
    getHumanoid()->setSeatPart(NULL);
    getHumanoid()->setSit(false);
    getHumanoid()->doneSittingSignal();
}

///////////////////////////////////////////////

const char* const sPlatformStanding = "PlatformStanding";

PlatformStanding::PlatformStanding(Humanoid* humanoid, StateType priorState)
    : Named<HumanoidState, sPlatformStanding>(humanoid, priorState)
{
    setCanThrottleState(true);
}

// note - constructor always sets to false

PlatformStanding::~PlatformStanding()
{
    getHumanoid()->setPlatformStanding(false);
    getHumanoid()->donePlatformStandingSignal();
}




} // namespace HUMAN
} // namespace Aya
