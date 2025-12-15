
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
class CSGMesh;

extern const char* const sCSGDictionaryService;

class CSGDictionaryService
    : public DescribedCreatable<CSGDictionaryService, FlyweightService, sCSGDictionaryService, Reflection::ClassDescriptor::PERSISTENT,
          Security::Roblox>
{
protected:
    typedef DescribedCreatable<CSGDictionaryService, FlyweightService, sCSGDictionaryService, Reflection::ClassDescriptor::PERSISTENT,
        Security::Roblox>
        Super;
    typedef boost::unordered_map<std::string, boost::shared_ptr<CSGMesh>> CSGMeshMap;
    CSGMeshMap cachedBREPMeshMap;
    CSGMeshMap cachedMeshMap;

    void reparentChildData(shared_ptr<Aya::Instance> sharedInstance);

    virtual void refreshRefCountUnderInstance(Aya::Instance* instance);

    boost::shared_ptr<CSGMesh> insertMesh(const std::string key, const Aya::BinaryString& meshData);

    boost::shared_ptr<CSGMesh> insertCachedMesh(const std::string key, const Aya::BinaryString& meshData);

    virtual void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

public:
    CSGDictionaryService();

    void storeData(PartOperation& partOperation, bool forceIncrement = false);
    void retrieveData(PartOperation& partOperation);

    void storeAllDescendants(shared_ptr<Aya::Instance> instance);
    void retrieveAllDescendants(shared_ptr<Aya::Instance> instance);

    void reparentAllChildData();

    void insertMesh(PartOperation& partOperation);
    boost::shared_ptr<CSGMesh> getMesh(PartOperation& partOperation);

    boost::shared_ptr<CSGMesh> getCachedMesh(PartOperation& partOperation);

    void retrieveMeshData(PartOperation& partOperation);

    void storePhysicsData(PartOperation& partOperation, bool forceIncrement = false);
    void retrievePhysicsData(PartOperation& partOperation);

    void onWorkspaceLoaded();
};
} // namespace Aya
