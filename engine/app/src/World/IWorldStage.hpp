

#pragma once

#include "Kernel/IStage.hpp"
#include "Debug.hpp"
#include "Utility/G3DCore.hpp"

namespace Aya
{

class World;
class Edge;
class Contact;
class Primitive;

class AyaBaseClass IWorldStage : public IStage
{
private:
    World* world;

public:
    typedef enum
    {
        NUM_CONTACTSTAGE_CONTACTS,
        NUM_STEPPING_CONTACTS,
        NUM_TOUCHING_CONTACTS,
        MAX_TREE_DEPTH
    } MetricType;

    IWorldStage(IStage* upstream, IStage* downstream, World* world)
        : IStage(upstream, downstream)
        , world(world)
    {
    }

    IWorldStage* getUpstreamWS()
    {
        return aya_static_cast<IWorldStage*>(getUpstream());
    }
    IWorldStage* getDownstreamWS()
    {
        return aya_static_cast<IWorldStage*>(getDownstream());
    }
    const IWorldStage* getDownstreamWS() const
    {
        return aya_static_cast<const IWorldStage*>(getDownstream());
    }

    World* getWorld()
    {
        return world;
    }

    ////////////////////////////////////////////
    //
    // Calls to DOWNSTREAM stage
    virtual void onEdgeAdded(Edge* e);
    virtual void onEdgeRemoving(Edge* e);

    virtual int getMetric(MetricType metricType)
    {
        AYAASSERT(getDownstreamWS());
        return getDownstreamWS()->getMetric(metricType);
    }
};
} // namespace Aya