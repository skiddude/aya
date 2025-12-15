
#include "DataModel/DataModel.hpp"
#include "DataModel/BaseRenderJob.hpp"
#include "DataModel/HackDefines.hpp"
#include "DataModel/ModelInstance.hpp"
#include "DataModel/Workspace.hpp"

#include "Profiler.hpp"
#include "ImGui.hpp"

LOGGROUP(TaskSchedulerTiming)

namespace Aya
{

BaseRenderJob::BaseRenderJob(double minFps, double maxFps, boost::shared_ptr<DataModel> dataModel)
    : Aya::DataModelJob("Render", Aya::DataModelJob::Render, false, dataModel, Aya::Time::Interval(.02))
    , isAwake(true)
    , minFrameRate(minFps)
    , maxFrameRate(maxFps)
{
    cyclicExecutive = true;
    // We originally intended RenderJob to run after Network, Physics, and so on
    // to reduce latency, however this introduced a weird variability into the dt between
    // each RenderJob::step. Since makes sure that the first job is always 16 - 17ms apart from
    // it's previous run, making RenderJob run first will grant more visual stability at the cost
    // of potentially some latency in input.
    cyclicPriority = CyclicExecutiveJobPriority_EarlyRendering;
}


void BaseRenderJob::wake()
{
    isAwake = true;
    if (!isCyclicExecutiveJob())
        TaskScheduler::singleton().reschedule(shared_from_this());
}

bool BaseRenderJob::tryJobAgain()
{
    if (isCyclicExecutiveJob() && !isAwake)
    {
        return true;
    }
    return false;
}

bool BaseRenderJob::isCyclicExecutiveJob()
{
    return Aya::TaskScheduler::singleton().isCyclicExecutive() && cyclicExecutive;
}

TaskScheduler::Job::Error BaseRenderJob::error(const Stats& stats)
{
    if (!isAwake && (!isCyclicExecutiveJob()))
    {
        return Job::Error();
    }

    Job::Error result;
    if (isCyclicExecutiveJob())
    {
        result = computeStandardError(stats, maxFrameRate);
        FASTLOG1F(FLog::TaskSchedulerTiming, "Error recalculated, time since last call: %f", (float)stats.timespanSinceLastStep.seconds());
    }
    else
    {
        result = computeStandardError(stats, minFrameRate);
    }
    return result;
}

Time::Interval BaseRenderJob::timeSinceLastRender() const
{
    return Time::now<Aya::Time::Fast>() - lastRenderTime;
}

TaskScheduler::StepResult BaseRenderJob::step(const Stats& stats)
{
    Profiler::onFrame();
    ImGui::onFrame();

    return DataModelJob::step(stats);
}

} // namespace Aya
