

#pragma once

#include "signal.hpp"
#include "Utility/G3DCore.hpp"

// hook up by overriding onServiceProvider call in this pattern:
//
//	/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider) {
//		Super::onServiceProvider(oldProvider, newProvider);
//		onServiceProviderHeartbeatInstance(oldProvider, newProvider);		// hooks up heartbeat
//	}
//

namespace Aya
{
class Heartbeat;
class ServiceProvider;

class HeartbeatInstance
{
private:
    Aya::signals::scoped_connection heartbeatConnection;

protected:
    // call this inside onServiceProvider
    void onServiceProviderHeartbeatInstance(ServiceProvider* oldProvider, ServiceProvider* newProvider);

    /*implement*/ virtual void onHeartbeat(const Heartbeat& event) = 0;

public:
    HeartbeatInstance() {}
    virtual ~HeartbeatInstance() {}
};
} // namespace Aya
