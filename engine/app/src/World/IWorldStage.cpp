


#include "World/IWorldStage.hpp"
#include "World/Contact.hpp"
#include "World/Edge.hpp"

namespace Aya
{

void IWorldStage::onEdgeAdded(Edge* e)
{
    AYAASSERT(getDownstreamWS());
    e->putInStage(this);
    getDownstreamWS()->onEdgeAdded(e);
}

void IWorldStage::onEdgeRemoving(Edge* e)
{
    AYAASSERT(getDownstreamWS());
    getDownstreamWS()->onEdgeRemoving(e);
    e->removeFromStage(this);
}

} // namespace Aya
