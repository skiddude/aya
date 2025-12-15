


#include "Humanoid/StrafingNoPhysics.hpp"
#include "Humanoid/Humanoid.hpp"

namespace Aya
{
namespace HUMAN
{

const char* const sStrafingNoPhysics = "StrafingNoPhysics";


StrafingNoPhysics::StrafingNoPhysics(Humanoid* humanoid, StateType priorState)
    : Named<MovingNoPhysicsBase, sStrafingNoPhysics>(humanoid, priorState)
{
}


} // namespace HUMAN
} // namespace Aya