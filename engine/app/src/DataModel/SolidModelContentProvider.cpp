


#include "DataModel/SolidModelContentProvider.hpp"
#include "DataModel/PartOperationAsset.hpp"
#include "DataModel/CSGMesh.hpp"
#include "DataModel/CSGDictionaryService.hpp"
#include "DataModel/NonReplicatedCSGDictionaryService.hpp"
#include "DataModel/FlyweightService.hpp"
#include "DataModel/ContentProvider.hpp"
#include "Xml/SerializerBinary.hpp"

namespace Aya
{

const char* const sSolidModelContentProvider = "SolidModelContentProvider";
SolidModelContentProvider::SolidModelContentProvider()
    : DescribedNonCreatable<SolidModelContentProvider, CacheableContentProvider, sSolidModelContentProvider,
          Aya::Reflection::ClassDescriptor::RUNTIME_LOCAL>(CACHE_ENFORCE_MEMORY_SIZE, 1024 * 1024 * 32)
{
    setName(sSolidModelContentProvider);
}

TaskScheduler::StepResult SolidModelContentProvider::ProcessTask(const std::string& id, shared_ptr<const std::string> data)
{
    if (data)
    {
        boost::shared_ptr<CacheableContentProvider::CachedItem> solidModel(new CacheableContentProvider::CachedItem());

        Aya::Instances instances;
        std::stringstream ss(*data);
        SerializerBinary::deserialize(ss, instances);

        if (instances.size() > 0)
        {
            if (shared_ptr<Aya::PartOperationAsset> partOperationAsset =
                    Aya::Instance::fastSharedDynamicCast<Aya::PartOperationAsset>(instances.front()))
            {
                const BinaryString& meshString = partOperationAsset->getMeshData();
                shared_ptr<CSGMesh> csgmesh(CSGMeshFactory::singleton()->createMesh());
                csgmesh->fromBinaryString(meshString.value());
                partOperationAsset->setRenderMesh(csgmesh);
                solidModel->data = partOperationAsset;
            }
        }
        else
        {
            Aya::StandardOut::singleton()->printf(
                Aya::MESSAGE_ERROR, "SolidModelContentProvider failed to process %s because 'could not fetch'", id.c_str());
            markContentFailed(id);
            return TaskScheduler::Stepped;
        }

        updateContent(id, solidModel);
        solidModel->requestResult = AsyncHttpQueue::Succeeded;

        --pendingRequests;
        return TaskScheduler::Stepped;
    }
    else
    {
        Aya::StandardOut::singleton()->printf(
            Aya::MESSAGE_ERROR, "SolidModelContentProvider failed to process %s because 'could not fetch'", id.c_str());
    }
    markContentFailed(id);
    return TaskScheduler::Stepped;
}

void SolidModelContentProvider::updateContent(const std::string& id, boost::shared_ptr<CacheableContentProvider::CachedItem> solidModel)
{
    if (solidModel->data)
    {
        boost::shared_ptr<Aya::PartOperationAsset> partOperationAsset = boost::static_pointer_cast<Aya::PartOperationAsset>(solidModel->data);
        boost::shared_ptr<CSGMesh> meshData = partOperationAsset->getRenderMesh();
        lruCache->insert(id, solidModel, meshData->getIndices().size() * sizeof(unsigned int) + meshData->getVertices().size() * sizeof(CSGVertex));
    }
    else
        lruCache->insert(id, solidModel, 0);
}

} // namespace Aya
