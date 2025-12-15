

#pragma once

#include "World/IWorldStage.hpp"

namespace Aya
{

class Mechanism;
class Assembly;

class MovingStage : public IWorldStage
{
private:
    class SpatialFilter* getSpatialFilter();

public:
    ///////////////////////////////////////////
    // IStage
    MovingStage(IStage* upstream, World* world);

    ~MovingStage();

    /*override*/ IStage::StageType getStageType() const
    {
        return IStage::MOVING_STAGE;
    }

    /////////////////////////////////////////////
    // From the Joint Stage
    //
    void onMechanismAdded(Mechanism* a);
    void onMechanismRemoving(Mechanism* a);
};
} // namespace Aya
