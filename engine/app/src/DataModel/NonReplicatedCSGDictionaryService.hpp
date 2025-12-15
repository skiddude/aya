
#pragma once

#include "Utility/BinaryString.hpp"
#include "Tree/Instance.hpp"
#include "Tree/Service.hpp"
#include "DataModel/FlyweightService.hpp"

#include <boost/unordered_map.hpp>
#include "Value.hpp"

namespace Aya
{
class PartOperation;

extern const char* const sNonReplicatedCSGDictionaryService;

class NonReplicatedCSGDictionaryService
    : public DescribedCreatable<NonReplicatedCSGDictionaryService, FlyweightService, sNonReplicatedCSGDictionaryService,
          Reflection::ClassDescriptor::PERSISTENT, Security::Roblox>
{
protected:
    typedef DescribedCreatable<NonReplicatedCSGDictionaryService, FlyweightService, sNonReplicatedCSGDictionaryService,
        Reflection::ClassDescriptor::PERSISTENT, Security::Roblox>
        Super;

    virtual void refreshRefCountUnderInstance(Aya::Instance* instance);

    void reparentChildData(shared_ptr<Aya::Instance> childInstance);

public:
    NonReplicatedCSGDictionaryService();

    void storeData(PartOperation& partOperation, bool forceIncrement = false);
    void retrieveData(PartOperation& partOperation);

    void storeAllDescendants(shared_ptr<Aya::Instance> instance);
    void retrieveAllDescendants(shared_ptr<Aya::Instance> instance);
};
} // namespace Aya
