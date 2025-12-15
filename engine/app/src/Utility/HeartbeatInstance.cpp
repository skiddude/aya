


#include "Utility/HeartbeatInstance.hpp"
#include "Utility/RunStateOwner.hpp"

namespace Aya
{

void HeartbeatInstance::onServiceProviderHeartbeatInstance(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
    AYAASSERT((oldProvider == NULL) != (newProvider == NULL));

    heartbeatConnection.disconnect();

    if (newProvider)
    {
        RunService* runService = ServiceProvider::create<RunService>(newProvider);
        AYAASSERT(runService);
        heartbeatConnection = runService->heartbeatSignal.connect(boost::bind(&HeartbeatInstance::onHeartbeat, this, _1));
    }
}

} // namespace Aya