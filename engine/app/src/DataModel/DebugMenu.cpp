#include "DebugMenu.hpp"
#include "imgui.h"
#include "implot.h"
#include <vector>
#include <boost/algorithm/string.hpp>
#include "Utility/FileSystem.hpp"
#include "SimpleJSON.hpp"
#include <ctime>
#include "ImGui.hpp"
namespace Aya
{

static boost::filesystem::path GetCustomStatsFilename()
{
    return Aya::FileSystem::getUserDirectory(false, Aya::DirExe, "ClientSettings") / "StatDisplaySettings.json";
}

class CustomStatsGuiJSON : public SimpleJSON
{
public:
    CustomStatsGuiJSON(DebugMenu* debugMenu);

    virtual bool DefaultHandler(const std::string& valueName, const std::string& valueData);

    void WriteFile();

private:
    CustomStatsGuiJSON() {};

    DebugMenu* mDebugMenu;
};

CustomStatsGuiJSON::CustomStatsGuiJSON(DebugMenu* debugMenu)
    : mDebugMenu(debugMenu)
{
    // this space intentionally left blank
}

bool CustomStatsGuiJSON::DefaultHandler(const std::string& valueName, const std::string& valueData)
{
    mDebugMenu->addCustomStat(valueName, valueData);
    return true;
}

void CustomStatsGuiJSON::WriteFile()
{
    std::ofstream localConfigFile(GetCustomStatsFilename().native().c_str());

    if (localConfigFile.is_open())
    {
        std::stringstream localConfigBuffer;
        localConfigFile.write("{\n", 2);

        auto& customMetrics = mDebugMenu->getCustomMetrics();

        for (const auto& pair : customMetrics)
        {
            std::string line = Aya::format("    \"%s\" : \"%s\"", pair.first.c_str(), pair.second.c_str());
            localConfigFile.write(line.c_str(), line.length());
            localConfigFile.write(",\n", 2);
        }
        localConfigFile.write("}", 1);
    }
}

MetricPlot::MetricPlot(size_t maxSize)
    : maxSize(maxSize)
{
    //
}

void MetricPlot::addValue(float value)
{
    if (values.size() >= maxSize)
        values.erase(values.begin());

    values.push_back(value);
}

const float* MetricPlot::data()
{
    return values.data();
}

size_t MetricPlot::size() const
{
    return values.size();
}

DebugMenu::DebugMenu(DataModel* dm)
    : dataModel(dm)
    , showGeneralStats(false)
    , showRenderStats(true)
    , showNetworkStats(false)
    , showPhysicsStats(false)
    , showSummaryStats(false)
    , showCustomStats(false)
    , networkStatsCounter(NETWORKSTATS_INVALID)
{
    CustomStatsGuiJSON json(this);
    std::string localSettingsData;
    std::ifstream localConfigFile(GetCustomStatsFilename().native().c_str());

    if (localConfigFile.is_open())
    {
        std::stringstream localConfigBuffer;
        localConfigBuffer << localConfigFile.rdbuf();
        localSettingsData = localConfigBuffer.str();
        if (localSettingsData.length() > 0)
        {
            json.ReadFromStream(localSettingsData.c_str());
        }
    }
}

void DebugMenu::addCustomStat(const std::string& label, const std::string& metric)
{
    customMetrics.push_back(std::make_pair(label, metric));
}

void DebugMenu::removeCustomStat(const std::string& label)
{
    auto it = std::remove_if(customMetrics.begin(), customMetrics.end(),
        [&label](const std::pair<std::string, std::string>& pair)
        {
            return pair.first == label;
        });
    if (it != customMetrics.end())
    {
        customMetrics.erase(it, customMetrics.end());
    }
}

void DebugMenu::saveCustomStats()
{
    CustomStatsGuiJSON json(this);
    json.WriteFile();
}

void DebugMenu::nextNetworkStats()
{
    int counter = (int)networkStatsCounter;
    counter++;

    networkStatsCounter = (NetworkStats)counter;

    if (networkStatsCounter >= NETWORKSTATS_COUNT)
    {
        networkStatsCounter = NETWORKSTATS_FIRST;
    }
}

void DebugMenu::toggleGeneralStats()
{
    showGeneralStats = !showGeneralStats;
}

void DebugMenu::toggleRenderStats()
{
    showRenderStats = !showRenderStats;
}

void DebugMenu::toggleNetworkStats()
{
    showNetworkStats = !showNetworkStats;
}

void DebugMenu::togglePhysicsStats()
{
    showPhysicsStats = !showPhysicsStats;
}

void DebugMenu::toggleSummaryStats()
{
    showSummaryStats = !showSummaryStats;
}

void DebugMenu::toggleCustomStats()
{
    showCustomStats = !showCustomStats;
}

void DebugMenu::addMetricValue(MetricPlot& plot, const std::string& value)
{
    std::vector<std::string> results;
    boost::split(results, value, boost::is_any_of(" "));

    std::string split = results[0];
    split.pop_back(); // remove trailing comma...
    float f = std::stof(split);
    plot.addValue(f);
}

void DebugMenu::metric(const char* label, const std::string& metric)
{
    const std::string value = dataModel->getMetric(metric);

    if (label == "InData")
        addMetricValue(dataSendPlot, value);
    else if (label == "InPhysics")
        addMetricValue(dataReceivePlot, value);
    else if (label == "PhysicsSend")
        addMetricValue(physicsSendPlot, value);
    else if (label == "PhysicsReceive")
        addMetricValue(physicsReceivePlot, value);

    ::ImGui::Text("%s: %s", label, value.c_str());
}

void DebugMenu::buildGeneralStats()
{
    if (!showGeneralStats)
        return;

    ::ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
    ::ImGui::Begin("General", &showGeneralStats);
    ::ImGui::PopStyleColor();

    ::ImGui::SeparatorText("World");
    metric("Instances", "numInstances");
    metric("Physics Mode", "physicsMode");
    metric("Primitives", "numPrimitives");
    metric("Moving Prims", "numMovingPrimitives");
    metric("Joints", "numJoints");
    metric("Contacts", "numContacts");
    metric("Tree", "maxTreeDepth");

    ::ImGui::SeparatorText("Timing");
    metric("Physics", "Physics");
    metric("Mouse Move", "MouseMove");
    metric("Render Nom", "Render Nominal FPS");
    metric("Delta btw render", "Delta Between Renders");
    metric("Scheduler render", "Render");
    metric("Total Render", "Total Render");

    ::ImGui::SeparatorText("Graphics");
    metric("FPS", "Effective FPS");
    metric("Available VRAM", "Video Memory");
    metric("Anti-Aliasing", "Anti-Aliasing");
    metric("VirtualVersion", "VirtualVersion");
    metric("Frame Id", "drawId");
    metric("World Id", "worldId");

    ::ImGui::SeparatorText("Throttles");
    metric("EnviroSpeed %", "environmentSpeed");
    metric("FRM", "FRM");
    metric("FRM Target", "FRM Target");
    metric("FRM Visible", "FRM Visible");
    metric("FRM Quality", "FRM Quality");

    ::ImGui::SeparatorText("Network");
    metric("RequestQueue", "RequestQueueSize");

    ::ImGui::SeparatorText("Contacts");
    metric("CtctStageCtcts", "numContactsInCollisionStage");
    metric("SteppingCtcts", "numSteppingContacts");
    metric("TouchingCtcts", "numTouchingContacts");
    metric("% Pair Hit", "contactPairHitRatio");
    metric("% Feature Hit", "contactFeatureHitRatio");
    metric("# link(p)", "numLinkCalls");
    metric("sH Nodes Out", "numHashNodes");
    metric("sH Max Bucket", "maxBucketSize");

    ::ImGui::SeparatorText("Kernel");
    metric("Solver Iteratn", "solverIterations"); // This is for fun / deception - we don't use matrices!!
    metric("Matrix Size", "matrixSize");
    metric("PGS Solver On", "pGSSolverActive");
    metric("Bodies", "numBodies");
    metric("MaxBodies", "maxBodies");
    metric("Leaves", "numLeafBodies");
    metric("Constraints", "numConstraints");
    metric("Points", "numPoints");
    metric("% Active CC's", "percentConnectorsActive");
    metric("Energy B", "energyBody");
    metric("Energy C", "energyConnector");
    metric("Energy T", "energyTotal");

    ::ImGui::End();
}

void DebugMenu::buildRenderStats()
{
    if (!showRenderStats)
        return;

    ::ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
    ::ImGui::Begin("Render", &showRenderStats);
    ::ImGui::PopStyleColor();

    ::ImGui::SeparatorText("Graphics");
    metric("FPS", "Effective FPS");
    metric("Anti-Aliasing", "Anti-Aliasing");
    metric("Available VRAM", "Video Memory");
    metric("VirtualVersion", "VirtualVersion");
    metric("Physics", "Physics");
    metric("Heartbeat", "Heartbeat");
    metric("Network send", "Network Send");
    metric("Network receive", "Network Receive");
    metric("Resolution", "RenderStatsResolution");

    ::ImGui::SeparatorText("Performance");
    metric("FRM Config", "RenderStatsFRMConfig");
    metric("FRM Blocks", "RenderStatsFRMBlocks");
    metric("FRM Adjust", "RenderStatsFRMAdjust");
    metric("FRM Target Time", "RenderStatsFRMTargetTime");
    metric("FRM Distance", "RenderStatsFRMDistance");
    metric("Framerate Variance", "FRM Variance");

    ::ImGui::SeparatorText("Timing");
    metric("Scheduler Render", "Render");
    metric("CPU", "RenderStatsTimeTotal");
    metric("GPU", "RenderStatsGPU");
    metric("Delta", "RenderStatsDelta");
    metric("Draw (total)", "RenderStatsPassTotal");
    metric("Draw (scene)", "RenderStatsPassScene");
    metric("Draw (shadow)", "RenderStatsPassShadow");
    metric("Draw (UI)", "RenderStatsPassUI");
    metric("Draw (3D Adorns)", "RenderStatsPass3dAdorns");
    metric("Light grid", "RenderStatsLightGrid");
    metric("Geometry Gen", "RenderStatsGeometryGen");
    metric("Clusters", "RenderStatsClusters");
    metric("Textures", "RenderStatsTM");
    metric("TC", "RenderStatsTC");

    ::ImGui::End();
}

void DebugMenu::buildNetworkStats()
{
    if (!showNetworkStats)
        return;

    ::ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
    ::ImGui::Begin("Network", &showNetworkStats);
    ::ImGui::PopStyleColor();

    ::ImGui::SeparatorText("HTTP");
    metric("Avg Time In Queue", "HttpTimeInQueue");
    metric("Avg Process Time", "HttpProcessTime");
    metric("Request Queue Size", "RequestQueueSize");
    metric("# slow requests", "HttpSlowReq");

    ::ImGui::SeparatorText("Replicator");
    metric("General (MTU Size, Ping)", "GeneralStats");

    ::ImGui::SeparatorText("Incoming");
    metric("Overall (KB/s, Pkt/s, Queue)", "IncomingStats");
    metric("In Data (KB/s, Pkt/s, Avg Size)", "InData");
    metric("In Physics (KB/s, Pkt/s, Avg Size, Avg Lag)", "InPhysics");
    metric("In Touches (KB/s, Pkt/s, Avg Size)", "InTouches");
    metric("In Clusters (KB/s, Pkt/s, Avg Size)", "InClusters");

    ::ImGui::SeparatorText("Outgoing");
    metric("Overall (KB/s)", "OutgoingStats");

    metric("Out Data (KB/s, Pkt/s, Avg Size, Throttle)", "OutData");
    metric("Out Physics (KB/s, Pkt/s, Avg Size, Throttle)", "OutPhysics");
    metric("Out Touches (KB/s, Pkt/s, Avg Size, #Waiting)", "OutTouches");
    metric("Out Clusters (KB/s, Pkt/s, Avg Size)", "OutClusters");

    switch (networkStatsCounter)
    {
    case NETWORKSTATS_RAKNET:
        ::ImGui::SeparatorText("RakNet");
        metric("ping", "RakNetPing");
        metric("msgTotalBytesPushed/s", "messageDataBytesSentPerSec");
        metric("msgTotalBytesSent/s", "messageTotalBytesSentPerSec");
        metric("msgDataBytesResent/s", "messageDataBytesResentPerSec");
        metric("msgBytesRecv/s", "messagesBytesReceivedPerSec");
        metric("msgBytesRecvAndIgnored/s", "messagesBytesReceivedAndIgnoredPerSec");
        metric("bytesSent/s", "bytesSentPerSec");
        metric("bytesReceived/s", "bytesReceivedPerSec");
        metric("outBandwidthLimitBytes/s", "outgoingBandwidthLimitBytesPerSecond");
        metric("limitedByOutBandwidthLimit", "isLimitedByOutgoingBandwidthLimit");
        metric("congestionCtrlLimitBytes/s", "congestionControlLimitBytesPerSecond");
        metric("limitedByCongestionControl", "isLimitedByCongestionControl");
        metric("messagesInResendQueue", "messagesInResendQueue");
        metric("bytesInResendQueue", "bytesInResendQueue");
        metric("packetlossLastSecond", "packetlossLastSecond");

        break;
    case NETWORKSTATS_PHYSICS:
        ::ImGui::SeparatorText("In Physics");
        ::ImGui::Text("Class) Type (KB/s, Avg Size, Count/s)");
        ::ImGui::Text(dataModel->getMetric("InPhysicsDetails").c_str());

        ::ImGui::SeparatorText("Out Physics");
        ::ImGui::Text("Class) Type (KB/s, Avg Size, Count/s)");
        ::ImGui::Text(dataModel->getMetric("OutPhysicsDetails").c_str());

        break;
    case NETWORKSTATS_DATATYPE:
        ::ImGui::SeparatorText("In Data");
        ::ImGui::Text("Class) Type (KB/s, Avg Size, Count/s)");
        ::ImGui::Text(dataModel->getMetric("InDataDetails").c_str());

        ::ImGui::SeparatorText("Out Data");
        ::ImGui::Text("Class) Type (KB/s, Avg Size, Count/s)");
        ::ImGui::Text(dataModel->getMetric("OutDataDetails").c_str());

        break;
    case NETWORKSTATS_STREAMING:
        ::ImGui::SeparatorText("Streaming");
        metric("Free Memory", "FreeMemory");
        metric("Memory Level", "MemoryLevel");

        ::ImGui::Text("Class) Type (KB/s, Avg Size, Count/s)");
        ::ImGui::Text(dataModel->getMetric("ReceivedStreamData").c_str());

        break;
    default:
        break;
    }

    ::ImGui::End();
}

void DebugMenu::buildPhysicsStats()
{
    if (!showPhysicsStats)
        return;

    ::ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
    ::ImGui::Begin("Physics", &showPhysicsStats);
    ::ImGui::PopStyleColor();

    ::ImGui::SeparatorText("FPS");
    metric("Physics", "Physics");
    metric("PhysicsReal", "PhysicsReal");
    metric("Render", "Render");
    metric("Heartbeat", "Heartbeat");
    metric("Network send", "Network Send");
    metric("Network recv", "Network Receive");
    metric("Rx data pkts", "Received Data Packets");
    metric("Rx phys pkts", "Received Physics Packets");

    ::ImGui::SeparatorText("World");
    metric("Player Radius", "Player Radius");
    metric("Parts", "numPrimitives");
    metric("Moving parts", "numMovingPrimitives");
    metric("Joints", "numJoints");
    metric("Contacts", "numContactsAll");
    metric("Pair Hit", "contactPairHitRatio");
    metric("Feature Hit", "contactFeatureHitRatio");
    metric("sH Nodes", "numHashNodes");

    ::ImGui::SeparatorText("Kernel");
    metric("PGS Solver On", "pGSSolverActive");
    metric("Bodies", "numBodiesAll");
    metric("Connectors", "numConnectorsAll");
    metric("Energy T", "energyTotal");
    metric("Iterations", "numIterations");

    ::ImGui::SeparatorText("World Step");
    metric("Break Time", "Break Time");
    metric("Assembler Time", "Assembler Time");
    metric("Filter Time", "Filter Time");
    metric("UI Step", "UI Step");
    metric("Broadphase", "Broadphase");
    metric("Collision", "Collision");
    metric("Joint Sleep", "Joint Sleep");
    metric("Wake", "Wake");
    metric("Sleep", "Sleep");
    metric("Joint Update", "Joint Update");
    metric("Bodies", "Kernel Bodies");
    metric("Connectors", "Kernel Connectors");

    ::ImGui::End();
}

void DebugMenu::buildSummaryStats()
{
    if (!showSummaryStats)
        return;

    ::ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
    ::ImGui::Begin("Summary", &showSummaryStats);
    ::ImGui::PopStyleColor();

    metric("FPS", "Effective FPS");
    metric("Physics", "Physics");
    metric("Render", "Render");
    metric("Network receive", "Network Receive");

    // Network plots
    ::ImGui::Spacing();
    ::ImGui::SeparatorText("Network Graphs");

    ::ImGui::SetWindowFontScale(0.9f);
    if (ImPlot::BeginPlot("Data Traffic", ImVec2(-1, 175)))
    {
        ::ImPlot::SetupLegend(ImPlotLocation_SouthEast, ImPlotLegendFlags_None);
        ::ImPlot::SetupAxes("Elapsed (sec)", "KB/s");
        ::ImPlot::SetupAxisLimits(ImAxis_X1, 0, 300, ImGuiCond_Always);
        ::ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 100, ImGuiCond_Once);

        if (dataReceivePlot.size() > 0)
        {
            ::ImPlot::PushStyleColor(ImPlotCol_Line, IM_COL32(0, 250, 150, 255));
            ::ImPlot::PlotLine("In", dataReceivePlot.data(), dataReceivePlot.size());
            ::ImPlot::PopStyleColor();
        }

        if (dataSendPlot.size() > 0)
        {
            ::ImPlot::PushStyleColor(ImPlotCol_Line, IM_COL32(0, 200, 255, 255));
            ::ImPlot::PlotLine("Out", dataSendPlot.data(), dataSendPlot.size());
            ::ImPlot::PopStyleColor();
        }

        ::ImPlot::EndPlot();
    }

    if (ImPlot::BeginPlot("Physics Traffic", ImVec2(-1, 175)))
    {
        ::ImPlot::SetupLegend(ImPlotLocation_SouthEast, ImPlotLegendFlags_None);
        ::ImPlot::SetupAxes("Elapsed (sec)", "KB/s");
        ::ImPlot::SetupAxisLimits(ImAxis_X1, 0, 300, ImGuiCond_Always);
        ::ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 100, ImGuiCond_Once);

        if (physicsReceivePlot.size() > 0)
        {
            ::ImPlot::PushStyleColor(ImPlotCol_Line, IM_COL32(0, 250, 150, 255));
            ::ImPlot::PlotLine("In", physicsReceivePlot.data(), physicsReceivePlot.size());
            ::ImPlot::PopStyleColor();
        }

        if (physicsSendPlot.size() > 0)
        {
            ::ImPlot::PushStyleColor(ImPlotCol_Line, IM_COL32(0, 200, 255, 255));
            ::ImPlot::PlotLine("Out", physicsSendPlot.data(), physicsSendPlot.size());
            ::ImPlot::PopStyleColor();
        }

        ::ImPlot::EndPlot();
    }
    ::ImGui::SetWindowFontScale(1.f);

    ::ImGui::SeparatorText("Network");
    metric("Data Ping", "Data Ping");
    metric("Send KBytesPerSec", "Send kBps");
    metric("Receive KBytesPerSec", "Receive kBps");
    metric("Incoming Packet Queue", "Packet Queue");
    metric("Received Data Packets", "Received Data Packets");
    metric("Received Physics Packets", "Received Physics Packets");
    metric("Received Cluster Packets", "Received Cluster Packets");
    metric("messagesInResendQueue", "messagesInResendQueue");
    metric("Sent Data Packets", "Sent Data Packets");
    metric("Sent Physics Packets", "Sent Physics Packets");

    ::ImGui::SeparatorText("Rendering");
    metric("Available VRAM", "Video Memory");
    metric("FRM Auto Quality", "Auto Quality");
    metric("FRM Quality", "FRM Quality");
    metric("FRM Visible", "FRM Visible");
    metric("Particles", "Particles");
    metric("Scheduler Render", "Render");
    metric("Present time", "Present Time");
    metric("Draw (total)", "RenderStatsPassTotal");
    metric("Draw (scene)", "RenderStatsPassScene");
    metric("Draw (shadow)", "RenderStatsPassShadow");
    metric("Draw (UI)", "RenderStatsPassUI");
    metric("Draw (3D Adorns)", "RenderStatsPass3dAdorns");

    ::ImGui::SeparatorText("Physics");
    metric("Parts", "numPrimitives");
    metric("Moving parts", "numMovingPrimitives");
    metric("Joints", "numJoints");
    metric("Contacts", "numContacts");
    metric("EnviroSpeed %", "environmentSpeed");
    metric("Energy T", "energyTotal");
    metric("TouchingCtcts", "numTouchingContacts");

    ::ImGui::End();
}

void DebugMenu::buildCustomStats()
{
    if (!showCustomStats)
        return;

    ::ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
    ::ImGui::Begin("Custom Stats", &showCustomStats);
    ::ImGui::PopStyleColor();

    for (const auto& pair : customMetrics)
    {
        metric(pair.first.c_str(), pair.second);
    }

    ::ImGui::End();
}

void DebugMenu::render()
{
    if (!ImGui::isInitialized())
        return;

    buildGeneralStats();
    buildRenderStats();
    buildNetworkStats();
    buildPhysicsStats();
    buildSummaryStats();
    buildCustomStats();
}
} // namespace Aya