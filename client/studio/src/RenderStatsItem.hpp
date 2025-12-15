/*
 *  RenderStatsItem.h
 *  RobloxStudio
 *  Copied from old Roblox app directory
 *
 *  Created by Vinod on 10/18/11.
 *  Copyright 2011 Roblox. All rights reserved.
 *
 */
#pragma once

#include "DataModel/Stats.hpp"

#include "Base/RenderStats.hpp"
#include "Base/FrameRateManager.hpp"
#include "boost/shared_ptr.hpp"

namespace Aya
{
namespace Stats
{
class Item;
class StatsService;
} // namespace Stats
namespace Profiling
{
class BucketProfile;
}
} // namespace Aya

class RenderStatsItem : public Aya::Stats::Item
{
public:
    RenderStatsItem(const Aya::RenderStats& renderStats);
    void update();

    static boost::shared_ptr<RenderStatsItem> create(const Aya::RenderStats& renderStats);
    static void addBucketToItem(Aya::Stats::Item* item, const Aya::Profiling::BucketProfile& bucketProfile);

private:
    Aya::Stats::Item* m_videoMemory;
    Aya::Stats::Item* m_majorStateChanges;
    Aya::Stats::Item* m_minorStateChanges;
    const Aya::RenderStats& m_renderStats;
};
