

#pragma once

#include "Tree/Instance.hpp"
#include "Utility/RunStateOwner.hpp"
#include "Utility/G3DCore.hpp"

LOGGROUP(ISteppedLifetime)

namespace Aya
{

class IStepped
{
public:
    enum StepType
    {
        StepType_Default,
        StepType_HighPriority,
        StepType_Render,
    };

private:
    StepType stepType;
    Aya::signals::scoped_connection steppedConnection;

protected:
    // call this inside onServiceProvider
    void onServiceProviderIStepped(ServiceProvider* oldProvider, ServiceProvider* newProvider);

    /*implement*/ virtual void onStepped(const Stepped& event) = 0;

    void stopStepping()
    {
        steppedConnection.disconnect();
    }

public:
    IStepped(StepType stepType = StepType_Default)
        : stepType(stepType)
    {
    }
    virtual ~IStepped() {}
};
} // namespace Aya