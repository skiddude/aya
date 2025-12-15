

#pragma once

#include "World/IWorldStage.hpp"

namespace Aya
{

class Primitive;

class ContactStage : public IWorldStage
{
private:
    class TreeStage* getTreeStage();

public:
    ///////////////////////////////////////////
    // IStage
    ContactStage(IStage* upstream, World* world);

    ~ContactStage() {}

    /*override*/ IStage::StageType getStageType() const
    {
        return IStage::CONTACT_STAGE;
    }

    /*override*/ void onEdgeAdded(Edge* e);
    /*override*/ void onEdgeRemoving(Edge* e);

    void onPrimitiveAdded(Primitive* p);
    void onPrimitiveRemoving(Primitive* p);
};
} // namespace Aya