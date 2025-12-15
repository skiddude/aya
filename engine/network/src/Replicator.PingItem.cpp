#include "Replicator.PingItem.hpp"

#include "Item.hpp"
#include "Replicator.hpp"
#include "ReplicatorStats.hpp"
#include "Security/FuzzyTokens.hpp"
#include "Security/ApiSecurity.hpp"
#include "DataModel/HackDefines.hpp"


#include "FastLog.hpp"
#include "DataModel/DataModel.hpp"


#include "RakNet/RakNetTime.hpp"
#include "RakNet/BitStream.hpp"



namespace Aya
{
namespace Network
{

DeserializedPingItem::DeserializedPingItem()
    : extraStats(0)
{
    type = Item::ItemTypePing;
}

void DeserializedPingItem::process(Replicator& replicator)
{
    replicator.readDataPingItem(this);
}

Replicator::PingItem::PingItem(Replicator* replicator, RakNet::Time time, unsigned int extraStats)
    : PooledItem(*replicator)
    , time(time)
    , extraStats(extraStats)
{
}

bool Replicator::PingItem::write(RakNet::BitStream& bitStream)
{
    boost::scoped_ptr<unsigned int> moreStatsCopy(new unsigned int);

    int byteStart = bitStream.GetNumberOfBytesUsed();

    writeItemType(bitStream, ItemTypePing);

    unsigned int moreStats = 0;
#if !defined(LOVE_ALL_ACCESS) && !defined(AYA_STUDIO) && !defined(AYA_PLATFORM_DURANGO)
    unsigned int sendStats =
        Tokens::simpleToken | DataModel::perfStats | DataModel::sendStats | DataModel::get(&replicator)->allHackFlagsOredTogether();
    // I'm sorry for this.  The values need to be spread out in memory and thus can't be an array.
    moreStats |= Aya::Security::getHackFlag<LINE_RAND4>(Aya::Security::hackFlag0);
    moreStats |= Aya::Security::getHackFlag<LINE_RAND4>(Aya::Security::hackFlag1);
    moreStats |= Aya::Security::getHackFlag<LINE_RAND4>(Aya::Security::hackFlag2);
    moreStats |= Aya::Security::getHackFlag<LINE_RAND4>(Aya::Security::hackFlag3);
    moreStats |= Aya::Security::getHackFlag<LINE_RAND4>(Aya::Security::hackFlag4);
    moreStats |= Aya::Security::getHackFlag<LINE_RAND4>(Aya::Security::hackFlag5);
    moreStats |= Aya::Security::getHackFlag<LINE_RAND4>(Aya::Security::hackFlag6);
    moreStats |= Aya::Security::getHackFlag<LINE_RAND4>(Aya::Security::hackFlag7);
    moreStats |= Aya::Security::getHackFlag<LINE_RAND4>(Aya::Security::hackFlag8);
    moreStats |= Aya::Security::getHackFlag<LINE_RAND4>(Aya::Security::hackFlag9);
    moreStats |= Aya::Security::getHackFlag<LINE_RAND4>(Aya::Security::hackFlag10);
    moreStats |= Aya::Security::getHackFlag<LINE_RAND4>(Aya::Security::hackFlag11);
    moreStats |= Aya::Security::getHackFlag<LINE_RAND4>(Aya::Security::hackFlag12);
    moreStats |= sendStats;
    *moreStatsCopy = moreStats;
#endif
    bitStream << false;
#if !defined(__linux) && !defined(__APPLE__)
    bitStream << time;
#else
    bitStream << static_cast<unsigned long long>(time);
#endif

    bitStream.Write(static_cast<uint32_t>(moreStats));
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

#if !defined(AYA_STUDIO)
    if (*moreStatsCopy != moreStats)
    {
        Aya::Tokens::apiToken.addFlagSafe(Aya::kPingItem); // can be changed to addFlagFast later.
    }

#endif
    return true;
}

shared_ptr<DeserializedItem> Replicator::PingItem::read(Replicator& replicator, RakNet::BitStream& inBitstream)
{
    shared_ptr<DeserializedPingItem> deserializedData(new DeserializedPingItem());

    int start = inBitstream.GetReadOffset();

    inBitstream >> deserializedData->pingBack;
    inBitstream >> deserializedData->time;
    inBitstream >> deserializedData->sendStats;
    if (replicator.canUseProtocolVersion(34))
    {
        inBitstream >> deserializedData->extraStats;
#ifdef AYA_SERVER
        if (deserializedData->time & 0x20) // change things up
        {
            deserializedData->extraStats = ~deserializedData->extraStats;
        }
#endif
    }

    if (replicator.settings().trackDataTypes)
    {
        replicator.replicatorStats.incrementPacketsReceived(ReplicatorStats::PACKET_TYPE_Ping);
        replicator.replicatorStats.samplePacketsReceived(ReplicatorStats::PACKET_TYPE_Ping, (inBitstream.GetReadOffset() - start) / 8);
    }

    return deserializedData;
}

} // namespace Network
} // namespace Aya
