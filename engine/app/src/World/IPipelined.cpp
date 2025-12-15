


#include "World/IPipelined.hpp"
#include "Kernel/Kernel.hpp"

namespace Aya
{


Kernel* IPipelined::getKernel() const
{
    AYAASSERT(this->inKernel());
    IStage* answer = getStage(IStage::KERNEL_STAGE);
    return aya_static_cast<Kernel*>(answer);
}

void IPipelined::putInKernel(Kernel* kernel)
{
    putInStage(kernel);
}

void IPipelined::removeFromKernel()
{
    AYAASSERT(currentStage);
    AYAASSERT(currentStage->getStageType() == IStage::KERNEL_STAGE);
    removeFromStage(currentStage);
}



IStage* IPipelined::getStage(IStage::StageType stageType) const
{
    AYAASSERT(currentStage);
    IStage* tryStage = currentStage;
    do
    {
        if (tryStage->getStageType() == stageType)
        {
            return tryStage;
        }
        tryStage = (tryStage->getStageType() > stageType) ? tryStage->getUpstream() : tryStage->getDownstream();
    } while (1);
}

void IPipelined::putInPipeline(IStage* stage)
{
    AYAASSERT(stage);
    AYAASSERT(!currentStage);
    currentStage = stage;
}

void IPipelined::removeFromPipeline(IStage* stage)
{
    AYAASSERT(stage);
    AYAASSERT(currentStage);
    AYAASSERT(currentStage == stage);
    currentStage = NULL;
}


void IPipelined::putInStage(IStage* stage)
{
    AYAASSERT(stage);
    AYAASSERT(currentStage);
    AYAASSERT(stage->getUpstream() == currentStage);
    AYAASSERT(currentStage->getDownstream() == stage);
    currentStage = stage;
}

void IPipelined::removeFromStage(IStage* stage)
{
    AYAASSERT(currentStage);
    AYAASSERT(stage);
    AYAASSERT(stage == currentStage);
    AYAASSERT(stage->getUpstream());
    currentStage = currentStage->getUpstream();
}

} // namespace Aya