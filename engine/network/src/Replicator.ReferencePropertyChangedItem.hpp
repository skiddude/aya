#pragma once

#include "Item.hpp"
#include "Replicator.hpp"

namespace Aya
{

class Instance;
namespace Reflection
{
class RefPropertyDescriptor;
}

namespace Network
{

class Replicator::ReferencePropertyChangedItem : public PooledItem
{
    const shared_ptr<const Instance> instance;
    const Reflection::RefPropertyDescriptor& desc;
    bool newValueIsNull;
    Guid::Data newValueGuid;

public:
    ReferencePropertyChangedItem(Replicator* replicator, const shared_ptr<const Instance>& instance, const Reflection::RefPropertyDescriptor& desc);

    /*implement*/ virtual bool write(RakNet::BitStream& bitStream);
};

} // namespace Network
} // namespace Aya
