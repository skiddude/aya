

#pragma once

#include "Peer.hpp"
#include "Utility/SystemAddress.hpp"



namespace Aya
{

namespace Network
{

extern const char* const sClient;
class Client
    : public DescribedCreatable<Client, Peer, sClient, Reflection::ClassDescriptor::INTERNAL_LOCAL>
    , public Service
{
private:
    typedef DescribedCreatable<Client, Peer, sClient, Reflection::ClassDescriptor::INTERNAL_LOCAL> Super;

    Aya::signals::scoped_connection closingConnection;
    RakNet::SystemAddress serverId;
    int userId;
    std::string ticket;
    NetworkSettings* networkSettings;

    static Reflection::BoundProp<std::string> prop_Ticket;

public:
    Client();
    ~Client();

    Aya::signal<void(std::string, shared_ptr<Instance>)> connectionAcceptedSignal;
    Aya::signal<void(std::string, int, std::string)> connectionFailedSignal;
    Aya::signal<void(std::string)> connectionRejectedSignal;

    void setTicket(const std::string& t)
    {
        ticket = t;
    }

    shared_ptr<Instance> playerConnect(int userId, std::string server, int serverPort, int clientPort, int threadSleepTime);
    void disconnect(int blockDuration);
    void disconnect()
    {
        disconnect(3000);
    }
    void setGameSessionID(std::string gameSessionID);

    /*override*/ RakNet::PluginReceiveResult OnReceive(RakNet::Packet* packet);
    /*override*/ void OnFailedConnectionAttempt(RakNet::Packet* packet, RakNet::PI2_FailedConnectionAttemptReason failedConnectionAttemptReason);

    static Client* findClient(const Aya::Instance* context, bool testInDatamodel = true);
    static bool clientIsPresent(const Aya::Instance* context, bool testInDatamodel = true);
    static const Aya::SystemAddress findLocalSimulatorAddress(const Aya::Instance* context); // if Client == clientAddress, else == NULL;
    static bool physicsOutBandwidthExceeded(const Aya::Instance* context);
    static double getNetworkBufferHealth(const Aya::Instance* context);

protected:
    virtual void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

private:
    void sendVersionInfo();
    void sendTicket();
    void sendPreferedSpawnName() const;
    void HandleConnection(RakNet::Packet* packet);
};
} // namespace Network
} // namespace Aya
