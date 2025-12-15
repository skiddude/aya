

#pragma once

#include "World/IWorldStage.hpp"
#include "World/Assembly.hpp"
#include "Utility/BiMultiMap.hpp"

namespace Aya
{
class Assembly;

class EdgeBuffer : public IWorldStage
{
private:
    // DEBUG ONLY
    typedef Aya::BiMultiMap<Assembly*, Edge*> AssemblyEdgeMap; // find incomplete Joints by primitive
    AssemblyEdgeMap assemblyEdges;

    bool debugPushEdgeToDownstream(Edge* e);
    bool debugRemoveEdgeFromDownstream(Edge* e);
    bool debugAddAssembly(Assembly* a);
    bool debugRemoveAssembly(Assembly* a);

    bool assemblyIsHere(Assembly* a);

    void assemblyPrimitiveAdded(Primitive* p);
    void assemblyPrimitiveRemoved(Primitive* p);

    void pushEdgeIfOk(Edge* e);
    bool pushSpringOk(Edge* e);
    bool pushKinematicOk(Edge* e);
    void removeEdgeIfDownstream(Edge* e);

protected:
    void afterAssemblyAdded(Assembly* a);
    void beforeAssemblyRemoving(Assembly* a);

    EdgeBuffer(IStage* upstream, IStage* downstream, World* world)
        : IWorldStage(upstream, downstream, world)
    {
    }

    virtual ~EdgeBuffer();

    /*override*/ void onEdgeAdded(Edge* e);
    /*override*/ void onEdgeRemoving(Edge* e);
};
} // namespace Aya
