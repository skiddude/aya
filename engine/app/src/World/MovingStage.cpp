


#include "World/MovingStage.hpp"
#include "World/SpatialFilter.hpp"
#include "World/Mechanism.hpp"
#include "World/Assembly.hpp"
#include "World/Primitive.hpp"
#include "Debug.hpp"

namespace Aya
{

///////////////////////////////////////////////////////////////////////////////////

#pragma warning(push)
#pragma warning(disable : 4355) // 'this' : used in base member initializer list
MovingStage::MovingStage(IStage* upstream, World* world)
    : IWorldStage(upstream, new SpatialFilter(this, world), world)
{
}
#pragma warning(pop)


MovingStage::~MovingStage() {}

SpatialFilter* MovingStage::getSpatialFilter()
{
    return aya_static_cast<SpatialFilter*>(getDownstreamWS());
}

void assertNotInPipeline2(Assembly* a)
{
    AYAASSERT(!a->inPipeline());
}

bool noAssembliesInPipeline2(Mechanism* m)
{
    Assembly* a = m->getTypedLower<Assembly>();
    a->visitAssemblies(boost::bind(&assertNotInPipeline2, _1));
    return true;
}


void MovingStage::onMechanismAdded(Mechanism* m)
{
    AYAASSERT(noAssembliesInPipeline2(m));

    m->putInStage(this);
    Assembly* root = m->getRootAssembly();

    Time now = Time::nowFast();
    if (root->computeIsGrounded())
    {

        getSpatialFilter()->onFixedAssemblyRootAdded(root);

        for (int i = 0; i < root->numChildren(); ++i)
        {
            Assembly* movingChild = root->getTypedChild<Assembly>(i);
            getSpatialFilter()->onMovingAssemblyRootAdded(movingChild, now);
        }
    }
    else
    {
        getSpatialFilter()->onMovingAssemblyRootAdded(root, now);
    }
}


void MovingStage::onMechanismRemoving(Mechanism* m)
{
    Assembly* root = m->getRootAssembly();

    getSpatialFilter()->onAssemblyRootRemoving(root);

    for (int i = 0; i < root->numChildren(); ++i)
    {
        Assembly* child = root->getTypedChild<Assembly>(i);
        getSpatialFilter()->onAssemblyRootRemoving(child);
    }

    m->removeFromStage(this);

    AYAASSERT(noAssembliesInPipeline2(m));
}




} // namespace Aya
