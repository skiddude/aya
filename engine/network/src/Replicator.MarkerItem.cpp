#include "Replicator.MarkerItem.hpp"

#include "Item.hpp"
#include "Replicator.hpp"
#include "Util.hpp"
#include "Replicator.StreamJob.hpp"

#include "RakNet/BitStream.hpp"
#include <string>

DYNAMIC_LOGGROUP(NetworkJoin)

namespace Aya
{
namespace Network
{

Replicator::MarkerItem::MarkerItem(Replicator* replicator, int id)
    : Item(*replicator)
    , id(id)
{
}

bool Replicator::MarkerItem::write(RakNet::BitStream& bitStream)
{
    if (!replicator.isInitialDataSent())
        return false;

    writeItemType(bitStream, ItemTypeMarker);

    bitStream << id;
    if (replicator.settings().printInstances)
    {
        Aya::StandardOut::singleton()->printf(
            Aya::MESSAGE_SENSITIVE, "Replication: Sending marker %d to %s", id, RakNetAddressToString(replicator.remotePlayerId).c_str());
    }

    replicator.onSentMarker(id);
    FASTLOG1F(DFLog::NetworkJoin, "MarkerItem %ld sent", id);

    return true;
}

shared_ptr<DeserializedItem> Replicator::MarkerItem::read(Replicator& replicator, RakNet::BitStream& bitStream)
{
    shared_ptr<DeserializedMarkerItem> deserializedData(new DeserializedMarkerItem());

    bitStream >> deserializedData->id;

    if (replicator.settings().printInstances)
    {
        Aya::StandardOut::singleton()->printf(
            Aya::MESSAGE_SENSITIVE, "Received marker %d from %s", deserializedData->id, RakNetAddressToString(replicator.remotePlayerId).c_str());
    }

    return deserializedData;
}

} // namespace Network
} // namespace Aya
