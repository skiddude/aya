#pragma once

#include "Item.hpp"
#include "Replicator.hpp"

#include "RakNet/RakNetTime.hpp"

namespace Aya
{
namespace Network
{

class Replicator::PingBackItem : public PooledItem
{
    RakNet::Time time;
    unsigned int extraStats;

public:
    PingBackItem(Replicator* replicator, RakNet::Time time, unsigned int extraStats);

    /*implement*/ virtual bool write(RakNet::BitStream& bitStream);
};

} // namespace Network
} // namespace Aya
