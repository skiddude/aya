#include "Replicator.ReferencePropertyChangedItem.hpp"

#include "Item.hpp"
#include "Replicator.hpp"

#include "Reflection/Property.hpp"

#include "RakNet/BitStream.hpp"

namespace Aya
{
namespace Network
{

Replicator::ReferencePropertyChangedItem::ReferencePropertyChangedItem(
    Replicator* replicator, const shared_ptr<const Instance>& instance, const Reflection::RefPropertyDescriptor& desc)
    : PooledItem(*replicator)
    , instance(instance)
    , desc(desc)
{

    Instance* refInstance = DescribedBase::fastDynamicCast<Instance>(desc.getRefValue(instance.get()));
    if (refInstance != NULL)
    {
        refInstance->getGuid().extract(newValueGuid);
    }
    else
    {
        newValueGuid.scope.setNull();
    }
}

bool Replicator::ReferencePropertyChangedItem::write(RakNet::BitStream& bitStream)
{
    replicator.writeChangedRefProperty(instance.get(), desc, newValueGuid, bitStream);
    return true;
}

} // namespace Network
} // namespace Aya
