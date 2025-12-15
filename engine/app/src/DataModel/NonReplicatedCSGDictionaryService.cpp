


#include "DataModel/NonReplicatedCSGDictionaryService.hpp"

#include "DataModel/CSGDictionaryService.hpp"
#include "DataModel/PartOperation.hpp"
#include "DataModel/Value.hpp"
#include "DataModel/DataModel.hpp"

FASTFLAGVARIABLE(IgnoreBlankDataOnStore, true)

FASTFLAG(StudioCSGAssets)

namespace Aya
{
const char* const sNonReplicatedCSGDictionaryService = "NonReplicatedCSGDictionaryService";

NonReplicatedCSGDictionaryService::NonReplicatedCSGDictionaryService()
{
    setName("NonReplicatedCSGDictionaryService");
}

void NonReplicatedCSGDictionaryService::reparentChildData(shared_ptr<Aya::Instance> childInstance)
{
    if (!isChildData(childInstance))
        return;

    CSGDictionaryService* dictionaryService = ServiceProvider::create<CSGDictionaryService>(DataModel::get(this));
    childInstance->setParent(dictionaryService);

    if (shared_ptr<Aya::BinaryStringValue> bStrValue = Aya::Instance::fastSharedDynamicCast<Aya::BinaryStringValue>(childInstance))
    {
        std::string key = createHashKey(bStrValue->getValue().value());
        instanceMap.erase(key);
    }
}

void NonReplicatedCSGDictionaryService::storeData(PartOperation& partOperation, bool forceIncrement)
{
    if (FFlag::StudioCSGAssets)
    {
        BinaryString tmpString = partOperation.getChildData();
        if (FFlag::IgnoreBlankDataOnStore && tmpString.value().size() > 0)
        {
            storeStringData(tmpString, forceIncrement, "ChildData");
            partOperation.setChildData(tmpString);
        }
    }
    else
    {
        BinaryString tmpString = partOperation.getChildData();
        storeStringData(tmpString, forceIncrement, "ChildData");
        partOperation.setChildData(tmpString);
    }
}

void NonReplicatedCSGDictionaryService::retrieveData(PartOperation& partOperation)
{
    if (FFlag::StudioCSGAssets)
    {
        BinaryString tmpString = partOperation.getChildData();
        retrieveStringData(tmpString);
        partOperation.setChildData(tmpString);
    }
    else
    {
        BinaryString tmpString = partOperation.getChildData();
        retrieveStringData(tmpString);
        partOperation.setChildData(tmpString);
    }
}

void NonReplicatedCSGDictionaryService::storeAllDescendants(shared_ptr<Instance> instance)
{
    if (instance->getChildren())
        for (Aya::Instances::const_iterator iter = instance->getChildren()->begin(); iter != instance->getChildren()->end(); ++iter)
            storeAllDescendants(*iter);

    if (shared_ptr<PartOperation> childOperation = Aya::Instance::fastSharedDynamicCast<PartOperation>(instance))
        storeData(*childOperation);
}

void NonReplicatedCSGDictionaryService::retrieveAllDescendants(shared_ptr<Instance> instance)
{
    if (instance->getChildren())
        for (Aya::Instances::const_iterator iter = instance->getChildren()->begin(); iter != instance->getChildren()->end(); ++iter)
            retrieveAllDescendants(*iter);

    if (shared_ptr<PartOperation> childOperation = Aya::Instance::fastSharedDynamicCast<PartOperation>(instance))
        retrieveData(*childOperation);
}

void NonReplicatedCSGDictionaryService::refreshRefCountUnderInstance(Aya::Instance* instance)
{
    if (Aya::PartOperation* partOperation = Aya::Instance::fastDynamicCast<Aya::PartOperation>(instance))
        storeData(*partOperation, true);

    if (instance->getChildren())
        for (Aya::Instances::const_iterator iter = instance->getChildren()->begin(); iter != instance->getChildren()->end(); ++iter)
            refreshRefCountUnderInstance(iter->get());
}
} // namespace Aya
