

#pragma once

#include "Replicator.hpp"

namespace Aya
{
namespace Network
{

struct PropValuePair
{
    const Reflection::PropertyDescriptor* descriptor;
    Reflection::Variant value;

    PropValuePair(const Reflection::PropertyDescriptor& descriptor, const Reflection::Variant& _value)
        : descriptor(&descriptor)
        , value(_value)
    {
    }
};

class DeserializedNewInstanceItem : public DeserializedItem
{
public:
    typedef std::vector<PropValuePair> PropValuePairList;
    PropValuePairList propValueList; // for non creatable instances

    Aya::Guid::Data id;
    Aya::Guid::Data parentId;
    const Reflection::ClassDescriptor* classDescriptor;

    shared_ptr<Instance> instance;
    shared_ptr<Instance> parent;
    bool deleteOnDisconnect;

    DeserializedNewInstanceItem();
    ~DeserializedNewInstanceItem(){};

    void reset()
    {
        propValueList.clear();
        instance.reset();
        parent.reset();
        id.scope.setNull();
        parentId.scope.setNull();
        classDescriptor = NULL;
    }

    /*implement*/ void process(Replicator& replicator);
};

class Replicator::NewInstanceItem : public PooledItem
{
    shared_ptr<const Instance> instance;
    bool useStoredParentGuid;
    Aya::Guid::Data parentIdAtItemCreation;

    bool deleteOnDisconnect;

public:
    NewInstanceItem(Replicator* replicator, shared_ptr<const Instance> instance);

    bool write(RakNet::BitStream& bitStream);
    static bool read(Replicator& replicator, RakNet::BitStream& bitStream, bool isJoinData, DeserializedNewInstanceItem& deserializedData);
    static shared_ptr<DeserializedItem> read(Replicator& replicator, RakNet::BitStream& bitStream, bool isJoinData);
};

} // namespace Network
} // namespace Aya
