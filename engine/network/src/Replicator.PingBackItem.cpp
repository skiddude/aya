#include "Replicator.PingBackItem.hpp"

#include "Item.hpp"
#include "Replicator.hpp"
#include "ReplicatorStats.hpp"
#include "DataModel/DataModel.hpp"

#include "DataModel/HackDefines.hpp"


#include "RakNet/RakNetTime.hpp"
#include "RakNet/BitStream.hpp"



namespace Aya
{
namespace Network
{

Replicator::PingBackItem::PingBackItem(Replicator* replicator, RakNet::Time time, unsigned int extraStats)
    : PooledItem(*replicator)
    , time(time)
    , extraStats(extraStats)
{
}

bool Replicator::PingBackItem::write(RakNet::BitStream& bitStream)
{

    int byteStart = bitStream.GetNumberOfBytesUsed();

    writeItemType(bitStream, ItemTypePingBack);
    bitStream << true;
#if !defined(__linux) && !defined(__APPLE__)
    bitStream << time;
#else
    bitStream << static_cast<unsigned long long>(time);
#endif

#if !defined(LOVE_ALL_ACCESS)
    unsigned int sendStats = DataModel::sendStats | DataModel::get(&replicator)->allHackFlagsOredTogether();
#else
    unsigned int sendStats = 0;
#endif
    bitStream << sendStats;

    if (replicator.canUseProtocolVersion(34))
    {
#if !defined(AYA_SERVER) && !defined(AYA_STUDIO)
        if (time & 0x20)
        {
            extraStats = ~extraStats;
        }

#endif
        bitStream.Write(static_cast<uint32_t>(extraStats));
    }

    if (replicator.settings().trackDataTypes)
    {
        replicator.replicatorStats.incrementPacketsSent(ReplicatorStats::PACKET_TYPE_Ping);
        replicator.replicatorStats.samplePacketsSent(ReplicatorStats::PACKET_TYPE_Ping, bitStream.GetNumberOfBytesUsed() - byteStart);
    }

    
    return true;
}

} // namespace Network
} // namespace Aya
