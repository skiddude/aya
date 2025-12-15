

#include "Client.hpp"
#include "ClientReplicator.hpp"

#include "RakNet/PacketLogger.hpp"
#include "Util.hpp"
#include "ConcurrentRakPeer.hpp"
#include "Players.hpp"

#include "NetworkOwner.hpp"

#include "Utility/StandardOut.hpp"

#include "Utility/ProtectedString.hpp"

#include "Utility/RbxStringTable.hpp"

#include "CPUCount.hpp"
#include "FastLog.hpp"
#include "AyaDbgInfo.hpp"

#include "DataModel/HackDefines.hpp"

#include "DataModel/Workspace.hpp"

#include "DataModel/DataModel.hpp"

#include "DataModel/DebugSettings.hpp"

#include "DataModel/TeleportService.hpp"


#include "Script/ScriptContext.hpp"

#include "RakNet/RakNetStatistics.hpp"

#include "Utility/AyaService.hpp"

LOGGROUP(US14116)
DYNAMIC_FASTFLAG(DebugDisableTimeoutDisconnect)
FASTFLAG(DebugLocalRccServerConnection)

DYNAMIC_LOGGROUP(NetworkJoin)
FASTFLAG(DebugProtocolSynchronization)

#ifndef _WIN32
// For inet_addr() call used below
#include <arpa/inet.h>
#endif

const char* const Aya::Network::sClient = "NetworkClient";

namespace Aya
{
extern const char* const sHopper;
class Instance;
} // namespace Aya

using namespace Aya;
using namespace Aya::Network;
using namespace RakNet;


Reflection::BoundProp<std::string> Client::prop_Ticket("Ticket", "Authentication", &Client::ticket);
static Reflection::BoundFuncDesc<Client, shared_ptr<Instance>(int, std::string, int, int, int)> f_connect(
    &Client::playerConnect, "PlayerConnect", "userId", "server", "serverPort", "clientPort", 0, "threadSleepTime", 30, Security::Plugin);
static Reflection::BoundFuncDesc<Client, void(int)> f_disconnect(&Client::disconnect, "Disconnect", "blockDuration", 3000, Security::LocalUser);
static Reflection::BoundFuncDesc<Client, void(std::string)> func_setGameSessionID(
    &Client::setGameSessionID, "SetGameSessionID", "gameSessionID", Security::Roblox);
static Reflection::EventDesc<Client, void(std::string, shared_ptr<Aya::Instance>)> event_ConnectionAccepted(
    &Client::connectionAcceptedSignal, "ConnectionAccepted", "peer", "replicator");
static Reflection::EventDesc<Client, void(std::string)> event_ConnectionRejected(&Client::connectionRejectedSignal, "ConnectionRejected", "peer");
static Reflection::EventDesc<Client, void(std::string, int, std::string)> event_ConnectionFailed(
    &Client::connectionFailedSignal, "ConnectionFailed", "peer", "code", "reason");
REFLECTION_END();

Client::Client()
    : userId(-1)
    , networkSettings(&NetworkSettings::singleton())
{
    Aya::Security::Context::current().requirePermission(Aya::Security::Plugin, "create a NetworkClient");
    setName(sClient);

    FASTLOG(FLog::Network, "NetworkClient:Create");
}

Client::~Client(void)
{
    FASTLOG(FLog::Network, "NetworkClient:Remove");
}

Client* Client::findClient(const Aya::Instance* context, bool testInDatamodel)
{
    const ServiceProvider* serviceProvider = ServiceProvider::findServiceProvider(context);
    AYAASSERT(!testInDatamodel || serviceProvider != NULL);
    return ServiceProvider::find<Client>(serviceProvider);
}

bool Client::clientIsPresent(const Aya::Instance* context, bool testInDatamodel)
{
    return findClient(context, testInDatamodel) != NULL;
}

bool Client::physicsOutBandwidthExceeded(const Aya::Instance* context)
{
    if (Client* client = Client::findClient(context))
    {
        if (ClientReplicator* clientRep = client->findFirstChildOfType<ClientReplicator>())
        {
            return clientRep->isLimitedByOutgoingBandwidthLimit();
        }
    }
    return true;
}

double Client::getNetworkBufferHealth(const Aya::Instance* context)
{
    if (Client* client = Client::findClient(context))
    {
        return client->rakPeer->GetBufferHealth();
    }
    return 0.0f;
}

const Aya::SystemAddress Client::findLocalSimulatorAddress(const Aya::Instance* context)
{
    if (Client* client = Client::findClient(context, false))
    {
        if (ClientReplicator* clientRep = client->findFirstChildOfType<ClientReplicator>())
        {
            return RakNetToRbxAddress(clientRep->getClientAddress());
        }
    }
    return Network::NetworkOwner::Unassigned();
}

shared_ptr<Instance> Client::playerConnect(int userId, std::string server, int serverPort, int clientPort, int threadSleepTime)
{
    FASTLOG3(FLog::Network, "Client:Connect serverPort(%d) clientPort(%d) threadSleepTime(%d)", serverPort, clientPort, threadSleepTime);

    this->userId = userId;
    Players* players = ServiceProvider::create<Players>(this);
    if (!players)
        throw Aya::runtime_error("Cannot get players");

    shared_ptr<Instance> player = players->createLocalPlayer(userId, TeleportService::getPreviousPlaceId() > 0);

    if (clientPort == 0)
    {
        clientPort = networkSettings->preferredClientPort;
    }

    RakNet::SocketDescriptor d(clientPort, "");
    StartupResult startRes = rakPeer->rawPeer()->Startup(1, &d, 1);
    if (startRes != RakNet::RAKNET_STARTED)
    {
        if (clientPort == 0)
        {
            throw Aya::runtime_error("Failed to start the network client");
        }
        else
        {
            throw Aya::runtime_error("Failed to start the network client on port %d", clientPort);
        }
    }

    // allow local and LAN games only.
    if (server != "localhost")
    {
        bool lansubnet = false;
        for (int i = 0; !lansubnet && i < MAXIMUM_NUMBER_OF_INTERNAL_IDS; ++i)
        {
            RakNet::SystemAddress localAddress = rakPeer->rawPeer()->GetInternalID(RakNet::UNASSIGNED_SYSTEM_ADDRESS, i);
            RakNet::SystemAddress remoteAddress(server.c_str(), serverPort);

            // match the a and b records
            lansubnet = (localAddress.GetBinaryAddress() & 0x00FF) == (remoteAddress.GetBinaryAddress() & 0x00FF);
        }
        if (!FFlag::DebugLocalRccServerConnection)
        {
            if (!lansubnet)
            {
                Aya::Security::Context::current().requirePermission(Aya::Security::Roblox, " connect to an extranet game");
            }
        }
    }

    RakNet::ConnectionAttemptResult connectRes =
        rakPeer->rawPeer()->Connect(server.c_str(), serverPort, Network::password.c_str(), Network::password.size());
    if (connectRes != RakNet::CONNECTION_ATTEMPT_STARTED)
    {
        throw Aya::runtime_error("Failed to connect to server, id %d", connectRes);
    }
    FASTLOG1F(DFLog::NetworkJoin, "playerConnect connecting to server @ %f s", Time::nowFastSec());

    if (DFFlag::DebugDisableTimeoutDisconnect || AyaService::localServer)
        rakPeer->rawPeer()->SetTimeoutTime(10 * 60 * 1000, UNASSIGNED_SYSTEM_ADDRESS);


    // StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "Connecting to %s:%d", server.c_str(), serverPort);

    FASTLOG2(FLog::Network, "Connecting to server, IP(inet_addr): %u Port: %u", inet_addr(server.c_str()), serverPort);

    Aya::AyaDbgInfo::SetServerIP(server.c_str());

    return player;
}



void Client::disconnect(int blockDuration)
{
    FASTLOG(FLog::Network, "Client:Disconnect");

    // The following line will remove the Replicator
    this->visitChildren(boost::bind(&Instance::unlockParent, _1));
    this->removeAllChildren();

    if (rakPeer)
    {
        rakPeer->rawPeer()->CloseConnection(this->serverId, true);
        rakPeer->rawPeer()->Shutdown(blockDuration);
    }
}

void Client::setGameSessionID(std::string value)
{
    if (value != Http::gameSessionID)
    {
        Http::gameSessionID = value;
    }
}

void Client::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
    if (oldProvider)
    {
        closingConnection.disconnect();

        disconnect(); // We should have disconnected by now (in response to the Closing event)

        Players* players = ServiceProvider::find<Players>(oldProvider);
        players->setConnection(NULL);
    }

    Super::onServiceProvider(oldProvider, newProvider);

    if (newProvider)
    {
        // We're in multiplayer mode, so burn out the studio tools
        if (Aya::DataModel* dataModel = Aya::DataModel::get(this))
        {
            if (dataModel->lockVerb.get())
                dataModel->lockVerb->doIt(NULL);
        }

        Players* players = ServiceProvider::create<Players>(newProvider);
        players->setConnection(rakPeer.get());

        // Disconnect now before we start getting DescendantRemoving events
        // If we don't disconnect first, then we'll send a shower of delete messages
        // to the Server
        closingConnection = newProvider->closingSignal.connect(boost::bind(&Client::disconnect, this));
    }
}

void Client::sendVersionInfo()
{
    RakNet::BitStream bitStream;
    bitStream << (unsigned char)ID_PROTOCOL_SYNC;
    bitStream << protocolVersion;

    rakPeer->rawPeer()->Send(&bitStream, networkSettings->getDataSendPriority(), DATAMODEL_RELIABILITY, DATA_CHANNEL, serverId, false);
}

void Client::sendTicket()
{
    RakNet::BitStream bitStream;
    bitStream << (unsigned char)ID_SUBMIT_TICKET;

    bitStream << userId;
    serializeStringCompressed(ticket, bitStream);

    serializeStringCompressed(Aya::DataModel::hash, bitStream);

    bitStream << protocolVersion;

    serializeStringCompressed(password, bitStream);

    // TODO: better way to track protocol changes between versions
    // Network Protocol version 2
    serializeStringCompressed(DebugSettings::singleton().osPlatform(), bitStream);
    serializeStringCompressed(DebugSettings::singleton().getRobloxProductName(), bitStream);

    serializeStringCompressed(Http::gameSessionID, bitStream);

    encryptDataPart(bitStream);

    // Send ID_SUBMIT_TICKET
    rakPeer->rawPeer()->Send(&bitStream, networkSettings->getDataSendPriority(), DATAMODEL_RELIABILITY, DATA_CHANNEL, serverId, false);
}

std::string rakIdToString(int id)
{
    switch (id)
    {
    case ID_INVALID_PASSWORD:
    case ID_HASH_MISMATCH:
        return "Aya version is out of date. Please uninstall and try again.";
    case ID_CONNECTION_ATTEMPT_FAILED:
        return "Connection attempt failed.";
    case ID_SECURITYKEY_MISMATCH:
        return "Version not compatible with server. Please uninstall and try again.";
    default:
        return Aya::format("Network error %d", id);
    }
}

void Client::OnFailedConnectionAttempt(RakNet::Packet* packet, RakNet::PI2_FailedConnectionAttemptReason failedConnectionAttemptReason)
{
    std::string message = rakIdToString(packet->data[0]);
    StandardOut::singleton()->printf(
        MESSAGE_SENSITIVE, "Failed to connect to %s. %s\n", RakNetAddressToString(packet->systemAddress).c_str(), message.c_str());
    connectionFailedSignal(RakNetAddressToString(packet->systemAddress), (int)packet->data[0], message);
}

void Client::sendPreferedSpawnName() const
{
    RakNet::BitStream bitStream;

    bitStream << (unsigned char)ID_SPAWN_NAME;

    serializeStringCompressed(TeleportService::GetSpawnName(), bitStream);

    FASTLOGS(FLog::Network, "serverId: %s", RakNetAddressToString(serverId).c_str());

    rakPeer->rawPeer()->Send(&bitStream, networkSettings->getDataSendPriority(), DATAMODEL_RELIABILITY, DATA_CHANNEL, serverId, false);
}

void Client::HandleConnection(RakNet::Packet* packet)
{
    shared_ptr<Replicator> proxy;
    try
    {
        // send previous placeId
        RakNet::BitStream bitStream;
        bitStream << (unsigned char)ID_PLACEID_VERIFICATION;

        bitStream << TeleportService::getPreviousPlaceId();

        rakPeer->rawPeer()->Send(&bitStream, networkSettings->getDataSendPriority(), DATAMODEL_RELIABILITY, DATA_CHANNEL, serverId, false);

        sendTicket();

        sendPreferedSpawnName();

        Workspace* workspace = Workspace::findWorkspace(this);
        workspace->clearTerrain();

        proxy =
            Creatable<Instance>::create<ClientReplicator>(packet->systemAddress, this, rakPeer->rawPeer()->GetExternalID(serverId), networkSettings);

        proxy->setAndLockParent(this);

        connectionAcceptedSignal(RakNetAddressToString(packet->systemAddress), proxy);
    }
    catch (Aya::base_exception& e)
    {
        Aya::StandardOut::singleton()->printf(Aya::MESSAGE_ERROR, "Error in ID_CONNECTION_REQUEST_ACCEPTED: %s", e.what());
        if (proxy)
        {
            // Disconnect
            proxy->unlockParent();
            proxy->setParent(NULL);
        }
    }
}

RakNet::PluginReceiveResult Client::OnReceive(RakNet::Packet* packet)
{
    RakNet::PluginReceiveResult result = Super::OnReceive(packet);
    if (result != RR_CONTINUE_PROCESSING)
        return result;

    switch ((unsigned char)packet->data[0])
    {
    case ID_CONNECTION_REQUEST_ACCEPTED:
    {
        StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "Connection accepted from %s\n", RakNetAddressToString(packet->systemAddress).c_str());

        serverId = packet->systemAddress;

        HandleConnection(packet);
        sendVersionInfo();
    }
        return RR_CONTINUE_PROCESSING;

    case ID_INVALID_PASSWORD:
        StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "Invalid password from %s", RakNetAddressToString(packet->systemAddress).c_str());
        connectionFailedSignal(RakNetAddressToString(packet->systemAddress), (int)packet->data[0], rakIdToString(packet->data[0]));
        connectionRejectedSignal(RakNetAddressToString(packet->systemAddress));
        return RR_STOP_PROCESSING_AND_DEALLOCATE;

    case ID_HASH_MISMATCH:
    case ID_SECURITYKEY_MISMATCH:
        connectionFailedSignal(RakNetAddressToString(packet->systemAddress), (int)packet->data[0], rakIdToString(packet->data[0]));
        return RR_STOP_PROCESSING_AND_DEALLOCATE;

    case ID_DISCONNECTION_NOTIFICATION:
    case ID_CONNECTION_LOST:
        AYAASSERT(packet->systemAddress == serverId);
        serverId = UNASSIGNED_SYSTEM_ADDRESS;
        return RR_CONTINUE_PROCESSING;

    default:
        return RR_CONTINUE_PROCESSING;
    }
}
