#pragma once

#include "DataModel/DataModelJob.hpp"

#include "Utility/SystemAddress.hpp"

#include "Utility/G3DCore.hpp"

#include "Utility/Region2.hpp"

#include <map>

namespace Aya
{

class PartInstance;
class Mechanism;
class DataModel;

namespace Network
{

class Server;
class ServerReplicator;

class NetworkOwnerJob : public DataModelJob
{
private:
    boost::weak_ptr<DataModel> dataModel;
    float networkOwnerRate;

    class ClientLocation
    {
    public:
        Region2::WeightedPoint clientPoint;
        ServerReplicator* clientProxy;
        const Mechanism* characterMechanism;
        bool overloaded;

        ClientLocation(const Region2::WeightedPoint& clientPoint, ServerReplicator* clientProxy, const Mechanism* characterMechanism)
            : clientPoint(clientPoint)
            , clientProxy(clientProxy)
            , characterMechanism(characterMechanism)
            , overloaded(false)
        {
        }
    };

    typedef std::map<Aya::SystemAddress, ClientLocation> ClientMap;
    typedef std::pair<Aya::SystemAddress, ClientLocation> ClientMapPair;
    typedef ClientMap::const_iterator ClientMapConstIt;
    typedef ClientMap::iterator ClientMapIt;

    ClientMap clientMap;

    void updatePlayerLocations(Server* server);
    void updateNetworkOwner(PartInstance* part);
    ClientMapConstIt findClosestClientToPart(PartInstance* part);
    ClientMapConstIt findClosestClient(const Vector2& testLocation);

    bool clientCanSimulate(PartInstance* part, ClientMapConstIt testLocation);
    bool isClientCharacterMechanism(PartInstance* part, ClientMapConstIt testLocation);
    bool switchOwners(PartInstance* part, ClientMapConstIt currentOwner, ClientMapConstIt possibleNewOwner);
    static void resetNetworkOwner(PartInstance* part, const Aya::SystemAddress value);

    // Job overrides
    /*override*/ virtual Error error(const Stats& stats);
    /*override*/ Time::Interval sleepTime(const Stats& stats);
    /*override*/ virtual TaskScheduler::StepResult stepDataModelJob(const Stats& stats);


public:
    NetworkOwnerJob(shared_ptr<DataModel> dataModel);
    void invalidateProjectileOwnership(Aya::SystemAddress addr);
};

} // namespace Network
} // namespace Aya
