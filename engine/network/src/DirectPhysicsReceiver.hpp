#pragma once

#include "PhysicsReceiver.hpp"
#include "ReplicatorStats.hpp"

namespace Aya
{

namespace Network
{

class DirectPhysicsReceiver : public PhysicsReceiver
{
private:
    MechanismItem tempItem;

public:
    DirectPhysicsReceiver(Replicator* replicator, bool isServer)
        : PhysicsReceiver(replicator, isServer)
    {
    }
    virtual void receivePacket(RakNet::BitStream& bitsream, RakNet::Time timeStamp, ReplicatorStats::PhysicsReceiverStats* stats);
};

} // namespace Network
} // namespace Aya