#pragma once

#include "Item.hpp"
#include "Replicator.hpp"
#include "Streaming.hpp"

#include <boost/scoped_ptr.hpp>

namespace Aya
{

class Instance;

namespace Network
{

class DeserializedDeleteInstanceItem : public DeserializedItem
{
public:
    Aya::Guid::Data id;

    DeserializedDeleteInstanceItem();
    ~DeserializedDeleteInstanceItem() {}

    /*implement*/ void process(Replicator& replicator);
};

class Replicator::DeleteInstanceItem : public PooledItem
{
    const IdSerializer::Id id;
    struct InstanceDetails
    {
        std::string className;
        std::string guid;
    };
    boost::scoped_ptr<InstanceDetails> details;

public:
    DeleteInstanceItem(Replicator* replicator, const shared_ptr<const Instance>& instance);

    /*implement*/ virtual bool write(RakNet::BitStream& bitStream);
    static shared_ptr<DeserializedItem> read(Replicator& replicator, RakNet::BitStream& bitStream);
};

} // namespace Network
} // namespace Aya
