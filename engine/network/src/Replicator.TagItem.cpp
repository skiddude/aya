#include "Replicator.TagItem.hpp"

#include "Item.hpp"
#include "Replicator.hpp"
#include "ClientReplicator.hpp"
#include "Util.hpp"
#include "Replicator.StreamJob.hpp"

#include "RakNet/BitStream.hpp"
#include <string>

DYNAMIC_LOGGROUP(NetworkJoin)

namespace Aya
{
namespace Network
{

DeserializedTagItem::DeserializedTagItem()
{
    type = Item::ItemTypeTag;
}

void DeserializedTagItem::process(Replicator& replicator)
{
    ClientReplicator* rep = aya_static_cast<ClientReplicator*>(&replicator);
    rep->readTagItem(this);
}

Replicator::TagItem::TagItem(Replicator* replicator, int id, boost::function<bool()> readyCallback)
    : Item(*replicator)
    , id(id)
    , readyCallback(readyCallback)
{
}

bool Replicator::TagItem::write(RakNet::BitStream& bitStream)
{
    if (readyCallback && !readyCallback())
    {
        return false;
    }

    writeItemType(bitStream, ItemTypeTag);

    bitStream << id;
    if (replicator.settings().printInstances)
    {
        Aya::StandardOut::singleton()->printf(
            Aya::MESSAGE_SENSITIVE, "Replication: Sending tag %d to %s", id, RakNetAddressToString(replicator.remotePlayerId).c_str());
    }

    replicator.onSentTag(id);

    return true;
}

shared_ptr<DeserializedItem> Replicator::TagItem::read(Replicator& replicator, RakNet::BitStream& inBitstream)
{
    shared_ptr<DeserializedTagItem> deserializedData(new DeserializedTagItem());

    inBitstream >> deserializedData->id;

    if (replicator.settings().printInstances)
    {
        Aya::StandardOut::singleton()->printf(
            Aya::MESSAGE_SENSITIVE, "Received tag %d from %s", deserializedData->id, RakNetAddressToString(replicator.remotePlayerId).c_str());
    }

    return deserializedData;
}

} // namespace Network
} // namespace Aya
