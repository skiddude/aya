#pragma once

#include "Tree/Service.hpp"
#include <queue>

namespace Aya
{

extern const char* const sCollectionService;
class CollectionService
    : public DescribedNonCreatable<CollectionService, Instance, sCollectionService>
    , public Service
{
public:
    CollectionService();

    Aya::signal<void(shared_ptr<Instance>)> itemAddedSignal;
    Aya::signal<void(shared_ptr<Instance>)> itemRemovedSignal;

    shared_ptr<const Instances> getCollection(std::string type);
    shared_ptr<const Instances> getCollection(const Name& className);
    template<class T>
    shared_ptr<const Instances> getCollection()
    {
        return getCollection(T::classDescriptor());
    }

    void removeInstance(shared_ptr<Instance> instance);
    void addInstance(shared_ptr<Instance> instance);

private:
    // TODO: Lookup by const Aya::Name*
    typedef std::map<std::string, shared_ptr<copy_on_write_ptr<Instances>>> CollectionMap;
    CollectionMap collections;
};

} // namespace Aya