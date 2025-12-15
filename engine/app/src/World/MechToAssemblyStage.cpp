


#include "World/MechToAssemblyStage.hpp"
#include "World/AssemblyStage.hpp"
#include "World/Mechanism.hpp"
#include "World/Assembly.hpp"
#include "World/Primitive.hpp"

namespace Aya
{

#pragma warning(push)
#pragma warning(disable : 4355) // 'this' : used in base member initializer list
MechToAssemblyStage::MechToAssemblyStage(IStage* upstream, World* world)
    : IWorldStage(upstream, new AssemblyStage(this, world), world)
{
}
#pragma warning(pop)

MechToAssemblyStage::~MechToAssemblyStage() {}


AssemblyStage* MechToAssemblyStage::getAssemblyStage()
{
    return aya_static_cast<AssemblyStage*>(getDownstreamWS());
}



void MechToAssemblyStage::onSimulateAssemblyRootAdded(Assembly* a)
{
    a->putInStage(this);
    getAssemblyStage()->onSimulateAssemblyRootAdded(a);

    a->visitDescendentAssemblies(boost::bind(&AssemblyStage::onSimulateAssemblyDescendentAdded, getAssemblyStage(), _1));
}


void MechToAssemblyStage::onSimulateAssemblyRootRemoving(Assembly* a)
{
    a->visitDescendentAssemblies(boost::bind(&AssemblyStage::onSimulateAssemblyDescendentRemoving, getAssemblyStage(), _1));

    getAssemblyStage()->onSimulateAssemblyRootRemoving(a);
    a->removeFromStage(this); // 2.  out of this stage
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////


void MechToAssemblyStage::onNoSimulateAssemblyRootAdded(Assembly* a)
{
    a->putInStage(this);
    getAssemblyStage()->onNoSimulateAssemblyRootAdded(a);

    a->visitDescendentAssemblies(boost::bind(&AssemblyStage::onNoSimulateAssemblyDescendentAdded, getAssemblyStage(), _1));
}


void MechToAssemblyStage::onNoSimulateAssemblyRootRemoving(Assembly* a)
{
    a->visitDescendentAssemblies(boost::bind(&AssemblyStage::onNoSimulateAssemblyDescendentRemoving, getAssemblyStage(), _1));

    getAssemblyStage()->onNoSimulateAssemblyRootRemoving(a);
    a->removeFromStage(this); // 2.  out of this stage
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

void MechToAssemblyStage::onFixedAssemblyAdded(Assembly* a)
{
    a->putInStage(this);

    getAssemblyStage()->onFixedAssemblyRootAdded(a);
}


void MechToAssemblyStage::onFixedAssemblyRemoving(Assembly* a)
{
    getAssemblyStage()->onFixedAssemblyRootRemoving(a);

    a->removeFromStage(this); // 2.  out of this stage
}

} // namespace Aya
