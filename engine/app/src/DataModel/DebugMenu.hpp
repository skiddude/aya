#pragma once

#include "DataModel.hpp"
#include <deque>

namespace Aya
{

class CustomStatsGuiJSON;

class MetricPlot
{
public:
    MetricPlot(size_t maxSize = 300);
    void addValue(float value);
    const float* data();
    size_t size() const;

private:
    std::vector<float> values;
    size_t maxSize;
};

class DebugMenu
{
public:
    friend class CustomStatsGuiJSON;

    enum NetworkStats
    {
        NETWORKSTATS_INVALID = 0,
        NETWORKSTATS_FIRST,
        NETWORKSTATS_RAKNET = NETWORKSTATS_FIRST,
        NETWORKSTATS_PHYSICS,
        NETWORKSTATS_DATATYPE,
        NETWORKSTATS_STREAMING,
        NETWORKSTATS_COUNT
    };

    DebugMenu(DataModel* dm);

    void addMetricValue(MetricPlot& plot, const std::string& value);
    void metric(const char* label, const std::string& metric);
    void render();
    void addCustomStat(const std::string& label, const std::string& metric);
    void removeCustomStat(const std::string& label);
    void saveCustomStats();

    void nextNetworkStats();

    std::vector<std::pair<std::string, std::string>>& getCustomMetrics()
    {
        return customMetrics;
    }

    void toggleGeneralStats();
    void toggleRenderStats();
    void toggleNetworkStats();
    void togglePhysicsStats();
    void toggleSummaryStats();
    void toggleCustomStats();

    bool isShowingGeneralStats() const
    {
        return showGeneralStats;
    }
    bool isShowingRenderStats() const
    {
        return showRenderStats;
    }
    bool isShowingNetworkStats() const
    {
        return showNetworkStats;
    }
    bool isShowingPhysicsStats() const
    {
        return showPhysicsStats;
    }
    bool isShowingSummaryStats() const
    {
        return showSummaryStats;
    }
    bool isShowingCustomStats() const
    {
        return showCustomStats;
    }

private:
    bool showGeneralStats; // StatsHud1, StatsHud2
    bool showRenderStats;
    bool showNetworkStats;
    bool showPhysicsStats; // PhysicsStats1, PhysicsStats2
    bool showSummaryStats;
    bool showCustomStats;

    void buildGeneralStats();
    void buildRenderStats();
    void buildNetworkStats();
    void buildPhysicsStats();
    void buildSummaryStats();
    void buildCustomStats();

    std::vector<std::pair<std::string, std::string>> customMetrics;

    // Network plot buffers
    MetricPlot dataSendPlot;
    MetricPlot dataReceivePlot;
    MetricPlot physicsSendPlot;
    MetricPlot physicsReceivePlot;

    NetworkStats networkStatsCounter;
    DataModel* dataModel;
};

} // namespace Aya