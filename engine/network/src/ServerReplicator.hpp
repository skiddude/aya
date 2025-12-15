

#pragma once

#include "Tree/Service.hpp"

#include "Utility/StandardOut.hpp"

#include "Player.hpp"

#include "Peer.hpp"
#include "Replicator.hpp"
#include "NetworkFilter.hpp"
#include "PropertySynchronization.hpp"
#include "DataModel/PartInstance.hpp"

#include "DataModel/Value.hpp"


#include "RakNet/PluginInterface2.hpp"
#include "RakNet/GetTime.hpp"

#include "boost/thread/thread.hpp"
#include <boost/thread/condition.hpp>

#include "Security/FuzzyTokens.hpp"

#ifdef AYA_SERVER
DYNAMIC_FASTSTRING(US30605p1)
#endif

namespace Aya
{
namespace Network
{

class Server;
class NetworkFilter;

enum PlaceAuthenticationState
{
    PlaceAuthenticationState_Init,
    PlaceAuthenticationState_Requesting,
    PlaceAuthenticationState_Authenticated,
    PlaceAuthenticationState_Denied,
    PlaceAuthenticationState_DisconnectingClient
};

extern const char* const sServerReplicator;
class ServerReplicator
    : public Aya::DescribedNonCreatable<ServerReplicator, Replicator, sServerReplicator, Reflection::ClassDescriptor::INTERNAL_LOCAL>
{
    typedef Aya::DescribedNonCreatable<ServerReplicator, Replicator, sServerReplicator, Reflection::ClassDescriptor::INTERNAL_LOCAL> Super;
    PropSync::Master propSync;
    class ServerStatsItem;
    friend class ServerStatsItem;
    boost::scoped_ptr<NetworkFilter> basicFilter;
    bool acceptsTerrainChanges;

    DescribedBase* lightingService;
    Reflection::PropertyDescriptor* globalShadowsDescriptor;
    Reflection::PropertyDescriptor* outdoorAmbientDescriptor;
    Reflection::PropertyDescriptor* outlinesDescriptor;

    std::string initialSpawnName;

    Instance* pendingCharaterRequest;
    Time pendingCharacterRequestStartTime;

    friend class Replicator::StreamJob;

    shared_ptr<TaskScheduler::Job> sendStatsJob;


public: // variables
    int numPartsOwned;
    Aya::signal<void(int, bool, int)> remoteTicketProcessedSignal;
    int remoteProtocolVersion;

protected:
    Aya::signal<void()> placeAutenticatedSignal;

    // metrics
    Time startTime;

    // TODO: Create a Property for this field:
    shared_ptr<Player> remotePlayer;
    Server* const server;
    boost::scoped_ptr<boost::thread> placeAuthenticationThread;

    PlaceAuthenticationState placeAuthenticationState;

    bool waitingForMarker;
    bool topReplicationContainersSent;
    bool remotePlayerInstalled;
    std::string gameSessionID;

    typedef std::vector<unsigned int> HashVector;
    unsigned long long securityTokens[3];


    void PlaceAuthenticationThread(int previousPlaceId, int requestedPlaceId);
    virtual void PlaceAuthenticationThreadImpl(int previousPlaceId, int requestedPlaceId);

    virtual void setAuthenticated(bool authenticated) {}

    // Replicator
    /*override*/ bool isProtectedStringEnabled();
    /*override*/ std::string encodeProtectedString(
        const ProtectedString& value, const Instance* instance, const Reflection::PropertyDescriptor& desc);
    /*override*/ boost::optional<ProtectedString> decodeProtectedString(
        const std::string& value, const Instance* instance, const Reflection::PropertyDescriptor& desc);

    /*override*/ bool checkDistributedReceive(PartInstance* part);
    /*override*/ bool checkDistributedSend(const PartInstance* part);
    /*override*/ bool checkDistributedSendFast(const PartInstance* part);
    /*override*/ bool isLegalReceiveInstance(Instance* instance, Instance* parent);
    /*override*/ bool isLegalDeleteInstance(Instance* instance);
    /*override*/ bool isLegalReceiveEvent(Instance* instance, const Reflection::EventDescriptor& desc);
    /*override*/ bool isLegalReceiveProperty(Instance* instance, const Reflection::PropertyDescriptor& desc);
    /*override*/ FilterResult filterPhysics(PartInstance* instance);
    /*override*/ bool prepareRemotePlayer(shared_ptr<Instance> instance);
    /*override*/ void onSentMarker(long id);
    /*override*/ void onSentTag(int id);
    /*override*/ bool canSendItems()
    {
        return true;
    } // The server always can send
    /*override*/ void rebroadcastEvent(Reflection::EventInvocation& eventInvocation);
    /*override*/ bool sendItemsPacket();
    /*override*/ void readItem(RakNet::BitStream& inBitstream, Aya::Network::Item::ItemType itemType);
    /*override*/ void addTopReplicationContainers(ServiceProvider* newProvider);
    /*override*/ void addTopReplicationContainer(
        Instance* instance, bool replicateProperties, bool replicateChildren, boost::function<void(shared_ptr<Instance>)> replicationMethodFunc);
    /*override*/ bool isLegalSendProperty(Instance* instance, const Reflection::PropertyDescriptor& desc);
    /*override*/ FilterResult filterReceivedChangedProperty(Instance* instance, const Reflection::PropertyDescriptor& desc);
    /*override*/ FilterResult filterReceivedParent(Instance* instance, Instance* parent);
    /*override*/ void dataOutStep();
    /*override*/ void onPropertyChanged(Instance* instance, const Reflection::PropertyDescriptor* descriptor);
    /*override*/ void writeChangedProperty(const Instance* instance, const Reflection::PropertyDescriptor& desc, RakNet::BitStream& outBitStream);
    /*override*/ void writeChangedRefProperty(
        const Instance* instance, const Reflection::RefPropertyDescriptor& desc, const Guid::Data& newRefGuid, RakNet::BitStream& outBitStream);
    /*override*/ void receiveCluster(RakNet::BitStream& inBitstream, Instance* instance, bool usingOneQuarterIterator);

    /*override*/ void setPropSyncExpiration(double value)
    {
        propSync.setExpiration(Aya::Time::Interval(value));
    }

    /*override*/ shared_ptr<Stats> createStatsItem();
    /*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

    void sendReplicatedFirstDescendants(shared_ptr<Instance> descendant);


private:
    void processRequestCharacter(Instance* instance, Aya::Guid::Data id, unsigned int sendStats, std::string preferedSpawnName);
    void readRequestCharacter(RakNet::BitStream& bitStream);
    void readClientQuotaUpdate(RakNet::BitStream& bitStream);
    void readRegionRemoval(RakNet::BitStream& bitStream);
    void readPropAcknowledgement(RakNet::BitStream& bitStream);
    virtual void installRemotePlayer(const std::string& preferedSpawnName);
    static void installRemotePlayerSafe(weak_ptr<ServerReplicator> weakThis, const std::string preferedSpawnName);
    void sendDictionaryFormat();
    static void toggleSendStatsJob(weak_ptr<ServerReplicator> weakServerReplicator, bool required, int version);

public: // methods
    /*override*/ Player* findTargetPlayer() const
    {
        return remotePlayer.get();
    }
    /*override*/ Player* getRemotePlayer() const
    {
        return remotePlayer.get();
    }
    /*override*/ bool canUseProtocolVersion(int protocolVersion) const;
    /*override*/ bool isProtocolCompatible() const;

    void setBasicFilteringEnabled(bool value);
    void preventTerrainChanges();

    virtual void serializeSFFlags(RakNet::BitStream& outBitStream) const;

    virtual void sendDictionaries();

    ServerReplicator(RakNet::SystemAddress systemAddress, Server* server, NetworkSettings* networkSettings);
    ~ServerReplicator();

    void sendTop(RakNet::RakPeerInterface* peer);

    const PartInstance* readPlayerSimulationRegion(Region2::WeightedPoint& weightedPoint);
    virtual void readPlayerSimulationRegion(const PartInstance* playerHead, Region2::WeightedPoint& weightedPoint);

    boost::function<FilterResult(shared_ptr<Instance>, std::string, Reflection::Variant)> filterProperty;
    boost::function<FilterResult(shared_ptr<Instance>, shared_ptr<Instance> parent)> filterNew;
    boost::function<FilterResult(shared_ptr<Instance>)> filterDelete;
    boost::function<FilterResult(shared_ptr<Instance>, std::string)> filterEvent;

    /*override*/ RakNet::PluginReceiveResult OnReceive(RakNet::Packet* packet);

    void writeDescriptorSchema(const Reflection::ClassDescriptor* desc, RakNet::BitStream& bitStream) const;

    void teachSchema();
    static RakNet::BitStream apiSchemaBitStream;
    static RakNet::BitStream apiDictionaryBitStream;
    const RakNet::BitStream& getSchemaBitStream() const;
    const RakNet::BitStream& getApiDictionaryBitStream() const;
    static void generateSchema(const ServerReplicator* serverRep, bool force);
    static void generateApiDictionary(const ServerReplicator* serverRep, bool force);

    /*override*/ bool isServerReplicator()
    {
        return true;
    };

    void onPlaceAuthenticationComplete(PlaceAuthenticationState placeAuthenticationResult);
};

#if defined(AYA_SERVER)
class CheatHandlingServerReplicator : public ServerReplicator
{
    bool isAuthenticated;
    bool isBadTicket;
    std::string ticket; // the ticket received from the remote client
    int userIdFromTicket;

    unsigned int sendStatsMask;
    unsigned int extraStatsMask;
    unsigned int apiStatsMask;

public:
    CheatHandlingServerReplicator(RakNet::SystemAddress systemAddress, Server* server, NetworkSettings* networkSettings);
    /*override*/ RakNet::PluginReceiveResult OnReceive(RakNet::Packet* packet);

private:
    double kickTimeSec;
    std::string kickName;
    bool processedTicket;
    void processTicket(RakNet::Packet* packet);
    void processSendStats(unsigned int sendStats, unsigned int extraStats);
    void processApiStats(unsigned long long apiStats);
    void preauthenticatePlayer(int userId);
    /*override*/ virtual void installRemotePlayer(const std::string& preferedSpawnName);
    void doRemoteSysStats(unsigned int sendStats, unsigned int mask, const char* codeName, const char* details,
        const std::string& configString = ::DFString::US30605p1);
    void doDelayedSysStats(unsigned int sendStats, unsigned int mask, const char* codeName, const char* details);
    ServerFuzzySecurityToken securityToken;
    ServerFuzzySecurityToken apiToken;
    unsigned long long prevApiToken;
    size_t numPingItems;
    bool reportedInvalid;
    bool reportedExploit;
    bool reportedSkip;
    bool reportedApiFail;
    bool reportedApiTamper;
    bool reportedRangeError;
    bool reportedPingItemTime;

protected:
    // Replicator
    /*override*/ bool checkRemotePlayer();
    /*override*/ bool canSendItems()
    {
        return processedTicket;
    } // The server can only send once we get a ticket from the client
    /*override*/ void setAuthenticated(bool authenticated)
    {
        isAuthenticated = authenticated;
    }
    /*override*/ void PlaceAuthenticationThreadImpl(int previousPlaceId, int requestedPlaceId);
    /*override*/ void checkPingItemTime();
};
#endif

} // namespace Network
} // namespace Aya