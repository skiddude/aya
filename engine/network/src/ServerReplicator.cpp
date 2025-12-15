

#include "ServerReplicator.hpp"
#include "Server.hpp"

#include "RakNet/RakPeer.hpp"

#include "Players.hpp"

#include "API.hpp"

#include "NetworkProfiler.hpp"
#include "NetworkSettings.hpp"
#include "NetworkOwnerJob.hpp"
#include "Replicator.NewInstanceItem.hpp"
#include "Replicator.StreamJob.hpp"
#include "Replicator.TagItem.hpp"
#include "Util.hpp"
#include "TopNErrorsPhysicsSender.hpp"
#include "DirectPhysicsReceiver.hpp"


#include "DataModel/Stats.hpp"

#include "DataModel/HackDefines.hpp"

#include "DataModel/PartInstance.hpp"

#include "DataModel/Workspace.hpp"
// TODO - move distributed physics switch somewhere else
#include "DataModel/Message.hpp"

#include "DataModel/DataModel.hpp"

#include "DataModel/Hopper.hpp"

#include "DataModel/Lighting.hpp"

#include "DataModel/LogService.hpp"

#include "DataModel/AdService.hpp"

#include "DataModel/Teams.hpp"

#include "DataModel/PointsService.hpp"

#include "DataModel/ReplicatedFirst.hpp"

#include "DataModel/HttpRbxApiService.hpp"

#include "DataModel/MegaCluster.hpp"

#include "World/Assembly.hpp"
#include "World/Mechanism.hpp"
#include "World/Primitive.hpp"
#include "World/DistributedPhysics.hpp"
#include "Script/ModuleScript.hpp"
#include "Script/script.hpp"
#include "Utility/Http.hpp"

#include "Utility/xxhash.hpp"

#include "Crypt.hpp"

#include "FastLog.hpp"
#include "NetworkOwner.hpp"

#include "Utility/Statistics.hpp"

#include "ConcurrentRakPeer.hpp"
#include "Utility/RobloxServicesTools.hpp"

#include "boost.hpp"

#include <boost/thread/xtime.hpp>
#include <boost/bind.hpp>
#include <sstream>


#include <boost/algorithm/string.hpp>
#include "Replicator.StatsItem.hpp"

#include "Script/LuaVM.hpp"

#ifdef _MSC_VER
#define POPCNT __popcnt
#else
#define POPCNT __builtin_popcount
#endif

#if (defined(__linux) && !defined(__aarch64__))
#include <cpuid.h> // For GCC/Clang
#endif

DYNAMIC_LOGGROUP(NetworkJoin)

LOGGROUP(JoinSendExtraItemCount)
FASTFLAG(DebugLocalRccServerConnection)
FASTFLAG(RemoveUnusedPhysicsSenders)
LOGGROUP(US14116)
DYNAMIC_FASTFLAGVARIABLE(UseProtocolCompatibilityCheck, false)
FASTFLAGVARIABLE(DebugForceRegenerateSchemaBitStream, false)
DYNAMIC_FASTFLAGVARIABLE(DebugLogProcessCharacterRequestTime, false)
DYNAMIC_FASTFLAGVARIABLE(DisablePlaceAuthenticationPoll, false)
DYNAMIC_FASTFLAGVARIABLE(FilterAllPlayerPropChanges, false)
DYNAMIC_FASTFLAGVARIABLE(LogAllPlayerPropChanges, false)

DYNAMIC_FASTFLAG(LoadGuisWithoutChar)

namespace Aya
{
namespace Network
{


class ServerReplicator::ServerStatsItem : public Replicator::Stats
{
    Item* itemCount;

    Item* sendStreamData;
    Item* avgStreamPacketSize;
    Item* avgInstancesPerStreamPacket;
    Item* numClientInstanceQuota;

public:
    ServerStatsItem(const shared_ptr<const ServerReplicator>& replicator)
        : Replicator::Stats(replicator)
    {
        Item* item = createChildItem("PropSync");
        itemCount = item->createChildItem("ItemCount");
        item->createBoundChildItem("RejectionCount", replicator->propSync.propertyRejectionCount);

        createBoundChildItem("Num Parts Owned", replicator->numPartsOwned);

        sendStreamData = createChildItem("Send Stream Data");
        avgStreamPacketSize = sendStreamData->createChildItem("Avg Packet Size");
        avgInstancesPerStreamPacket = sendStreamData->createChildItem("Avg Instance per step");
        numClientInstanceQuota = sendStreamData->createChildItem("Client instance quota");
    }

    /*override*/ void update()
    {
        Replicator::Stats::update();

        if (shared_ptr<const ServerReplicator> locked = shared_static_cast<const ServerReplicator>(replicator.lock()))
        {
            itemCount->formatValue(locked->propSync.itemCount());

            if (locked->streamJob)
            {
                sendStreamData->formatRate(locked->streamJob->packetsSent);
                avgStreamPacketSize->formatValue((int)locked->streamJob->avgPacketSize.value());
                avgInstancesPerStreamPacket->formatValue((int)locked->streamJob->avgInstancePerStep.value());
                numClientInstanceQuota->formatValue(locked->streamJob->getClientInstanceQuota());
            }
        }
    }
};

class Replicator::SendStatsJob : public ReplicatorJob
{
    int version;

public:
    SendStatsJob(Replicator& replicator, int requestedVersion)
        : version(requestedVersion)
        , ReplicatorJob("Replicator StatsSender", replicator, DataModelJob::DataOut)
    {
    }

private:
    virtual Time::Interval sleepTime(const Stats& stats)
    {
        return computeStandardSleepTime(stats, 1);
    }

    virtual Error error(const Stats& stats)
    {
        return computeStandardErrorCyclicExecutiveSleeping(stats, 1);
    }

    virtual TaskScheduler::StepResult stepDataModelJob(const Stats& stats)
    {
        replicator->sendStats(version);
        return TaskScheduler::Stepped;
    }
};
} // namespace Network
} // namespace Aya

const char* const Aya::Network::sServerReplicator = "ServerReplicator";

using namespace Aya;
using namespace Aya::Network;


static Reflection::BoundCallbackDesc<FilterResult(shared_ptr<Instance>, shared_ptr<Instance>)> desc_filterNew(
    "NewFilter", &ServerReplicator::filterNew, "newItem", "parent", Security::RobloxPlace);
static Reflection::BoundCallbackDesc<FilterResult(shared_ptr<Instance>)> desc_filterDelete(
    "DeleteFilter", &ServerReplicator::filterDelete, "deletingItem", Security::RobloxPlace);
static Reflection::BoundCallbackDesc<FilterResult(shared_ptr<Instance>, std::string, Reflection::Variant)> desc_filterProperty(
    "PropertyFilter", &ServerReplicator::filterProperty, "changingItem", "member", "value", Security::RobloxPlace);
static Reflection::BoundCallbackDesc<FilterResult(shared_ptr<Instance>, std::string)> desc_filterEvent(
    "EventFilter", &ServerReplicator::filterEvent, "firingItem", "event", Security::RobloxPlace);
static Reflection::BoundFuncDesc<ServerReplicator, void(bool)> desc_SetBasicFilteringEnabled(
    &ServerReplicator::setBasicFilteringEnabled, "SetBasicFilteringEnabled", "value", Security::RobloxPlace);
static Reflection::BoundFuncDesc<ServerReplicator, void()> desc_PreventTerrainChanges(
    &ServerReplicator::preventTerrainChanges, "PreventTerrainChanges", Security::RobloxPlace);
static Reflection::EventDesc<ServerReplicator, void(int, bool, int)> event_TicketProcessed(
    &ServerReplicator::remoteTicketProcessedSignal, "TicketProcessed", "userId", "isAuthenticated", "protocolVersion");
REFLECTION_END();

shared_ptr<Replicator::Stats> ServerReplicator::createStatsItem()
{
    return Creatable<Instance>::create<ServerStatsItem>(shared_from(this));
};

bool ServerReplicator::canUseProtocolVersion(int protocolVersion) const
{
    return remoteProtocolVersion == 0 || protocolVersion <= remoteProtocolVersion;
}

bool ServerReplicator::isProtocolCompatible() const
{
    if (!DFFlag::UseProtocolCompatibilityCheck)
    {
        return true;
    }
    return remoteProtocolVersion == NETWORK_PROTOCOL_VERSION || remoteProtocolVersion == 0;
}

ServerReplicator::ServerReplicator(RakNet::SystemAddress systemAddress, Server* server, NetworkSettings* networkSettings)
    : Super(systemAddress, server->rakPeer, networkSettings, /*ClusterDebounce=*/false)
    , server(server)
    , waitingForMarker(true)
    , topReplicationContainersSent(false)
    , remotePlayerInstalled(false)
    , acceptsTerrainChanges(true)
    , numPartsOwned(0)
    , remoteProtocolVersion(NETWORK_PROTOCOL_VERSION)
    , placeAuthenticationState(PlaceAuthenticationState_Init)
    , pendingCharaterRequest(NULL)
    , startTime(Time::nowFast())
{
    generateSchema(this, FFlag::DebugForceRegenerateSchemaBitStream);
    generateApiDictionary(this, FFlag::DebugForceRegenerateSchemaBitStream);

    if (settings().distributedPhysicsEnabled)
    {

        if (FFlag::RemoveInterpolationReciever)
        {
            physicsReceiver.reset(new DirectPhysicsReceiver(this, true));
            physicsReceiver->start(physicsReceiver);
        }
        else
        {
            createPhysicsReceiver(NetworkSettings::Direct, true);
        }
    }
    setName("ServerReplicator");

    setBasicFilteringEnabled(true);

    lightingService = ServiceProvider::find<Lighting>(server);
    globalShadowsDescriptor = lightingService ? lightingService->getDescriptor().findPropertyDescriptor("GlobalShadows") : NULL;
    outdoorAmbientDescriptor = lightingService ? lightingService->getDescriptor().findPropertyDescriptor("OutdoorAmbient") : NULL;
    outlinesDescriptor = lightingService ? lightingService->getDescriptor().findPropertyDescriptor("Outlines") : NULL;

    canTimeout = false;
}

ServerReplicator::~ServerReplicator() {}

void ServerReplicator::setBasicFilteringEnabled(bool value)
{
    basicFilter.reset(value ? new NetworkFilter(this) : NULL);
}


void ServerReplicator::preventTerrainChanges()
{
    acceptsTerrainChanges = false;
}

void ServerReplicator::receiveCluster(RakNet::BitStream& inBitstream, Instance* instance, bool usingOneQuarterIterator)
{
    // bounce-back? Ideally, the client wouldn't be allowed to make terrain changes
}

const PartInstance* ServerReplicator::readPlayerSimulationRegion(Region2::WeightedPoint& weightedPoint)
{
    if (const Player* proxyPlayer = findTargetPlayer())
    {
        CoordinateFrame temp;
        if (const PartInstance* head = proxyPlayer->hasCharacterHead(temp))
        {
            weightedPoint.point = temp.translation.xz();
            weightedPoint.radius = proxyPlayer->getSimulationRadius();
            // TODO: FIX THIS
            // Doing this will help with not assigning physics ownership to
            // assemblies before they are totally streamed, but it does not
            // prevent the client from doing local non-owner simulation of
            // those assemblies.
            if (streamJob)
            {
                streamJob->adjustSimulationOwnershipRange(&weightedPoint);
            }
            return head;
        }
    }
    return NULL;
}


void ServerReplicator::readPlayerSimulationRegion(const PartInstance* playerHead, Region2::WeightedPoint& weightedPoint)
{
    Player* player = findTargetPlayer();
    if (player && playerHead)
    {
        weightedPoint.point = playerHead->getCoordinateFrame().translation.xz();
        weightedPoint.radius = player->getSimulationRadius();
    }
}

PartInstance* getMechanismRootMovingPart(PartInstance* part)
{
    Primitive* p = Mechanism::getRootMovingPrimitive(part->getPartPrimitive());
    return PartInstance::fromPrimitive(p);
}

const PartInstance* getConstMechanismRootMovingPart(const PartInstance* part)
{
    const Primitive* p = Mechanism::getConstRootMovingPrimitive(part->getConstPartPrimitive());
    return PartInstance::fromConstPrimitive(p);
}

bool ServerReplicator::checkDistributedReceive(PartInstance* part)
{
    PartInstance* rootMoving = getMechanismRootMovingPart(part);
    bool isRootPart = (rootMoving == part);
    bool fromOwner = (rootMoving->getNetworkOwner() == RakNetToRbxAddress(remotePlayerId));

    return (isRootPart && fromOwner); // i.e. - allow receive if received from owner
}

bool ServerReplicator::checkDistributedSend(const PartInstance* part)
{
    AYAASSERT(part);

    const PartInstance* rootMoving = getConstMechanismRootMovingPart(part);
    bool isRootPart = (rootMoving == part);
    if (isRootPart)
    {
        return checkDistributedSendFast(rootMoving);
    }
    else
    {
        return false;
    }
}

// assumes part is root
bool ServerReplicator::checkDistributedSendFast(const PartInstance* part)
{
    AYAASSERT(part);
    AYAASSERT(getConstMechanismRootMovingPart(part) == part);

    bool toOwner = part->getNetworkOwner() == RakNetToRbxAddress(remotePlayerId);

    // allow send only if NOT to owner
    return (!toOwner);
}


void ServerReplicator::rebroadcastEvent(Reflection::EventInvocation& eventInvocation)
{
    eventInvocation.replicateEvent();
}


bool ServerReplicator::prepareRemotePlayer(shared_ptr<Instance> instance)
{
    shared_ptr<Player> player = Instance::fastSharedDynamicCast<Player>(instance);
    if (!player)
        return false;

    if (remotePlayer)
        throw std::runtime_error("remotePlayer already exists"); // we already have a player!

    remotePlayer = player;
    remotePlayer->setRemoteAddress(remotePlayerId);

    if (streamJob)
    {
        streamJob->setupListeners(remotePlayer.get());
        // force early spawn location calculation if we are streaming
        remotePlayer->setForceEarlySpawnLocationCalculation();
    }

    // if we already sent all initial instances, just spawn the character
    if (topReplicationContainersSent && !remotePlayerInstalled)
    {
        installRemotePlayer(initialSpawnName);
    }

    return server->getIsPlayerAuthenticationRequired();
}

void ServerReplicator::addTopReplicationContainers(ServiceProvider* newProvider)
{
    Super::addTopReplicationContainers(newProvider);

    // add tag item indicating end of initial instances
    pendingItems.push_back(new TagItem(this, TOP_REPLICATION_CONTAINER_FINISHED_TAG, boost::bind(&ServerReplicator::isInitialDataSent, this)));
}

void ServerReplicator::addTopReplicationContainer(
    Instance* instance, bool replicateProperties, bool replicateChildren, boost::function<void(shared_ptr<Instance>)> replicationMethodFunc)
{
    if (replicateProperties)
        replicationMethodFunc(shared_from(instance));

    Super::addTopReplicationContainer(instance, replicateProperties, replicateChildren, replicationMethodFunc);
}

bool ServerReplicator::isLegalDeleteInstance(Instance* instance)
{
    if (!Super::isLegalDeleteInstance(instance))
        return false;

    if (strictFilter)
    {
        if (strictFilter->filterDelete(instance) == Reject)
        {
            if (settings().printDataFilters)
                StandardOut::singleton()->printf(
                    MESSAGE_WARNING, "Filtering is enabled. Instance delete (%s) not be accepted.", instance->getName().c_str());

            return false;
        }
        return true;
    }

    FilterResult result;
    if (basicFilter && basicFilter->filterDelete(instance, result))
    {
        if (!server->dataBasicFilteredSignal.empty())
            server->dataBasicFilteredSignal(shared_from(this), result, shared_from(instance), "~");
        return result == Accept;
    }

    if (this->filterDelete)
    {
        result = filterDelete(shared_from(instance));

        if (!server->dataCustomFilteredSignal.empty())
            server->dataCustomFilteredSignal(shared_from(this), result, shared_from(instance), "~");

        return result == Accept;
    }

    return true;
}

bool ServerReplicator::isLegalReceiveInstance(Instance* instance, Instance* parent)
{
    if (!Super::isLegalReceiveInstance(instance, parent))
        return false;

    if (!server->isLegalReceiveInstance(instance))
        return false;

    if (Instance::fastDynamicCast<Message>(instance))
        return false;

    if (remotePlayer)
    {
        if (Instance::fastDynamicCast<Player>(instance))
        {
            // After we've captured the remotePlayer, don't accept other player data from the client
            throw std::runtime_error("remotePlayer already exists"); // we already have a player!
        }
    }

    if (strictFilter)
        return strictFilter->filterNew(instance, parent) == Accept;

    FilterResult result;
    if (basicFilter && basicFilter->filterNew(instance, parent, result))
    {
        if (!server->dataBasicFilteredSignal.empty())
            server->dataBasicFilteredSignal(shared_from(this), result, shared_from(instance), instance->getClassName().str);
        return result == Accept;
    }

    if (this->filterNew)
    {
        result = filterNew(shared_from(instance), shared_from(parent));

        if (!server->dataCustomFilteredSignal.empty())
            server->dataCustomFilteredSignal(shared_from(this), result, shared_from(instance), instance->getClassName().str);

        return result == Accept;
    }

    return true;
}

bool ServerReplicator::isLegalReceiveEvent(Instance* instance, const Reflection::EventDescriptor& desc)
{
    if (Player* player = Instance::fastDynamicCast<Player>(instance))
    {
        // Player events are tied to the player, the server will otherwise ignore them
        if (remotePlayer.get() != player)
            return false;
    }

    if (strictFilter)
        return strictFilter->filterEvent(instance, desc) == Accept;

    FilterResult result;
    if (basicFilter && basicFilter->filterEvent(instance, desc, result))
    {
        if (!server->dataBasicFilteredSignal.empty())
            server->dataBasicFilteredSignal(shared_from(this), result, shared_from(instance), desc.name.str);
        return result == Accept;
    }

    if (this->filterEvent)
    {
        result = filterEvent(shared_from(instance), desc.name.str);

        if (!server->dataCustomFilteredSignal.empty())
            server->dataCustomFilteredSignal(shared_from(this), result, shared_from(instance), desc.name.str);

        return result == Accept;
    }

    return true;
}

bool ServerReplicator::isLegalReceiveProperty(Instance* instance, const Reflection::PropertyDescriptor& desc)
{
    if (Instance::fastDynamicCast<Player>(instance))
    {
        if (DFFlag::FilterAllPlayerPropChanges)
            return false;
        else
            return (desc != Player::prop_userId) && (desc != Player::prop_userIdDeprecated) && (desc != Instance::desc_Name);
    }

    if (desc == Script::prop_EmbeddedSourceCode)
        return false; // TODO: Report hacker?

    if (desc == BaseScript::prop_SourceCodeId)
        return false; // TODO: Report hacker?

    if (desc == ModuleScript::prop_Source)
        return false;

    return Super::isLegalReceiveProperty(instance, desc);
}

void ServerReplicator::onSentMarker(long id)
{
    waitingForMarker = false;
}

void ServerReplicator::onSentTag(int id)
{
    if (id == TOP_REPLICATION_CONTAINER_FINISHED_TAG)
    {
        topReplicationContainersSent = true;
        if (FFlag::RemoveUnusedPhysicsSenders)
        {
            physicsSender.reset(new TopNErrorsPhysicsSender(*this));
            PhysicsSender::start(physicsSender);
        }
        else
        {
            createPhysicsSender(settings().getPhysicsSendMethod());
        }

        if (streamJob)
        {
            // order matters!
            streamJob->sendPackets(-1);
            streamJob->setReady(true);
        }

        // install the player and spawn character
        if (remotePlayer)
        {
            // submit a write task here, because we are in data out step
            DataModel::get(this)->submitTask(
                boost::bind(&ServerReplicator::installRemotePlayerSafe, weak_from(this), initialSpawnName), DataModelJob::Write);
        }
        // if we have not received the player object, defer install until we do (see prepareRemotePlayer() )

        // reset ping timers
        canTimeout = true;
        replicatorStats.resetSecurityTimes();
    }
}

bool ServerReplicator::isLegalSendProperty(Instance* instance, const Reflection::PropertyDescriptor& desc)
{
    return true;
}

void ServerReplicator::sendTop(RakNet::RakPeerInterface* peer)
{
#ifdef NETWORK_DEBUG
    StandardOut::singleton()->printf(Aya::MESSAGE_INFO, "SendTop");
#endif
    // First, set tags
    FASTLOG(FLog::Network, "ServerReplicator:sendTop - begin");
    RakNet::BitStream bitStream;
    bitStream << (unsigned char)ID_SET_GLOBALS;

    bitStream << settings().distributedPhysicsEnabled;
    bitStream << (streamJob ? true : false);

    Workspace* workspace = ServiceProvider::find<Workspace>(this);
    if (!workspace)
        throw std::runtime_error("ServerReplicator unable to find workspace.");

    bitStream << (workspace->getNetworkFilteringEnabled());
    if (canUseProtocolVersion(32))
    {
        bitStream << workspace->getAllowThirdPartySales();
    }

    if (canUseProtocolVersion(31))
    {
        // for analytics
        AYAASSERT(players);
        bitStream << players->getCharacterAutoSpawnProperty();
    }

    serverScope = Guid::getLocalScope();

    std::string scopeName = serverScope.getName()->toString();
    bitStream << scopeName;

    if (LuaVM::useSecureReplication())
    {
        unsigned int xorKey = boost::hash_value(DataModel::get(this)->getPlaceID());

        unsigned int scriptKey = LuaVM::getKey() ^ xorKey;
        bitStream << scriptKey;

        unsigned int coreScriptModKey = LuaVM::getModKeyCore() ^ xorKey;
        bitStream << coreScriptModKey;
    }

    // push other replication data into stream
    TopReplConts::iterator end = topReplicationContainers.end();
    std::vector<TopReplConts::iterator> removeVector;

    bool topRepContSync = true;
    if (topRepContSync)
    {
        AYAASSERT(topReplicationContainers.size() < 0xFF);
        bitStream << (uint8_t)topReplicationContainers.size();
    }

    for (TopReplConts::iterator iter = topReplicationContainers.begin(); iter != end; ++iter)
    {
        AYAASSERT(*iter != NULL);
        bool canReplicate = topRepContSync || canReplicateInstance(*iter, remoteProtocolVersion);

        if (topRepContSync)
        {
            DescriptorSender<Aya::Reflection::ClassDescriptor>::IdContainer idContainer = classDictionary.getId(&(*iter)->getDescriptor());
            classDictionary.send(bitStream, idContainer.id);
        }

        // if remoteProtocolVersion is 0, this means we are running locally (don't worry about what can replicate)
        if ((remoteProtocolVersion == 0) || canReplicate)
        {
            serializeId(bitStream, *iter);

            {
                // ReplicatedFirst needs to just send it's descendants once (so each client can get it)
                if (Aya::ReplicatedFirst* replicatedFirst = Aya::Instance::fastDynamicCast<Aya::ReplicatedFirst>(*iter))
                {
                    replicatedFirst->visitDescendants(boost::bind(&ServerReplicator::sendReplicatedFirstDescendants, this, _1));

                    // send a tag so client knows the replicatedFirst container is completely replicated when this is received
                    highPriorityPendingItems.push_back(new TagItem(this, REPLICATED_FIRST_FINISHED_TAG, NULL));
                }
            }
        }
        else
        {
            if (!topRepContSync)
            {
                FASTLOGS(FLog::Network, "ServerReplicator::sendTop Not replicating %s", (*iter)->getName().c_str());
                removeVector.push_back(iter);
                disconnectReplicationData(shared_from(*iter));
            }
        }
    }

    if (topRepContSync)
    {
        topReplicationContainersMap.clear();
        topReplicationContainers.clear();
    }
    else
    {
        // if we didn't send any top replication containers, remove them here
        for (std::vector<TopReplConts::iterator>::iterator removeIter = removeVector.begin(); removeIter != removeVector.end(); ++removeIter)
            topReplicationContainers.erase(*removeIter);
    }

    // Send ID_SET_GLOBALS
    peer->Send(&bitStream, settings().getDataSendPriority(), DATAMODEL_RELIABILITY, DATA_CHANNEL, remotePlayerId, false);
}

void ServerReplicator::sendReplicatedFirstDescendants(shared_ptr<Instance> descendant)
{
    highPriorityPendingItems.push_back(new (newInstancePool.get()) NewInstanceItem(this, descendant));
}

#if defined(AYA_SERVER)
CheatHandlingServerReplicator::CheatHandlingServerReplicator(RakNet::SystemAddress systemAddress, Server* server, NetworkSettings* networkSettings)
    : ServerReplicator(systemAddress, server, networkSettings)
    , isAuthenticated(false)
    , isBadTicket(false)
    , userIdFromTicket(0)
    , processedTicket(false)
    , apiStatsMask(0)
    , sendStatsMask(0)
    , extraStatsMask(0)
    , kickTimeSec(-1)
    , securityToken(ServerFuzzySecurityToken(0, ~getSecurityMask(DFString::US30605p1, '.')))
    , apiToken(ServerFuzzySecurityToken(0xFFFFFFFFFFFFFFFFULL, 0))
    , reportedInvalid(false)
    , reportedExploit(false)
    , reportedSkip(false)
    , reportedApiFail(false)
    , reportedApiTamper(false)
    , reportedRangeError(false)
{
}
#endif

#if defined(AYA_SERVER)
void CheatHandlingServerReplicator::preauthenticatePlayer(int userId)
{
    try
    {
        if (server->getIsPlayerAuthenticationRequired())
        {
            if (ticket.empty())
            {
                FASTLOG(FLog::Error, "ServerReplicator:preauthenticatePlayer - MissingTicket");
                throw Aya::runtime_error("missing ticket");
            }
            if (server->preusedTickets.find(ticket) != server->preusedTickets.end())
            {
                FASTLOG(FLog::Error, "ServerReplicator:preauthenticatePlayer - DuplicateTicket");
                throw Aya::runtime_error("ticket has already been used: %s", ticket.c_str());
            }
            server->preusedTickets.insert(ticket);

            // The ticket contains a timestamp and the signature
            std::vector<std::string> s;
            boost::split(s, ticket, boost::is_any_of(";"));
            if (s.size() != 3)
            {
                FASTLOG(FLog::Error, "ServerReplicator:preauthenticatePlayer - BadTicket");
                throw Aya::runtime_error("bad pre-ticket '%s'", ticket.c_str());
            }
            std::string timestamp = s[0];
            std::string signature2 = s[2];

            // re-create the signed message
            std::string message = Aya::format("%d\n%s\n%s", userId, DataModel::get(this)->jobId.c_str(), timestamp.c_str());

            // why does roblox only check the second signature?
            try
            {
                // verify now!
                Crypt().verifySignatureBase64(message, signature2);
            }
            catch (Aya::base_exception&)
            {
                FASTLOG(FLog::Error, "ServerReplicator:preauthenticatePlayer - BadTicketSignature");
                throw;
            }
        }
    }
    catch (Aya::base_exception& e)
    {
        isBadTicket = true;
        if (ContentProvider* contentProvider = ServiceProvider::find<ContentProvider>(this))
        {
            ReportStatisticWithMessage(contentProvider->getBaseUrl(), "Preauthenticate-TicketFail", e.what());
        }
        return;
    }
    userIdFromTicket = userId;
    isAuthenticated = true;
}
#endif

void ServerReplicator::installRemotePlayerSafe(weak_ptr<ServerReplicator> weakThis, const std::string preferedSpawnName)
{
    shared_ptr<ServerReplicator> strongThis = weakThis.lock();
    if (!strongThis)
    {
        return;
    }

    try
    {
        strongThis->installRemotePlayer(preferedSpawnName);
    }
    catch (Aya::base_exception& e)
    {
        // catch the exception.  Something has gone wrong in this process.  This shouldn't crash RCC.
        (void)(e);
        strongThis->remotePlayer = Aya::Creatable<Aya::Instance>::create<Aya::Network::Player>();
        strongThis->requestDisconnect(DisconnectReason_SendPacketError);
    }
}

void ServerReplicator::installRemotePlayer(const std::string& preferedSpawnName)
{
    if (remotePlayerInstalled)
        return;

    remotePlayer->setGameSessionID(gameSessionID);

    remotePlayer->setParent(players);
    FASTLOG(FLog::Network, "ServerReplicator:InstallRemotePlayer - LoadCharacter");
    if (players->getShouldAutoSpawnCharacter())
    {
        remotePlayer->loadCharacter(true, preferedSpawnName);
    }
    else if (DFFlag::LoadGuisWithoutChar && !ServiceProvider::find<Workspace>(this)->getNetworkFilteringEnabled())
    {
        remotePlayer->rebuildGui();
    }

    remotePlayerInstalled = true;
}

void ServerReplicator::sendDictionaryFormat()
{
    RakNet::BitStream bitStream;
    bitStream << (unsigned char)ID_DICTIONARY_FORMAT;
    bitStream << true; // Protocol schema sync
    bitStream << true; // ApiDictionaryCompression;
    rakPeer->rawPeer()->Send(&bitStream, settings().getDataSendPriority(), DATAMODEL_RELIABILITY, DATA_CHANNEL, remotePlayerId, false);
}

PluginReceiveResult ServerReplicator::OnReceive(Packet* packet)
{
    if (packet->systemAddress != remotePlayerId)
    {
        return RR_CONTINUE_PROCESSING;
    }

    switch ((unsigned char)packet->data[0])
    {
    case ID_NEW_INCOMING_CONNECTION:
    {

        sendDictionaryFormat();
        sendDictionaries();
        teachSchema();

        // if we are not using a real game replicator (aka CheatHandlingServerReplicator), then send top containers right away
        // Essentially we don't need to wait for the ticket to come in, as we are replicating in studio (guaranteed to have all the same top classes)
#if !defined(AYA_SERVER)
        sendTop(rakPeer->rawPeer());
#endif
    }
        return RR_CONTINUE_PROCESSING;

    case ID_SPAWN_NAME:
    {
        RakNet::BitStream inBitstream(packet->data, packet->length, false);
        inBitstream.IgnoreBits(8); // Ignore the packet id
        deserializeStringCompressed(initialSpawnName, inBitstream);
        FASTLOGS(DFLog::NetworkJoin, "initialSpawnName: %s", initialSpawnName);
    }
        return RR_STOP_PROCESSING_AND_DEALLOCATE;

    case ID_PLACEID_VERIFICATION:
    {
        RakNet::BitStream inBitstream(packet->data, packet->length, false);
        inBitstream.IgnoreBits(8); // Ignore the packet id
        int previousPlaceId;
        inBitstream >> previousPlaceId;
#ifdef NETWORK_DEBUG
        StandardOut::singleton()->printf(MESSAGE_INFO, "Received previous PlaceID: %d", previousPlaceId);
#endif
        {
            boost::optional<int> placeAutenticationResult = server->getPlaceAuthenticationResultForOrigin(previousPlaceId);
            if (placeAutenticationResult)
                placeAuthenticationState = (PlaceAuthenticationState)placeAutenticationResult.get();
            else
                placeAuthenticationThread.reset(new boost::thread(Aya::thread_wrapper(
                    boost::bind(&ServerReplicator::PlaceAuthenticationThread, shared_from(this), previousPlaceId, DataModel::get(this)->getPlaceID()),
                    "PlaceAuthenticationThread")));
        }
    }
        return RR_STOP_PROCESSING_AND_DEALLOCATE;

    case ID_PROTOCOL_SYNC:
    {
        RakNet::BitStream inBitstream(packet->data, packet->length, false);
        inBitstream.IgnoreBits(8); // Ignore the packet id
        inBitstream >> remoteProtocolVersion;

#ifdef NETWORK_DEBUG
        StandardOut::singleton()->printf(MESSAGE_INFO, "Client protocol version: %d", remoteProtocolVersion);
#endif
    }
        return RR_STOP_PROCESSING_AND_DEALLOCATE;
    case ID_REQUEST_STATS:
    {
        bool req;
        int version = 0;
        RakNet::BitStream inBitstream(packet->data, packet->length, false);
        inBitstream.IgnoreBits(8); // Ignore the packet id
        inBitstream >> req;
        if (req)
        {
            inBitstream >> version;
        }
        DataModel::get(this)->create<LogService>()->runCallbackIfPlayerHasConsoleAccess(
            shared_from(getRemotePlayer()), boost::bind(&ServerReplicator::toggleSendStatsJob, weak_from(this), req, version));

        return RR_STOP_PROCESSING_AND_DEALLOCATE;
    }
    default:
        return Replicator::OnReceive(packet);
    }
}

#if defined(AYA_SERVER)
bool CheatHandlingServerReplicator::checkRemotePlayer()
{
    if (remotePlayer)
    {
        return true;
    }
    remotePlayer = Aya::Creatable<Aya::Instance>::create<Aya::Network::Player>();
    requestDisconnect(DisconnectReason_SendPacketError);
    return false;
}
#endif

#if defined(AYA_SERVER)
void CheatHandlingServerReplicator::installRemotePlayer(const std::string& preferedSpawnName)
{
    if (remotePlayerInstalled)
        return;

    bool shouldSetParent = !isBadTicket;
    try
    {
        FASTLOG1(FLog::Network, "ServerReplicator:InstallRemotePlayer. UserID: %d", remotePlayer->getUserID());

        remotePlayer->setGameSessionID(gameSessionID);

        if (remotePlayer->getParent() != players)
        {
            if (server->getIsPlayerAuthenticationRequired())
            {
                if (players->getPlayerByID(remotePlayer->getUserID()))
                {
                    FASTLOG(FLog::Error, "ServerReplicator:InstallRemotePlayer - DuplicatePlayer");
                    // uh-oh! This player is already in the world
                    StandardOut::singleton()->printf(MESSAGE_WARNING, "Player %d is already in the world", remotePlayer->getUserID());
                    requestDisconnect(DisconnectReason_DuplicatePlayer);

                    if (ContentProvider* contentProvider = ServiceProvider::find<ContentProvider>(this))
                    {
                        ReportStatisticWithMessage(contentProvider->getBaseUrl(), "Authenticate-DupePlayer", "");
                    }
                    return;
                }
            }
            if (!DFFlag::IgnoreInvalidTicket)
            {
                remotePlayer->setParent(players);
                remotePlayer->loadChatInfo();
            }
        }
        else
        {
            shouldSetParent = false;
        }

        if (FFlag::DebugLocalRccServerConnection)
        {
            // skip the security check
        }
        else
        {
            if (userIdFromTicket != remotePlayer->getUserID())
                throw Aya::runtime_error("userId has morphed");
        }

        if (server->getIsPlayerAuthenticationRequired())
        {
            if (ticket.empty())
            {
                FASTLOG(FLog::Error, "ServerReplicator:InstallRemotePlayer - MissingTicket");
                throw Aya::runtime_error("missing ticket");
            }

            if (server->usedTickets.find(ticket) != server->usedTickets.end())
            {
                FASTLOG(FLog::Error, "ServerReplicator:InstallRemotePlayer - DuplicateTicket");
                throw Aya::runtime_error("ticket has already been used: %s", ticket.c_str());
            }

            server->usedTickets.insert(ticket);

            // The ticket contains a timestamp and the signature
            std::vector<std::string> s;
            boost::split(s, ticket, boost::is_any_of(";"));
            if (s.size() < 2)
            {
                FASTLOG(FLog::Error, "ServerReplicator:InstallRemotePlayer - BadTicket");
                throw Aya::runtime_error("bad ticket '%s'", ticket.c_str());
            }
            std::string timestamp = s[0];
            std::string signature = s[1];

            // re-create the signed message
            std::string message = Aya::format("%d\n%s\n%s\n%s\n%s", remotePlayer->getUserID(), remotePlayer->getName().c_str(),
                remotePlayer->getCharacterAppearance().c_str(), DataModel::get(this)->jobId.c_str(), timestamp.c_str());

            try
            {
                // verify now!
                Crypt().verifySignatureBase64(message, signature);
            }
            catch (Aya::base_exception&)
            {
                FASTLOG(FLog::Error, "ServerReplicator:InstallRemotePlayer - BadTicketSignature");
                throw;
            }
        }
    }
    catch (Aya::base_exception& e)
    {
        isBadTicket = true;
        if (ContentProvider* contentProvider = ServiceProvider::find<ContentProvider>(this))
        {
            ReportStatisticWithMessage(contentProvider->getBaseUrl(), "Authenticate-TicketFail", e.what());
        }

        FASTLOGS(FLog::Network, "ServerReplicator:InstallRemotePlayer - Authenticate-TicketFail: %s", e.what());
        throw;
    }

    if (DFFlag::IgnoreInvalidTicket && shouldSetParent)
    {
        remotePlayer->setParent(players);
        remotePlayer->loadChatInfo();
    }

    FASTLOG(FLog::Network, "ServerReplicator:InstallRemotePlayer - LoadCharacter");
    if (players->getShouldAutoSpawnCharacter())
    {
        remotePlayer->loadCharacter(true, preferedSpawnName);
    }

    remotePlayerInstalled = true;
}
#endif

#if defined(AYA_SERVER)
PluginReceiveResult CheatHandlingServerReplicator::OnReceive(Packet* packet)
{
    if (packet->systemAddress != remotePlayerId)
        return ServerReplicator::OnReceive(packet);

    switch ((unsigned char)packet->data[0])
    {
    case ID_SUBMIT_TICKET:
        // need to process tickets right away before other packets such as DATA
        processTicket(packet);
        remoteTicketProcessedSignal(userIdFromTicket, isAuthenticated, remoteProtocolVersion);
        processedTicket = true;

        FASTLOG1(FLog::Network, "Player authenticated = %d", isAuthenticated);

        return RR_STOP_PROCESSING_AND_DEALLOCATE;

    case ID_DATA:
        if (!isAuthenticated)
        {
            FASTLOGS(FLog::Warning, "Player not authenticated %s", RakNetAddressToString(packet->systemAddress).c_str());
            // Discard all data coming from non-authenticated clients
            return RR_STOP_PROCESSING_AND_DEALLOCATE;
        }
        return ServerReplicator::OnReceive(packet);

    default:
        return ServerReplicator::OnReceive(packet);
    }
}
#endif

bool ServerReplicator::sendItemsPacket()
{
    if (!Super::sendItemsPacket())
        return false;

    int limit = settings().sendPacketBufferLimit;

    // during loading of a level, we'll send multiple packets at a time
    if (limit == -1 && !topReplicationContainersSent)
    {
        for (int i = 0; i < FLog::JoinSendExtraItemCount; i++)
            if (!Super::sendItemsPacket())
                return false;
    }

    return true;
}

#if defined(AYA_SERVER)
void CheatHandlingServerReplicator::processTicket(Packet* packet)
{
    try
    {
        FASTLOG(FLog::Network, "ServerReplicator:processTicket");

        if (!ticket.empty())
        {
            // We already got a ticket. This must be a hack!
            // TODO: report the hack
            requestDisconnect(DisconnectReason_DuplicateTicket);
            return;
        }

        RakNet::BitStream inBitstream(packet->data, packet->length, false);
        inBitstream.IgnoreBits(8); // Ignore the packet id

        Peer::decryptDataPart(inBitstream);

        int userId;
        inBitstream >> userId;

        deserializeStringCompressed(ticket, inBitstream);

        preauthenticatePlayer(userId);

        std::string hash;
        deserializeStringCompressed(hash, inBitstream);

        inBitstream >> remoteProtocolVersion;

        if (!isProtocolCompatible())
        {
            // invalidate caches
            oneQuarterClusterPacketCache.reset();
            clusterPacketCache.reset();
            instancePacketCache.reset();
        }

        FASTLOG1(FLog::Network, "ServerReplicator:processTicket - remoteProtocolVersion = %i", remoteProtocolVersion);

        if (!server->protocolVersionMatches(remoteProtocolVersion))
        {
            isAuthenticated = false;

            // send disconnect message to client
            RakNet::BitStream bitStream;
            bitStream << (unsigned char)ID_PROTOCAL_MISMATCH;
            rakPeer->rawPeer()->Send(&bitStream, IMMEDIATE_PRIORITY, DATAMODEL_RELIABILITY, DATA_CHANNEL, remotePlayerId, false);
        }

        std::string securityKey;
        deserializeStringCompressed(securityKey, inBitstream);

        std::string platform, product;
        deserializeStringCompressed(platform, inBitstream);
        deserializeStringCompressed(product, inBitstream);

        deserializeStringCompressed(gameSessionID, inBitstream);

        if (!server->securityKeyMatches(securityKey))
        {
            isAuthenticated = false;

            // log using StatsService
            if (Aya::Stats::StatsService* stats = ServiceProvider::find<Aya::Stats::StatsService>(this))
            {
                shared_ptr<Reflection::ValueTable> entry(new Reflection::ValueTable());
                (*entry)["PlayerId"] = userIdFromTicket;
                stats->report("SecurityMismatch", entry);
            }

            FASTLOG(FLog::Network, "ServerReplicator:processTicket - Security key mismatch");

            // send disconnect message to client
            RakNet::BitStream bitStream;
            bitStream << (unsigned char)ID_SECURITYKEY_MISMATCH;
            rakPeer->rawPeer()->Send(&bitStream, IMMEDIATE_PRIORITY, DATAMODEL_RELIABILITY, DATA_CHANNEL, remotePlayerId, false);

            requestDisconnect(DisconnectReason_SecurityKeyMismatch);
        }

        // Now make sure we have top replication containers, since we have remote network version to filter accordingly with
        FASTLOG(FLog::Network, "ServerReplicator:processTicket - sendTop");
        sendTop(rakPeer->rawPeer());
    }
    catch (std::exception e)
    {
        // This is here due to exploiters.
        FASTLOG1(FLog::Network, "ServerReplicator::processTicket exception:  %s", e.what());
        requestDisconnect(DisconnectReason_SendPacketError);
        return;
    }
}
#endif


void ServerReplicator::toggleSendStatsJob(weak_ptr<ServerReplicator> weakServerReplicator, bool required, int version)
{
    if (shared_ptr<ServerReplicator> serverReplicator = weakServerReplicator.lock())
    {
        shared_ptr<TaskScheduler::Job>& sendStatsJob = serverReplicator->sendStatsJob;
        if (required)
        {
            if (!sendStatsJob)
            {
                sendStatsJob = shared_ptr<Replicator::SendStatsJob>(new Replicator::SendStatsJob(*serverReplicator, version));
                TaskScheduler::singleton().add(sendStatsJob);
            }
        }
        else
        {
            TaskScheduler::singleton().remove(sendStatsJob);
            sendStatsJob.reset();
        }
    }
}

#if defined(AYA_SERVER)
void CheatHandlingServerReplicator::processApiStats(unsigned long long apiStats)
{
    // split the msb's out from the lsb
    unsigned int apiStatsUpper = apiStats >> 32;
    unsigned int apiStatsLower = apiStats & ~apiStatsMask;
    bool configError = false;
    if (apiStatsUpper)
    {
        if (DFFlag::US28292p3)
        {
            players->onRemoteSysStats(remotePlayer->getUserID(), "1920x1200", "Zot");
        }
        reportedApiTamper = true;
    }
    else if (apiStatsLower)
    {
        uint32_t configMask;
        configMask = getSecurityMask(DFString::US30605p3, kKickChar, &configError);
        if (configMask & apiStatsLower)
        {
            std::stringstream msgStream;
            msgStream << "ApiStats: ";
            msgStream << std::hex << apiStatsLower;
            std::string msg = msgStream.str();
        }
        configMask |= getSecurityMask(DFString::US30605p3, kReportChar, &configError);
        if (configMask & apiStatsLower)
        {
            players->onRemoteSysStats(remotePlayer->getUserID(), "1920x1200", "Zix");
        }
        apiStatsMask |= apiStatsLower;
    }
    if (configError)
    {
        reportConfigMaskError("US30605p3");
    }
}
#endif


#if defined(AYA_SERVER)
void CheatHandlingServerReplicator::doRemoteSysStats(
    unsigned int sendStats, unsigned int mask, const char* codeName, const char* details, const std::string& configString)
{
    bool configError = false;
    if (sendStats & mask)
    {
        uint32_t configMask = getSecurityMask(configString, kKickChar, &configError);
        if (configMask & mask)
        {
            players->onRemoteSysStats(remotePlayer->getUserID(), "1920x1200", codeName);
        }
        configMask |= getSecurityMask(configString, kReportChar, &configError);
    }
    if (configError)
    {
        reportConfigMaskError(configString.c_str());
    }
}
void CheatHandlingServerReplicator::doDelayedSysStats(unsigned int sendStats, unsigned int mask, const char* codeName, const char* details)
{
    bool configError = false;
    if (sendStats & mask)
    {
        uint32_t configMask = getSecurityMask(DFString::US30605p1, kKickChar, &configError);
        if (configMask & mask)
        {
            kickName = codeName;
            kickTimeSec = Time::nowFastSec() + 60 + (rand() % 0x80);
        }
        configMask |= getSecurityMask(DFString::US30605p1, kReportChar, &configError);
    }
    if (configError)
    {
        reportConfigMaskError("US30605p1");
    }
}
#endif


#if defined(AYA_SERVER)
void CheatHandlingServerReplicator::processSendStats(unsigned int sendStats, unsigned int extraStats)
{
    unsigned int maskedSendStats = sendStats & ~sendStatsMask;
    if (maskedSendStats)
    {
#if defined(_DEBUG) || defined(_NOOPT)
        StandardOut::singleton()->printf(
            MESSAGE_ERROR, "ServerReplicator::processSendStats sendStats=0x%08X, extraStats=0x%08X", sendStats, extraStats);
#endif
        doRemoteSysStats(maskedSendStats, HATE_IMPOSSIBLE_ERROR, "impala", "Impossible Error (31)");
        if (maskedSendStats && ((maskedSendStats & HATE_IMPOSSIBLE_ERROR) == 0))
        {
            doRemoteSysStats(maskedSendStats, HATE_CE_ASM, "robert", "WriteCopy changed (30)");
            doRemoteSysStats(maskedSendStats, HATE_NEW_AV_CHECK, "moded", "Stealth Edit Revival (29)");
            doRemoteSysStats(maskedSendStats, HATE_HASH_FUNCTION_CHANGED, "booing", "Tried to modify hash function (28)");

            doRemoteSysStats(maskedSendStats, HATE_RETURN_CHECK, "bobby", "Function Return Check Failed (27)");
            doRemoteSysStats(maskedSendStats, HATE_VERB_SNATCH, "vera", "Tried to get build tools (26)");
            doDelayedSysStats(maskedSendStats, HATE_VEH_HOOK, "vegah", "VEH used (25)");
            doRemoteSysStats(maskedSendStats, HATE_HSCE_HASH_CHANGED, "fisher", "HumanoidState::computeEvent changed (24)");

            doDelayedSysStats(maskedSendStats, HATE_DLL_INJECTION, "dallas", "DLL Injection (23)");
            doRemoteSysStats(maskedSendStats, HATE_INVALID_ENVIRONMENT, "tomy", "Sandbox or VM detected (22)");
            doRemoteSysStats(maskedSendStats, HATE_SPEEDHACK, "usain", "Speedhack. (21)");
            doRemoteSysStats(maskedSendStats, HATE_LUA_VM_HOOKED, "carol", "Lua vm hooked (20)");

            doRemoteSysStats(maskedSendStats, HATE_OSX_MEMORY_HASH_CHANGED, "steven", "OSX hash changed (19)");
            doDelayedSysStats(maskedSendStats, HATE_UNHOOKED_VEH, "larry", "Our VEH hook removed (18)");
            doRemoteSysStats(maskedSendStats, HATE_CHEATENGINE_NEW, "mal", "Any New CE Method (17)");
            doRemoteSysStats(maskedSendStats, HATE_HSCE_EBX, "ebx", " HumanoidState::computeEvent changed ebx (16)");

            doRemoteSysStats(maskedSendStats, HATE_WEAK_DM_POINTER_BROKEN, "terrance", "Early null weak pointer (15)");
            doRemoteSysStats(maskedSendStats, HATE_LUA_HASH_CHANGED, "ursula", "Lua hash changed (14)");
            doRemoteSysStats(maskedSendStats, HATE_DESTROY_ALL, "bruger", "Speculative Call Check (13)");
            doRemoteSysStats(maskedSendStats, HATE_SEH_CHECK, "seth", "SEH chain into dll (12)");

            doDelayedSysStats(maskedSendStats, HATE_HOOKED_GTX, "curly", "Hooked API function (11)");
            doRemoteSysStats(maskedSendStats, HATE_DEBUGGER, "olivia", "Debugger found (10)");
            doRemoteSysStats(maskedSendStats, HATE_LUA_SCRIPT_HASH_CHANGED, "norman", "Lua script hash changed (9)");
            doRemoteSysStats(maskedSendStats, HATE_CATCH_EXECUTABLE_ACCESS_VIOLATION, "mallory", "Catch executable acccess violation (8)");

            doRemoteSysStats(maskedSendStats, HATE_CONST_CHANGED, "lance", "Const Changed (7)");
            doRemoteSysStats(maskedSendStats, HATE_INVALID_BYTECODE, "jack", "Invalid bytecode (6)");
            doRemoteSysStats(maskedSendStats, HATE_MEMORY_HASH_CHANGED, "imogen", "Memory hash changed (5)");
            doRemoteSysStats(maskedSendStats, HATE_ILLEGAL_SCRIPTS, "ivan", "Illegal scripts (4)");

            doRemoteSysStats(maskedSendStats, HATE_SIGNATURE, "omar", "Bad signature (3)");
            doDelayedSysStats(maskedSendStats, HATE_NEW_HWBP, "moe", "detected HWBP (2)");
            doRemoteSysStats(maskedSendStats, HATE_XXHASH_BROKEN, "lafayette", "xxhash broken (1)");
            doRemoteSysStats(maskedSendStats, HATE_CHEATENGINE_OLD, "murdle", "Cheat Engine Stable Methods (0)"); // send stats
        }
        sendStatsMask |= sendStats;
    }

    unsigned int maskedExtraStats = extraStats & ~extraStatsMask;
    if (maskedExtraStats)
    {
        doRemoteSysStats(maskedExtraStats, SCORN_IMPOSSIBLE_ERROR, "impala", "Scorn Impossible Error (31:12)", ::DFString::US30605p5);
        if (maskedExtraStats && ((maskedExtraStats & SCORN_IMPOSSIBLE_ERROR) == 0))
        {
            doRemoteSysStats(maskedExtraStats, SCORN_REPLICATE_PROP, "tochigi", "Scorn Replication (11:0)", ::DFString::US30605p5);
        }
        extraStatsMask |= extraStats;
    }

    if ((kickTimeSec > 0) && (Time::nowFastSec() > kickTimeSec))
    {
        players->onRemoteSysStats(remotePlayer->getUserID(), "1920x1200", kickName);
    }
}
#endif

void ServerReplicator::readItem(RakNet::BitStream& inBitstream, Aya::Network::Item::ItemType itemType)
{
    switch (itemType)
    {
    default:
        Super::readItem(inBitstream, itemType);
        break;

    case Item::ItemTypeUpdateClientQuota:
        readClientQuotaUpdate(inBitstream);
        break;

    case Item::ItemTypeRegionRemoval:
        NETPROFILE_START("ReadItemTypeRegionRemoval", &inBitstream);
        streamJob->readRegionRemoval(inBitstream);
        NETPROFILE_END("ReadItemTypeRegionRemoval", &inBitstream);
        break;

    case Item::ItemTypeInstanceRemoval:
        streamJob->readInstanceRemoval(inBitstream);
        break;

    case Item::ItemTypeRequestCharacter:
        readRequestCharacter(inBitstream);
        break;

    case Item::ItemTypePropAcknowledgement:
        readPropAcknowledgement(inBitstream);
        break;
    }
}

void ServerReplicator::readClientQuotaUpdate(RakNet::BitStream& bitStream)
{
    int diff;
    short maxRegionRadius;
    bitStream >> diff;
    bitStream >> maxRegionRadius;

    if (streamJob)
        streamJob->updateClientQuota(diff, maxRegionRadius);
}

void ServerReplicator::PlaceAuthenticationThread(int previousPlaceId, int requestedPlaceId)
{
    PlaceAuthenticationThreadImpl(previousPlaceId, requestedPlaceId);
}

void ServerReplicator::PlaceAuthenticationThreadImpl(int previousPlaceId, int requestedPlaceId)
{
    placeAuthenticationState = PlaceAuthenticationState_Authenticated;
}

void ServerReplicator::onPlaceAuthenticationComplete(PlaceAuthenticationState placeAuthenticationResult)
{
    AYAASSERT(placeAuthenticationState == placeAuthenticationResult);

    if (placeAuthenticationState == PlaceAuthenticationState_Authenticated)
        placeAutenticatedSignal();
}

#if defined(AYA_SERVER)
void CheatHandlingServerReplicator::PlaceAuthenticationThreadImpl(int previousPlaceId, int requestedPlaceId)
{
    placeAuthenticationState = PlaceAuthenticationState_Requesting;

#ifdef NETWORK_DEBUG
    if (requestedPlaceId == 0)
    {
        requestedPlaceId = 1818;
    }
#endif

    std::string baseUrl = ServiceProvider::create<ContentProvider>(this)->getApiBaseUrl();
    char urlBuf[2048] = {0};

    const char* a = (!baseUrl.empty() && baseUrl.back() == '/') ? "" : "/";

    sprintf_s(urlBuf, sizeof(urlBuf), "%s%suniverses/validate-place-join?originPlaceId=%d&destinationPlaceId=%d", baseUrl.c_str(), a, previousPlaceId,
        requestedPlaceId);

    std::string url = urlBuf;
    try
    {
        std::string response = "";

        if (Aya::HttpRbxApiService* apiService = Aya::ServiceProvider::find<Aya::HttpRbxApiService>(this))
        {
            Aya::Http http(url);
            apiService->get(http, true, Aya::PRIORITY_EXTREME, response);
        }
        else
        {
            Aya::Http(url).get(response);
        }
#ifdef NETWORK_DEBUG
        StandardOut::singleton()->printf(
            MESSAGE_INFO, "Place authentication requested (%d -> %d). Result: %s", previousPlaceId, requestedPlaceId, response.c_str());
#endif
        if (response == "true")
        {
            placeAuthenticationState = PlaceAuthenticationState_Authenticated;
        }
        else
        {
            placeAuthenticationState = PlaceAuthenticationState_Denied;
        }
    }
    catch (Aya::base_exception& e)
    {
        Aya::StandardOut::singleton()->printf(Aya::MESSAGE_ERROR, "Exception in place validation: %s", e.what());
        Aya::StandardOut::singleton()->printf(Aya::MESSAGE_ERROR, "url used for validation: %s", url.c_str());
        // we have a web service exception, just let the user play if they are not teleporting
        // if (previousPlaceId == 0)
        //{
        placeAuthenticationState = PlaceAuthenticationState_Authenticated;
        //}
        // else
        //{
        //    placeAuthenticationState = PlaceAuthenticationState_Denied;
        //}
    }

    if (DFFlag::DisablePlaceAuthenticationPoll)
    {
        // use a write task here to avoid threading issues
        DataModel::get(this)->submitTask(
            boost::bind(&ServerReplicator::onPlaceAuthenticationComplete, this, placeAuthenticationState), DataModelJob::Write);
    }

    server->registerPlaceAuthenticationResult(previousPlaceId, placeAuthenticationState);
}
#endif


void ServerReplicator::readRequestCharacter(RakNet::BitStream& inBitstream)
{
    // #TODO: remove this function

    // Note, this function can be called multiple times due to multiple client requests
    // Make sure all the core logic here are wrapped by canSpawnPlayer
#ifdef NETWORK_DEBUG
    StandardOut::singleton()->printf(MESSAGE_INFO, "Handling character request...");
#endif
    bool canSpawnPlayer = true;
    if (!DFFlag::DisablePlaceAuthenticationPoll)
    {
        if (placeAuthenticationState == PlaceAuthenticationState_DisconnectingClient)
        {
            canSpawnPlayer = false;
        }
        else
        {
            if (placeAuthenticationState == PlaceAuthenticationState_Requesting)
            {
                // ask client to retry
                RakNet::BitStream outBitStream;
                bool retry = true;
                outBitStream << (unsigned char)ID_PLACEID_VERIFICATION;
                outBitStream << retry;
                rakPeer->rawPeer()->Send(&outBitStream, settings().getDataSendPriority(), DATAMODEL_RELIABILITY, DATA_CHANNEL, remotePlayerId, false);
                canSpawnPlayer = false;
            }
            else if (placeAuthenticationState == PlaceAuthenticationState_Authenticated)
            {
                canSpawnPlayer = true;
            }
            else
            {
                canSpawnPlayer = false;
            }
        }
    }

    unsigned int sendStats;
    inBitstream >> sendStats;

    std::string spawnName;
    inBitstream >> spawnName;

    shared_ptr<Instance> instance;
    Aya::Guid::Data id;

    if (deserializeInstanceRef(inBitstream, instance, id))
    {
        // ignore character request from client, it'll be automatically handled after server receives the player and sent all initial instances.
        return;
    }
    else
    {
        FASTLOG(FLog::Error, "RequestCharacter - could not resolve RemotePlayer");
        throw std::runtime_error(
            format("Couldn't resolve remotePlayer %s from %s", id.readableString().c_str(), RakNetAddressToString(remotePlayerId).c_str()));
    }
}


void ServerReplicator::processRequestCharacter(Instance* instance, Aya::Guid::Data id, unsigned int sendStats, std::string preferedSpawnName)
{
    if (settings().printInstances)
    {
        StandardOut::singleton()->printf(
            MESSAGE_SENSITIVE, "Received remotePlayer %s from %s", id.readableString().c_str(), RakNetAddressToString(remotePlayerId).c_str());
    }

    if (!remotePlayer)
    {
        FASTLOG(FLog::Error, "RequestCharacter - RemotePlayer is null");
        StandardOut::singleton()->printf(
            MESSAGE_SENSITIVE, "RequestCharacter - RemotePlayer is null. %s", RakNetAddressToString(remotePlayerId).c_str());
        throw std::runtime_error("remotePlayer is NULL");
    }

    if (remotePlayer.get() != instance)
    {
        FASTLOG(FLog::Error, "RequestCharacter - RemotePlayer is wrong");
        // TODO: Should we report this, too (using onRemoteSysStats)?
        throw std::runtime_error("remotePlayer is wrong");
    }

    processSendStats(sendStats, kNoScornFlags);

    installRemotePlayer(preferedSpawnName);
}

FilterResult ServerReplicator::filterReceivedChangedProperty(Instance* instance, const Reflection::PropertyDescriptor& desc)
{
    AYAASSERT(instance);

    if (Super::filterReceivedChangedProperty(instance, desc) == Reject)
        return Reject;

    if (propSync.onReceivedPropertyChanged(Reflection::ConstProperty(desc, instance)) == Reject)
        return Reject;

    if (strictFilter)
    {
        FilterResult result = strictFilter->filterChangedProperty(instance, desc);

        if (result == Reject && settings().printDataFilters)
            Aya::StandardOut::singleton()->printf(Aya::MESSAGE_WARNING,
                "Filtering is enabled. Property %s change for instance %s will not be replicated.", desc.name.c_str(),
                instance->getFullName().c_str());

        return result;
    }

    FilterResult result;
    if (basicFilter && basicFilter->filterChangedProperty(instance, desc, result))
    {
        if (!server->dataBasicFilteredSignal.empty())
            server->dataBasicFilteredSignal(shared_from(this), result, shared_from(instance), desc.name.str);
        return result;
    }

    if (desc != Instance::propParent) // propParent is handled in filterReceivedParent
        if (this->filterProperty)
        {
            result = filterProperty(shared_from(instance), desc.name.str, Reflection::Variant());

            if (!server->dataCustomFilteredSignal.empty())
                server->dataCustomFilteredSignal(shared_from(this), result, shared_from(instance), desc.name.str);

            return result;
        }

    return Accept;
}

FilterResult ServerReplicator::filterReceivedParent(Instance* instance, Instance* parent)
{
    AYAASSERT(instance);

    if (Super::filterReceivedParent(instance, parent) == Reject)
        return Reject;

    FilterResult result;

    // Can't move illegal scripts
    if (!server->isScriptLegal(instance))
        return Reject;

    if (strictFilter)
    {
        result = strictFilter->filterParent(instance, parent);

        if (result == Reject)
            Aya::StandardOut::singleton()->printf(Aya::MESSAGE_WARNING,
                "Filtering is enabled. Parent %s change for instance %s will not be accepted.", parent->getName().c_str(),
                instance->getFullName().c_str());

        return result;
    }

    if (basicFilter && basicFilter->filterParent(instance, parent, result))
    {
        if (!server->dataBasicFilteredSignal.empty())
            server->dataBasicFilteredSignal(shared_from(this), result, shared_from(instance), Instance::propParent.name.str);
        return result;
    }

    if (this->filterProperty)
    {
        result = filterProperty(shared_from(instance), Instance::propParent.name.str, shared_from(parent));

        if (!server->dataCustomFilteredSignal.empty())
            server->dataCustomFilteredSignal(shared_from(this), result, shared_from(instance), Instance::propParent.name.str);

        return result;
    }

    return Accept;
}

FilterResult ServerReplicator::filterPhysics(PartInstance* instance)
{
    if (Super::filterPhysics(instance) == Reject)
        return Reject;
    return propSync.onReceivedPropertyChanged(Reflection::ConstProperty(PartInstance::prop_CFrame, instance));
}


void ServerReplicator::dataOutStep()
{
    if (placeAuthenticationState == PlaceAuthenticationState_Denied)
    {
        // place authentication failed, disconnect the client
        placeAuthenticationState = PlaceAuthenticationState_DisconnectingClient;
        RakNet::BitStream outBitStream;
        bool retry = false;
        outBitStream << (unsigned char)ID_PLACEID_VERIFICATION;
        outBitStream << retry;
        rakPeer->rawPeer()->Send(&outBitStream, IMMEDIATE_PRIORITY, DATAMODEL_RELIABILITY, DATA_CHANNEL, remotePlayerId, false);
        setAuthenticated(false);
    }

    propSync.expireItems();
    Super::dataOutStep();
}

void ServerReplicator::onPropertyChanged(Instance* instance, const Reflection::PropertyDescriptor* descriptor)
{
    Super::onPropertyChanged(instance, descriptor);
    propSync.onPropertyChanged(Reflection::ConstProperty(*descriptor, instance));
}

void ServerReplicator::writeChangedProperty(const Instance* instance, const Reflection::PropertyDescriptor& desc, RakNet::BitStream& outBitStream)
{
    DescriptorSender<Aya::Reflection::PropertyDescriptor>::IdContainer idContainer = propDictionary.getId(&desc);

    int byteStart = outBitStream.GetNumberOfBytesUsed();

    Item::writeItemType(outBitStream, Item::ItemTypeChangeProperty);

    // Write the GUID
    serializeId(outBitStream, instance);

    // Write property name
    propDictionary.send(outBitStream, idContainer.id);

    // This is the mirror image of ClientReplicator::readChangedProperty
    bool versionReset = propSync.onPropertySend(Reflection::ConstProperty(desc, instance)) == PropSync::Master::SendVersionReset;
    outBitStream << versionReset;

    serializePropertyValue(Reflection::ConstProperty(desc, instance), outBitStream, true /*useDictionary*/);

    if (settings().printProperties)
    {
        Aya::StandardOut::singleton()->printf(Aya::MESSAGE_SENSITIVE, "Replication prop: %s:%s.%s >> %s, bytes: %d", instance->getClassName().c_str(),
            instance->getGuid().readableString().c_str(), desc.name.c_str(), RakNetAddressToString(remotePlayerId).c_str(),
            outBitStream.GetNumberOfBytesUsed() - byteStart);
    }
    if (settings().trackDataTypes)
    {
        replicatorStats.incrementPacketsSent(desc.category.str);
        replicatorStats.samplePacketsSent(desc.category.str, outBitStream.GetNumberOfBytesUsed() - byteStart);
    }
}

void ServerReplicator::writeChangedRefProperty(
    const Instance* instance, const Reflection::RefPropertyDescriptor& desc, const Guid::Data& newRefGuid, RakNet::BitStream& outBitStream)
{
    DescriptorSender<Aya::Reflection::PropertyDescriptor>::IdContainer idContainer = propDictionary.getId(&desc);

    int byteStart = outBitStream.GetNumberOfBytesUsed();

    Item::writeItemType(outBitStream, Item::ItemTypeChangeProperty);

    // Write the GUID
    serializeId(outBitStream, instance);

    // Write property name
    propDictionary.send(outBitStream, idContainer.id);

    // This is the mirror image of ClientReplicator::readChangedProperty
    bool versionReset = propSync.onPropertySend(Reflection::ConstProperty(desc, instance)) == PropSync::Master::SendVersionReset;
    outBitStream << versionReset;

    if (newRefGuid.scope.isNull())
    {
        scopeNames.sendEmptyItem(outBitStream);
    }
    else
    {
        serializeId(outBitStream, newRefGuid);
    }

    if (settings().printProperties)
    {
        Aya::StandardOut::singleton()->printf(Aya::MESSAGE_SENSITIVE, "Replication ref prop: %s:%s.%s >> %s, bytes: %d",
            instance->getClassName().c_str(), instance->getGuid().readableString().c_str(), desc.name.c_str(),
            RakNetAddressToString(remotePlayerId).c_str(), outBitStream.GetNumberOfBytesUsed() - byteStart);
    }
    if (settings().trackDataTypes)
    {
        replicatorStats.incrementPacketsSent(desc.category.str);
        replicatorStats.samplePacketsSent(desc.category.str, outBitStream.GetNumberOfBytesUsed() - byteStart);
    }
    if (DFFlag::DebugLogProcessCharacterRequestTime)
    {
        if (pendingCharaterRequest && (pendingCharaterRequest == instance) && (desc.name == "Character"))
        {
            pendingCharaterRequest = NULL;
        }
    }
}

void ServerReplicator::readPropAcknowledgement(RakNet::BitStream& inBitstream)
{
    int version;
    inBitstream >> version;

    const Reflection::PropertyDescriptor* propertyDescriptor;
    propDictionary.receive(inBitstream, propertyDescriptor, false);

    shared_ptr<Instance> instance;
    Aya::Guid::Data id;
    if (deserializeInstanceRef(inBitstream, instance, id))
        propSync.onReceivedAcknowledgement(Reflection::ConstProperty(*propertyDescriptor, instance.get()), version);
}

static bool mightRunOnClient(const Instance* instance)
{
    if (!instance)
        return false;

    if (Instance::fastDynamicCast<const LocalScript>(instance))
        return true;

    return false;
}

void ServerReplicator::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
    if (streamJob)
    {
        TaskScheduler::singleton().remove(streamJob);
        streamJob->unregisterCoarsePrimitiveCallback();
        streamJob.reset();
    }

    if (sendStatsJob)
    {
        TaskScheduler::singleton().remove(sendStatsJob);
        sendStatsJob.reset();
    }

    if (newProvider)
    {
        placeAuthenticationState = PlaceAuthenticationState_Init;
        Workspace* workspace = newProvider->find<Workspace>();
        if (!workspace)
            throw std::runtime_error("ServerReplicator unable to find workspace.");

        if (workspace->getNetworkStreamingEnabled())
        {
            streamingEnabled = true;
            streamJob = shared_ptr<Replicator::StreamJob>(new Replicator::StreamJob(*this));
            TaskScheduler::singleton().add(streamJob);
        }

        if (workspace->getNetworkFilteringEnabled())
            strictFilter.reset(new StrictNetworkFilter(this));
    }

    // TODO: FIX THIS BEFORE INTEGRATING TO CI
    // addTopReplicationContainers needs streamJob to be set.
    // Ideally addTopReplicationContainers should only be called by server replicators.
    // If addTopReplicationContainers was called here, then we could manage to initialize
    // streamJob in the right order.
    Super::onServiceProvider(oldProvider, newProvider);
}

static void serializeSFFlag(const std::string& name, const std::string& varValue, void* context)
{
    RakNet::BitStream* bitStream = reinterpret_cast<RakNet::BitStream*>(context);
    // Aya::StandardOut::singleton()->printf(Aya::MESSAGE_INFO,
    //     "Sent FFLag: %s: %s", name.c_str(), varValue.c_str());
    RakNet::RakString rakName = name.c_str();
    RakNet::RakString rakValue = varValue.c_str();
    bitStream->Write(rakName);
    bitStream->Write(rakValue);
    FLog::SetValueFromServer(name, varValue);
}

void ServerReplicator::serializeSFFlags(RakNet::BitStream& outBitStream) const
{
    outBitStream.Write(FLog::GetNumSynchronizedVariable());
    FLog::ForEachVariable(&serializeSFFlag, &outBitStream, FASTVARTYPE_SYNC);
}

void ServerReplicator::sendDictionaries()
{
    rakPeer->rawPeer()->Send(
        &getApiDictionaryBitStream(), settings().getDataSendPriority(), DATAMODEL_RELIABILITY, DATA_CHANNEL, remotePlayerId, false);
}

using namespace Aya::Reflection;
void ServerReplicator::writeDescriptorSchema(const ClassDescriptor* classDesc, RakNet::BitStream& bitStream) const
{
    unsigned int classId = classDictionary.getId(classDesc).id;
#ifdef NETWORK_DEBUG
    // StandardOut::singleton()->printf(Aya::MESSAGE_INFO, "Class name: %s, id: %d", classDesc->name.toString().c_str(), classId);
#endif

    RakNet::RakString className = classDesc->name.c_str();
    bitStream << classId;       // uint
    bitStream.Write(className); // rakStr
    bitStream << (unsigned char)classDesc->getReplicationLevel();
    bitStream << (unsigned int)classDesc->Reflection::MemberDescriptorContainer<Reflection::PropertyDescriptor>::descriptor_size(); // uint
    Reflection::MemberDescriptorContainer<Reflection::PropertyDescriptor>::Collection::const_iterator propIter =
        classDesc->Reflection::MemberDescriptorContainer<Reflection::PropertyDescriptor>::descriptors_begin();
    for (; propIter != classDesc->Reflection::MemberDescriptorContainer<Reflection::PropertyDescriptor>::descriptors_end(); propIter++)
    {
        const Reflection::PropertyDescriptor* propDesc = *propIter;
        unsigned int propId = propDictionary.getId(propDesc).id;
        RakNet::RakString propName = propDesc->name.c_str();
        unsigned int typeId = typeDictionary.getId(&propDesc->type).id;
        RakNet::RakString propType = propDesc->type.name.c_str();
        bitStream << propId;                   // uint
        bitStream.Write(propName);             // rakStr
        bitStream << typeId;                   // unit
        bitStream.Write(propType);             // rakStr
        bitStream << propDesc->canReplicate(); // bool
        bitStream << propDesc->bIsEnum;        // bool
        if (propDesc->bIsEnum)
        {
            const EnumPropertyDescriptor& enumDesc = static_cast<const EnumPropertyDescriptor&>(*propDesc);
            bitStream << (unsigned int)enumDesc.enumDescriptor.getEnumCountMSB(); // uint
        }
        // StandardOut::singleton()->printf(Aya::MESSAGE_INFO, "    Prop name: %s, type: %s, id: %d"
        //     , propDesc->name.c_str()
        //     , propDesc->type.name.c_str()
        //     , propertyId
        //     );
    }

    bitStream << (unsigned int)classDesc->Reflection::MemberDescriptorContainer<Reflection::EventDescriptor>::descriptor_size(); // uint
    Reflection::MemberDescriptorContainer<Reflection::EventDescriptor>::Collection::const_iterator eventIter =
        classDesc->Reflection::MemberDescriptorContainer<Reflection::EventDescriptor>::descriptors_begin();
    for (; eventIter != classDesc->Reflection::MemberDescriptorContainer<Reflection::EventDescriptor>::descriptors_end(); eventIter++)
    {
        const Reflection::EventDescriptor* eventDesc = *eventIter;
        const Reflection::SignatureDescriptor& signatureDescriptor = eventDesc->getSignature();
        unsigned int eventId = eventDictionary.getId(eventDesc).id;
        bitStream << eventId; // uint
        RakNet::RakString eventName = eventDesc->name.c_str();
        bitStream.Write(eventName);                                      // rakStr
        bitStream << (unsigned int)signatureDescriptor.arguments.size(); // unit
        for (std::list<Reflection::SignatureDescriptor::Item>::const_iterator typeIter = signatureDescriptor.arguments.begin();
            typeIter != signatureDescriptor.arguments.end(); ++typeIter)
        {
            unsigned int typeId = typeDictionary.getId(&(*typeIter->type)).id;
            bitStream << typeId;
        }
        // StandardOut::singleton()->printf(Aya::MESSAGE_INFO, "    Event name: %s, id: %d, size: %d"
        //     , eventDesc->name.c_str()
        //     , eventId
        //     , signatureDescriptor.arguments.size()
        //     );
    }
}

RakNet::BitStream ServerReplicator::apiSchemaBitStream;

void ServerReplicator::generateSchema(const ServerReplicator* serverRep, bool force)
{
    if (force)
    {
        // for unit tests
        // since we will be testing with different API sets in unit tests, we will have different schema
        apiSchemaBitStream.Reset();
    }
    else if (apiSchemaBitStream.GetNumberOfBytesUsed() > 0)
    {
        // already generated
        return;
    }

    apiSchemaBitStream << (unsigned char)ID_SCHEMA_SYNC;

    RakNet::BitStream bitStream;

    // first, teach all enums
    bitStream << (unsigned int)EnumDescriptor::allEnumSize(); // uint
    std::vector<const EnumDescriptor*>::const_iterator enumIter = EnumDescriptor::enumsBegin();
    std::vector<const EnumDescriptor*>::const_iterator enumEnd = EnumDescriptor::enumsEnd();
    while (enumIter != enumEnd)
    {
        const EnumDescriptor* e = *enumIter;
        RakNet::RakString enumName = e->name.c_str();
        bitStream.Write(enumName);                       // rakStr
        bitStream << (unsigned int)e->getEnumCountMSB(); // uint
        ++enumIter;
    }

    // then teach all classes
    bitStream << (unsigned int)ClassDescriptor::all_size(); // uint
    ClassDescriptor::ClassDescriptors::const_iterator iter = ClassDescriptor::all_begin();
    ClassDescriptor::ClassDescriptors::const_iterator end = ClassDescriptor::all_end();
    while (iter != end)
    {
        serverRep->writeDescriptorSchema(*iter, bitStream);
        ++iter;
    }

    compressBitStream(bitStream, apiSchemaBitStream, 9);
}

RakNet::BitStream ServerReplicator::apiDictionaryBitStream;

void ServerReplicator::generateApiDictionary(const ServerReplicator* serverRep, bool force)
{
    bool teachSchema = true;
    if (force)
    {
        apiDictionaryBitStream.Reset();
    }
    else if (apiDictionaryBitStream.GetNumberOfBytesUsed() > 0)
    {
        return;
    }
    apiDictionaryBitStream << (unsigned char)ID_TEACH_DESCRIPTOR_DICTIONARIES;

    RakNet::BitStream bitStream;
    teachDictionaries(serverRep, bitStream, teachSchema, true);

    compressBitStream(bitStream, apiDictionaryBitStream, 9);
}

const RakNet::BitStream& ServerReplicator::getSchemaBitStream() const
{
    AYAASSERT(apiSchemaBitStream.GetNumberOfBytesUsed() > 0);
    return apiSchemaBitStream;
}

const RakNet::BitStream& ServerReplicator::getApiDictionaryBitStream() const
{
    AYAASSERT(apiDictionaryBitStream.GetNumberOfBytesUsed() > 0);
    return apiDictionaryBitStream;
}

void ServerReplicator::teachSchema()
{
#ifdef NETWORK_DEBUG
    StandardOut::singleton()->printf(Aya::MESSAGE_INFO, "ServerReplicator::teachSchema()");
#endif
    FASTLOG(FLog::Network, "ServerReplicator::teachSchema()");

    rakPeer->rawPeer()->Send(&getSchemaBitStream(), settings().getDataSendPriority(), DATAMODEL_RELIABILITY, DATA_CHANNEL, remotePlayerId, false);
}

bool ServerReplicator::isProtectedStringEnabled()
{
    return false;
}

std::string ServerReplicator::encodeProtectedString(
    const ProtectedString& value, const Instance* instance, const Reflection::PropertyDescriptor& desc)
{
    if (LuaVM::useSecureReplication())
    {
        boost::optional<long> index = server->getScriptIndexForSource(value.getSource());

        if (!index)
        {
            return std::string();
        }
        else if (desc == Script::prop_EmbeddedSourceCode && instance && !mightRunOnClient(instance))
        {
            return StringConverter<long>::convertToString(*index);
        }
        else if (canUseProtocolVersion(28))
        {
            boost::optional<std::string> bytecode = server->getScriptBytecodeForIndex(*index, canUseProtocolVersion(33));
            AYAASSERT(bytecode);

            return *bytecode;
        }
        else
        {
            return value.getSource();
        }
    }
    else
    {
        return value.getSource();
    }
}

boost::optional<ProtectedString> ServerReplicator::decodeProtectedString(
    const std::string& value, const Instance* instance, const Reflection::PropertyDescriptor& desc)
{
    if (LuaVM::useSecureReplication())
    {
        if (desc == Script::prop_EmbeddedSourceCode && instance && !mightRunOnClient(instance))
        {
            long index;

            if (StringConverter<long>::convertToValue(value, index))
                return server->getScriptSourceForIndex(index);
            else
                return boost::optional<ProtectedString>();
        }
        else if (canUseProtocolVersion(28))
        {
            boost::optional<long> index = server->getScriptIndexForBytecode(value, canUseProtocolVersion(33));

            if (index)
                return server->getScriptSourceForIndex(*index);
            else
                return boost::optional<ProtectedString>();
        }
        else
            return ProtectedString::fromTrustedSource(value);
    }
    else
    {
        return ProtectedString::fromTrustedSource(value);
    }
}
