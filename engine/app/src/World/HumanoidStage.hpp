

#pragma once

#include "World/IWorldStage.hpp"

namespace Aya
{

class Assembly;

class HumanoidStage : public IWorldStage
{
private:
    std::set<Assembly*> movingHumanoidAssemblies;
    void toDynamics(Assembly* a);
    void toHumanoid(Assembly* a);
    void fromDynamics(Assembly* a);
    void fromHumanoid(Assembly* a);


public:
    HumanoidStage(IStage* upstream, World* world);

    ~HumanoidStage();

    /*override*/ IStage::StageType getStageType() const
    {
        return IStage::HUMANOID_STAGE;
    }

    void onAssemblyAdded(Assembly* assembly);
    void onAssemblyRemoving(Assembly* assembly);

    const std::set<Assembly*>& getMovingHumanoidAssemblies()
    {
        return movingHumanoidAssemblies;
    }
};
} // namespace Aya
