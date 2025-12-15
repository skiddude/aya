


#include "World/HumanoidStage.hpp"
#include "World/SleepStage.hpp"
#include "World/SendPhysics.hpp"
#include "World/World.hpp"
#include "World/Assembly.hpp"
#include "World/Joint.hpp"

namespace Aya
{


#pragma warning(push)
#pragma warning(disable : 4355) // 'this' : used in base member initializer list
HumanoidStage::HumanoidStage(IStage* upstream, World* world)
    : IWorldStage(upstream, new SleepStage(this, world), world)
{
}
#pragma warning(pop)


HumanoidStage::~HumanoidStage()
{
    AYAASSERT(movingHumanoidAssemblies.size() == 0);
}


void HumanoidStage::toDynamics(Assembly* a)
{
    aya_static_cast<SleepStage*>(getDownstreamWS())->onAssemblyAdded(a);
}

void HumanoidStage::fromDynamics(Assembly* a)
{
    aya_static_cast<SleepStage*>(getDownstreamWS())->onAssemblyRemoving(a);
}

void HumanoidStage::toHumanoid(Assembly* a)
{
    AYAASSERT(a->getAssemblyState() == Sim::ANCHORED);
    a->setAssemblyState(Sim::AWAKE);

    getWorld()->getSendPhysics()->onMovingAssemblyRootAdded(a);
    bool ok = movingHumanoidAssemblies.insert(a).second;
    AYAASSERT(ok);
}

void HumanoidStage::fromHumanoid(Assembly* a)
{
    int num = movingHumanoidAssemblies.erase(a);
    AYAASSERT(num);
    getWorld()->getSendPhysics()->onMovingAssemblyRootRemoving(a);

    AYAASSERT(a->getAssemblyState() == Sim::AWAKE);
    a->setAssemblyState(Sim::ANCHORED);
}


void HumanoidStage::onAssemblyAdded(Assembly* a)
{
    a->putInStage(this);
    //	a->computeMaxRadius();		// force a compute so Physics Service can use "getLastComputedValue" without an assert because not computed

    if (a->getAssemblyPrimitive()->getEngineType() == Primitive::DYNAMICS_ENGINE)
    {
        toDynamics(a);
    }
    else
    {
        toHumanoid(a);
    }
}


void HumanoidStage::onAssemblyRemoving(Assembly* a)
{
    if (a->inStage(this))
    {
        fromHumanoid(a);
    }
    else
    {
        fromDynamics(a);
    }
    a->removeFromStage(this);
}

} // namespace Aya