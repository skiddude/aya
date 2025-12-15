

#pragma once

#include "World/IWorldStage.hpp"


namespace Aya
{

class Primitive;

class EdgeStage : public IWorldStage
{
private:
    typedef IWorldStage Super;
    class ContactStage* getContactStage();

public:
    ///////////////////////////////////////////
    // IStage
    EdgeStage(IStage* upstream, World* world);

    ~EdgeStage() {}

    /*override*/ IStage::StageType getStageType() const
    {
        return IStage::EDGE_STAGE;
    }

    /*override*/ void onEdgeAdded(Edge* e);
    /*override*/ void onEdgeRemoving(Edge* e);

    void onPrimitiveAdded(Primitive* p);
    void onPrimitiveRemoving(Primitive* p);
};
} // namespace Aya