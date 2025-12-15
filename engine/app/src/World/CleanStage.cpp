


#include "World/CleanStage.hpp"
#include "World/JointStage.hpp"
#include "World/Primitive.hpp"
#include "World/Joint.hpp"
#include "Debug.hpp"

namespace Aya
{

#pragma warning(push)
#pragma warning(disable : 4355) // 'this' : used in base member initializer list
CleanStage::CleanStage(IStage* upstream, World* world)
    : IWorldStage(upstream, new JointStage(this, world), world)
{
}
#pragma warning(pop)


JointStage* CleanStage::getJointStage()
{
    return aya_static_cast<JointStage*>(getDownstream());
}

bool CleanStage::primitivesAreOk(Edge* e)
{
    return (e->getPrimitive(0) && e->getPrimitive(1) && (e->getPrimitive(0) != e->getPrimitive(1)));
}

void CleanStage::onPrimitiveAdded(Primitive* p)
{
    p->putInPipeline(this);

    getJointStage()->onPrimitiveAdded(p); // this should now make some joints ok - their primitives are in the state
}

void CleanStage::onPrimitiveRemoving(Primitive* p)
{
    getJointStage()->onPrimitiveRemoving(p);

    p->removeFromPipeline(this);
}

void CleanStage::onJointPrimitiveNulling(Joint* j, Primitive* nulling)
{
    AYAASSERT(j);

    if (j->downstreamOfStage(this))
    {
        AYAASSERT(primitivesAreOk(j));
        getJointStage()->onEdgeRemoving(j);
    }
    else if (j->inStage(this))
    {
        AYAASSERT(!primitivesAreOk(j));
    }
}

void CleanStage::onJointPrimitiveSet(Joint* j, Primitive* p)
{
    AYAASSERT(!j->downstreamOfStage(this));

    if (primitivesAreOk(j))
    {
        getJointStage()->onEdgeAdded(j);
    }
}

void CleanStage::onEdgeAdded(Edge* e)
{
    e->putInPipeline(this);

    if (primitivesAreOk(e))
    {
        getJointStage()->onEdgeAdded(e);
    }
}


void CleanStage::onEdgeRemoving(Edge* e)
{
    if (e->downstreamOfStage(this))
    {
        AYAASSERT(primitivesAreOk(e));
        getJointStage()->onEdgeRemoving(e);
    }
    else
    {
        AYAASSERT(!primitivesAreOk(e));
    }

    e->removeFromPipeline(this);
}


} // namespace Aya