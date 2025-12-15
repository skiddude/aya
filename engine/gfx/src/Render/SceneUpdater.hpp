#pragma once

#include "DataModel/DataModel.hpp"
#include "Utility/Profiling.hpp"

#include "Base/GfxPart.hpp"
#include "Base/RenderStats.hpp"
#include "Base/RenderCaps.hpp"
#include "Base/RenderSettings.hpp"

#include "Voxel/CellChangeListener.hpp"
#include "Voxel2/GridListener.hpp"

#include "Frustum.hpp"

#include <boost/pool/pool_alloc.hpp>

namespace Aya
{
class Humanoid;
class MegaClusterInstance;

namespace Voxel
{
struct DataModelPartCache;
}
} // namespace Aya

namespace Aya
{
namespace Graphics
{
using Aya::Voxel::DataModelPartCache;
class VisualEngine;
class FastCluster;
class SuperCluster;
template<class Cluster>
class SpatialGrid;
struct SpatialGridIndex;
struct LightGridChunk;

class SceneUpdater
    : public Voxel::CellChangeListener
    , public Voxel2::GridListener
{
private:
    Aya::mutex queue_mutex;
    CRenderSettings* mSettings;

    // debugging.
    std::vector<Aya::signals::connection> connections;

protected:
    boost::shared_ptr<Aya::DataModel> dataModel;
    RenderStats* mRenderStats; // for debugging.
    const RenderCaps* mRenderCaps;

    unsigned long currentFrameNum;
    Aya::Frustum updateFrustum;

public:
    void unbind();

    // some of these should be made private?
    // call this in response to sleep events.
    void notifyAwake(Aya::GfxPart* part);
    void notifySleeping(Aya::GfxPart* part);

    void notifyDestroyed(Aya::GfxPart* part);

    // call this when a part or a cluster's properties changed, and must be refreshed.
    void queueInvalidatePart(Aya::GfxPart* part);

    void queueInvalidateFastCluster(Aya::GfxPart* cluster);
    void queuePriorityInvalidateFastCluster(Aya::GfxPart* cluster);
    void queueFullInvalidateMegaCluster(Aya::GfxPart* part);
    void queueChunkInvalidateMegaCluster(Aya::GfxPart* part, const SpatialRegion::Id& pos, bool isWaterChunk);

    void invalidateAllFastClusters();
    void eraseAllHumanoidClusters();

    void updatePrepare(unsigned long currentFrameNum, const Aya::Frustum& updateFrustum);
    void updatePerform();

    void notifyWaitingForAssets(Aya::GfxPart* part, const std::vector<Aya::ContentId>& ids);

    void queuePartToCreate(const boost::shared_ptr<Aya::PartInstance>& part);
    void queueAttachementToCreate(const boost::shared_ptr<Aya::Instance>& instance);

    void queueFastClusterCheck(Aya::GfxPart* cluster, bool isFW);

    bool arePartsWaitingForAssets();

    size_t getUpdateQueueSize() const;

    void setComputeLightingEnabled(bool value);

    // struct and typedef visible for testing
    struct MegaClusterChunk
    {
        MegaClusterChunk()
            : chunkPos(Vector3int16::zero()){};
        MegaClusterChunk(Aya::GfxPart* part, const SpatialRegion::Id& pos, bool isWaterChunk)
            : cluster(part)
            , chunkPos(pos)
            , isWaterChunk(isWaterChunk){};
        Aya::GfxPart* cluster;
        SpatialRegion::Id chunkPos;
        bool isWaterChunk;
    };
    typedef std::vector<MegaClusterChunk> MegaClusterChunkList;

    static bool isPartStatic(Aya::PartInstance* part);

    const boost::shared_ptr<Aya::DataModel>& getDataModel() const
    {
        return dataModel;
    }

protected:
    Aya::signals::scoped_connection workspaceDescendantAddedConnection;
    Aya::signals::scoped_connection propertyChangedSignal;

    void addMegaCluster(const shared_ptr<Aya::PartInstance>& part);
    void addFastPart(const shared_ptr<Aya::PartInstance>& part, bool isFW, bool isPriorityPart);
    void addAttachment(const shared_ptr<Aya::Instance>& instance);

    typedef boost::unordered_set<Aya::GfxPart*, boost::hash<Aya::GfxPart*>, std::equal_to<Aya::GfxPart*>, boost::fast_pool_allocator<Aya::GfxPart*>>
        GfxPartSet;
    GfxPartSet mFastClustersToCheck;
    GfxPartSet mFastClustersToCheckFW;

private:
    void onWorkspaceDescendantAdded(boost::shared_ptr<Aya::Instance> descendant);

    void updateDynamicParts();
    void processPendingMegaClusters();
    void processPendingParts(bool priorityParts);
    void processPendingAttachments();
    void updateAllInvalidParts(bool bulkExecution);
    void updateWaitingParts(bool bulkExecution);
    void updateInvalidatedFastClusters(bool bulkExecution);
    void checkFastClusters();
    void computeLightingPrepare();
    void computeLightingPerform();

    void onPropertyChanged(const Aya::Reflection::PropertyDescriptor* descriptor);

    void updateMegaClusters(bool bulkExecution);

    typedef boost::unordered_map<Aya::PartInstance*, boost::weak_ptr<Aya::PartInstance>> PartInstanceSet;

    GfxPartSet mDynamicNodes;
    GfxPartSet mInvalidatedParts;

    GfxPartSet mInvalidatedFastClusters;
    GfxPartSet mPriorityInvalidateFastClusters;

    PartInstanceSet mAddedParts;
    PartInstanceSet mAddedMegaClusters;

    GfxPartSet mMegaClusters;

    GfxPartSet mFullInvalidatedClusters;
    MegaClusterChunkList mCloseChunkInvalidates;
    MegaClusterChunkList mMiddleChunkInvalidates;
    MegaClusterChunkList mFarChunkInvalidates;

    typedef boost::unordered_map<Aya::Instance*, boost::weak_ptr<Aya::Instance>> InstanceSet;
    InstanceSet mAddedAttachementInstances;

    typedef std::multimap<Aya::GfxPart*, Aya::ContentId> AssetPartMap;
    AssetPartMap mWaitingParts; // parts waiting for content.

public:
    typedef SpatialGrid<FastCluster> FastGrid;
    typedef SpatialGrid<SuperCluster> FastGridSC;

    SceneUpdater(shared_ptr<Aya::DataModel> dataModel, VisualEngine* ve);
    ~SceneUpdater();

    void destroyAttachment(GfxPart* object);
    void destroyFastCluster(FastCluster* cluster);
    void destroySuperCluster(SuperCluster* cluster);

    static Aya::Humanoid* getHumanoid(Aya::PartInstance* part);
    bool checkAddSeenFastClusters(const SpatialGridIndex& index);

    void lightingInvalidateOccupancy(const Aya::Extents& extents, const Aya::Vector3& highPriorityPoint, bool isFixed);
    void lightingInvalidateLocal(const Aya::Extents& extents);

    Aya::WindowAverage<double, double>::Stats getLightingTimeStats();
    unsigned getLastOccupancyUpdates()
    {
        return mLastOccupancyUpdates;
    }
    unsigned getLastLightingUpdates()
    {
        return mLastLightingUpdates;
    }
    unsigned getLightOldestAge();
    bool isLightingActive()
    {
        return mLightingActive;
    }

    void setPointOfInterest(const Vector3& poi)
    {
        pointOfInterest = poi;
    }

    /*override*/ virtual void terrainCellChanged(const Voxel::CellChangeInfo& info);
    /*override*/ virtual void onTerrainRegionChanged(const Voxel2::Region& region);

private:
    VisualEngine* mVisualEngine;

private:
    typedef std::map<void*, FastCluster*> HumanoidClusterMap;
    HumanoidClusterMap mHumanoidClusters;
    scoped_ptr<FastGridSC> mFastGridSC;

    GfxPartSet mAttachments;

    std::vector<SpatialGridIndex> mSeenFastClusters;

    bool seenIndexBefore(const SpatialGridIndex& index);
    void checkAndActivateLighting();

    Aya::WindowAverage<double, double> mLightingComputeAverage;
    unsigned mLastOccupancyUpdates;
    unsigned mLastLightingUpdates;

    unsigned mAgeDirtyProportion;
    bool mLightingActive;

    bool computeLightingEnabled;

    std::vector<DataModelPartCache> mOccupancyPartCache;
    std::vector<std::pair<LightGridChunk*, unsigned>> mLgridchunksToUpdate; // .first = chunk, .second = cached chunk's dirty flags
    Vector3 mFocusPoint;
    bool mLightgridMoved;
    unsigned getChunkBudget();

    Vector3 pointOfInterest;
};
} // namespace Graphics
} // namespace Aya
