


#include "Utility/SteppedInstance.hpp"
#include "FastLog.hpp"

namespace Aya
{

void IStepped::onServiceProviderIStepped(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
    AYAASSERT((oldProvider == NULL) != (newProvider == NULL));

    steppedConnection.disconnect();

    if (newProvider)
    {
        RunService* runService = ServiceProvider::create<RunService>(newProvider);
        AYAASSERT(runService);
        FASTLOG1(FLog::ISteppedLifetime, "Subscribed to IStepped", this);

        switch (stepType)
        {
        case StepType_Default:
            steppedConnection = runService->steppedSignal.connect(boost::bind(&IStepped::onStepped, this, _1));
            break;

        case StepType_HighPriority:
            steppedConnection = runService->highPrioritySteppedSignal.connect(boost::bind(&IStepped::onStepped, this, _1));
            break;

        case StepType_Render:
            steppedConnection = runService->renderSteppedSignal.connect(boost::bind(&IStepped::onStepped, this, _1));
            break;

        default:
            AYAASSERT(false);
            break;
        }
    }
}
} // namespace Aya