

#pragma once

#include "Replicator.hpp"
#include "PropertySynchronization.hpp"
#include "threadsafe.hpp"

#include "Utility/MemoryStats.hpp"

#include "Replicator.StreamJob.hpp"

namespace Aya
{

namespace Network
{

const float kMaxIncomePacketWaitTime = 0.25f;

class Client;
class StrictNetworkFilter;
class DeserializedStatsItem;
class DeserializedTagItem;
class DeserializedStreamDataItem;

extern const char* const sClientReplicator;
class ClientReplicator
    : public Aya::DescribedNonCreatable<ClientReplicator, Replicator, sClientReplicator, Reflection::ClassDescriptor::INTERNAL_LOCAL>
{
private:
    typedef Aya::DescribedNonCreatable<ClientReplicator, Replicator, sClientReplicator, Reflection::ClassDescriptor::INTERNAL_LOCAL> Super;

    class RequestCharacterItem;
    class ClientCapacityUpdateItem;
    class ClientStatsItem;
#ifdef ENABLE_VOICE_CHAT
    class SendOpusDataJob;
#endif
    class GCJob;
    friend class ClientStatsItem;
    friend class StatsItem;
    friend class DeserializedStatsItem;
    friend class DeserializedTagItem;
    friend class DeserializedStreamDataItem;

    PropSync::Slave propSync;
    RakNet::SystemAddress clientAddress;
    bool receivedGlobals;
    boost::scoped_ptr<Aya::AutoMemPool> cframePool;
    boost::shared_ptr<GCJob> gcJob;
#ifdef ENABLE_VOICE_CHAT
    boost::shared_ptr<SendOpusDataJob> opusJob;
#endif

    RunningAverage<double> avgInstancesPerStreamData;
    RunningAverage<double> avgStreamDataReadTime;
    RunningAverage<int> avgRequestCount;
    Aya::MemoryStats::MemoryLevel memoryLevel;

    Aya::signals::scoped_connection playerCharacterAddedConnection;
    void onPlayerCharacterAdded();

    shared_ptr<Reflection::ValueTable> readStats(RakNet::BitStream& bitStream);

    // streaming
    double sampleTimer;
    int clientInstanceQuota;
    StreamRegion::Id lastReadStreamId;

    int pendingInstanceRequests;
    int numInstancesRead;
    bool loggedLowMemWarning;
    void readStreamData(RakNet::BitStream& bitStream);
    void readStreamDataItem(DeserializedStreamDataItem* item);
    void processStreamDataRegionId(Replicator::StreamJob::RegionIteratorSuccessor successorBitMask, StreamRegion::Id id);
    bool canUpdateClientCapacity();
    void streamOutTerrain(const Vector3int16& cellPos);
    void streamOutInstance(Instance* part,
        bool deleteImmediately = true); // NOTE: passing false to deleteImmediately requires handling auto joints and deleting the instance externally
    void streamOutPartHelper(const Guid::Data& data, PartInstance* part, shared_ptr<Instance> descendant);
    void streamOutAutoJointHelper(std::vector<shared_ptr<PartInstance>> pendingRemovalParts, shared_ptr<Instance> instance);

    void markerReceived();

protected:
    /*override*/ bool isProtectedStringEnabled();
    /*override*/ std::string encodeProtectedString(
        const ProtectedString& value, const Instance* instance, const Reflection::PropertyDescriptor& desc);
    /*override*/ boost::optional<ProtectedString> decodeProtectedString(
        const std::string& value, const Instance* instance, const Reflection::PropertyDescriptor& desc);

    /*override*/ bool checkDistributedReceive(PartInstance* part);
    /*override*/ bool checkDistributedSend(const PartInstance* part);
    /*override*/ bool checkDistributedSendFast(const PartInstance* part);
    /*override*/ void processPacket(RakNet::Packet* packet);
    /*override*/ bool canSendItems();

    void readTag(RakNet::BitStream& inBitstream);
    void readTagItem(DeserializedTagItem* item);
    void processTag(int tag);

    /*override*/ void readItem(RakNet::BitStream& inBitstream, Aya::Network::Item::ItemType itemType);
    /*override*/ shared_ptr<DeserializedItem> deserializeItem(RakNet::BitStream& inBitstream, Aya::Network::Item::ItemType itemType);
    /*override*/ bool isLegalSendInstance(const Instance* instance);
    /*override*/ bool isLegalSendProperty(Instance* instance, const Reflection::PropertyDescriptor& desc);
    /*override*/ bool isLegalSendEvent(Instance* instance, const Reflection::EventDescriptor& desc);

    /*override*/ void readChangedProperty(RakNet::BitStream& bitStream, Reflection::Property prop);
    /*override*/ void readChangedPropertyItem(DeserializedChangePropertyItem* item, Reflection::Property prop);
    bool processChangedParentPropertyForStreaming(const Guid::Data& parentId, Reflection::Property prop);

    /*override*/ void writeChangedProperty(const Instance* instance, const Reflection::PropertyDescriptor& desc, RakNet::BitStream& outBitStream);
    /*override*/ void writeChangedRefProperty(
        const Instance* instance, const Reflection::RefPropertyDescriptor& desc, const Guid::Data& newRefGuid, RakNet::BitStream& outBitStream);
    /*override*/ void writeProperties(const Instance* instance, RakNet::BitStream& outBitstream, PropertyCacheType cacheType, bool useDictionary);

    /*override*/ Player* findTargetPlayer() const;
    /*override*/ Player* getRemotePlayer() const
    {
        return NULL;
    }
    /*override*/ void dataOutStep();

    /*override*/ void setPropSyncExpiration(double value)
    {
        propSync.setExpiration(Aya::Time::Interval(value));
    }

    /*override*/ shared_ptr<Stats> createStatsItem();

    /*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

    /*override*/ FilterResult filterChangedProperty(Instance* instance, const Reflection::PropertyDescriptor& desc);
    /*override*/ FilterResult filterReceivedParent(Instance* instance, Instance* parent);

    /*override*/ bool wantReplicate(const Instance* source) const;
    /*override*/ void terrainCellChanged(const Voxel::CellChangeInfo& info);
    /*override*/ void onTerrainRegionChanged(const Voxel2::Region& region);

    // protocol schema
    /*override*/ bool ProcessOutdatedChangedProperty(RakNet::BitStream& inBitstream, const Aya::Guid::Data& id, const Instance* instance,
        const Reflection::PropertyDescriptor* propertyDescriptor, unsigned int propId);
    /*override*/ bool ProcessOutdatedProperties(RakNet::BitStream& inBitstream, Instance* instance, PropertyCacheType cacheType, bool useDictionary,
        bool preventBounceBack, std::vector<PropValuePair>* valueArray);
    /*override*/ bool ProcessOutdatedInstance(RakNet::BitStream& inBitstream, bool isJoinData, const Aya::Guid::Data& id,
        const Reflection::ClassDescriptor* classDescriptor, unsigned int classId);
    /*override*/ bool ProcessOutdatedEventInvocation(RakNet::BitStream& inBitstream, const Aya::Guid::Data& id, const Instance* instance,
        const Reflection::EventDescriptor* eventDescriptor, unsigned int eventId);
    /*override*/ bool ProcessOutdatedEnumSerialization(
        const Reflection::Type& type, const Reflection::Variant& value, RakNet::BitStream& outBitStream);
    /*override*/ bool ProcessOutdatedEnumDeserialization(RakNet::BitStream& inBitStream, const Reflection::Type& type, Reflection::Variant& value);
    /*override*/ bool ProcessOutdatedPropertyEnumSerialization(const Reflection::ConstProperty& property, RakNet::BitStream& outBitStream);
    /*override*/ bool ProcessOutdatedPropertyEnumDeserialization(Reflection::Property& property, RakNet::BitStream& inBitStream);
    /*override*/ bool isClassRemoved(const Instance* instance);
    /*override*/ bool isPropertyRemoved(const Instance* instance, const Name& propertyName);
    /*override*/ bool isEventRemoved(const Instance* instance, const Name& eventName);

    bool needGC();
    bool hasEnoughMemoryToReceiveInstances();

public:
    Aya::signal<void()> receivedGlobalsSignal;
    Aya::signal<void()> gameLoadedSignal;
    Aya::signal<void(shared_ptr<const Reflection::ValueTable>)> statsReceivedSignal;

    ClientReplicator(RakNet::SystemAddress systemAddress, Client* client, RakNet::SystemAddress clientAddress, NetworkSettings* networkSettings);
    ~ClientReplicator();

    void requestServerStats(bool request);
    void requestCharacterImpl();
    /*override*/ void requestCharacter();
    /*override*/ void updateClientCapacity();
    /*override*/ RakNet::PluginReceiveResult OnReceive(RakNet::Packet* packet);
    /*override*/ bool canUseProtocolVersion(int protocolVersion) const;
    /*override*/ void receiveCluster(RakNet::BitStream& inBitstream, Instance* instance, bool usingOneQuarterIterator);
    /*override*/ void postProcessPacket();
    /*override*/ shared_ptr<Instance> sendMarker();

    const RakNet::SystemAddress getClientAddress() const
    {
        return clientAddress;
    }

    void writePropAcknowledgementIfNeeded(const Instance* instance, const Reflection::PropertyDescriptor& desc, RakNet::BitStream& outBitStream);

    bool isLimitedByOutgoingBandwidthLimit() const;
    virtual void deserializeSFFlags(RakNet::BitStream& inBitStream);

    void renderStreamedRegions(Adorn* adorn);

    void renderPartMovementPath(Adorn* adorn);

    Aya::MemoryStats::MemoryLevel getMemoryLevel()
    {
        return memoryLevel;
    }
    void updateMemoryStats();

    // streaming debug
    int getNumRegionsToGC() const;
    short getGCDistance() const;
    int getNumStreamedRegions() const;

private:
    // ------------- protocol schema --------------
    class ReflectionPropertyContainer
    {
    public:
        bool needSync;
        unsigned int id;
        const Name& name;
        unsigned int typeId;
        std::string typeName;
        const Reflection::Type* type;
        bool canReplicate;
        size_t enumMSB;
        ReflectionPropertyContainer(unsigned int _id, const Name& _name, unsigned int _typeId, std::string _typeName, const Reflection::Type* _type,
            bool _canReplicate, size_t _enumMSB)
            : id(_id)
            , name(_name)
            , typeId(_typeId)
            , typeName(_typeName)
            , type(_type)
            , canReplicate(_canReplicate)
            , enumMSB(_enumMSB)
            , needSync(false)
        {
        }
    };
    typedef std::vector<shared_ptr<ReflectionPropertyContainer>> ReflectionPropertyList;

    class ReflectionEventContainer
    {
    public:
        bool needSync;
        unsigned int id;
        const Name& name;
        std::vector<const Reflection::Type*> argTypes;
        ReflectionEventContainer(unsigned int _id, const Name& _name)
            : id(_id)
            , name(_name)
            , needSync(false)
        {
        }
    };
    typedef std::vector<shared_ptr<ReflectionEventContainer>> ReflectionEventList;

    class ReflectionClassContainer
    {
    public:
        bool needSync;
        unsigned int id;
        const Name& name;
        Reflection::ReplicationLevel replicationLevel;
        ReflectionPropertyList properties;
        ReflectionEventList events;
        ReflectionClassContainer(unsigned int _id, const Name& _name, Reflection::ReplicationLevel _repLevel)
            : id(_id)
            , name(_name)
            , replicationLevel(_repLevel)
            , needSync(false)
        {
        }
    };

    typedef boost::unordered_map<const Aya::Name*, const shared_ptr<ReflectionClassContainer>> ReflectionClassMap;
    ReflectionClassMap serverClasses;

    class ReflectionEnumContainer
    {
    public:
        size_t enumMSB;
        ReflectionEnumContainer(size_t _enumMSB)
            : enumMSB(_enumMSB)
        {
        }
    };

    typedef boost::unordered_map<const Aya::Name*, ReflectionEnumContainer> ReflectionEnumMap;
    ReflectionEnumMap serverEnums;

    typedef std::map<Aya::Guid::Data, const shared_ptr<ReflectionClassContainer>> InstanceClassMap;
    InstanceClassMap serverInstanceClassMap;

    // Maintain a string dictionary for each Property/event type I don't recognize
    typedef std::map<unsigned int, shared_ptr<SharedStringDictionary>> DummyPropertyStrings;
    DummyPropertyStrings dummyStrings;

    typedef std::map<unsigned int, shared_ptr<SharedStringProtectedDictionary>> DummyPropertyProtectedStrings;
    DummyPropertyProtectedStrings dummyProtectedStrings;

    typedef std::map<unsigned int, shared_ptr<SharedStringDictionary>> DummyEventStrings;
    DummyEventStrings dummyEventStrings;

    typedef std::map<unsigned int, shared_ptr<SharedBinaryStringDictionary>> DummyPropertyBinaryStrings;
    DummyPropertyBinaryStrings dummyBinaryStrings;

    void learnSchema(RakNet::BitStream& bitStream);

    void skipPropertyValue(RakNet::BitStream& inBitStream, const shared_ptr<ReflectionPropertyContainer>&, bool useDictionary);
    void skipPropertiesInternal(const shared_ptr<ReflectionPropertyContainer>&, RakNet::BitStream& inBitstream, bool useDictionary);
    void skipChangedProperty(RakNet::BitStream& bitStream, const shared_ptr<ReflectionPropertyContainer>& prop);
    void skipEventInvocation(RakNet::BitStream& bitStream, const shared_ptr<ReflectionEventContainer>& event);

    bool getServerBasedProperty(const Name& className, int propertyId, shared_ptr<ReflectionPropertyContainer>& serverBasedProperty);
    bool getServerBasedEvent(const Name& className, int eventId, shared_ptr<ReflectionEventContainer>& serverBasedEvent);

    SharedStringProtectedDictionary& getSharedPropertyProtectedDictionaryById(unsigned int propertyId);
    SharedStringDictionary& getSharedPropertyDictionaryById(unsigned int propertyId);
    SharedStringDictionary& getSharedEventDictionaryById(unsigned int eventId);
    SharedBinaryStringDictionary& getSharedPropertyBinaryDictionaryById(unsigned int propertyId);

    bool hasEnumChanged(const Reflection::EnumDescriptor& enumDesc, size_t& newMSB);
};
} // namespace Network
} // namespace Aya
