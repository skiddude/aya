

#include "DataModel/SleepingJob.hpp"

namespace Aya
{

SleepingJob::SleepingJob(const char* name, TaskType taskType, bool isPerPlayer, shared_ptr<Aya::DataModelArbiter> arbiter,
    Aya::Time::Interval stepBudget, double desiredFps)
    : Aya::DataModelJob(name, taskType, isPerPlayer, arbiter, stepBudget)
    , desiredFps(desiredFps)
{
    isAwake = false;
}

void SleepingJob::wake()
{
    if (!isAwake.compare_and_swap(1, 0))
    {
        lastWakeTime = Aya::Time::now<Aya::Time::Fast>();
        Aya::TaskScheduler::singleton().reschedule(shared_from_this());
    }
}

void SleepingJob::sleep()
{
    isAwake = false;
}

Time::Interval SleepingJob::sleepTime(const Stats&)
{
    return isAwake ? Aya::Time::Interval(0) : Aya::Time::Interval::max();
}

TaskScheduler::Job::Error SleepingJob::error(const Stats& stats)
{
    if (!isAwake)
        return Job::Error();

    Stats fakedStats = stats;
    Aya::Time::Interval timeSinceAwoke = Aya::Time::now<Aya::Time::Fast>() - lastWakeTime;
    if (timeSinceAwoke < fakedStats.timespanSinceLastStep)
    {
        fakedStats.timespanSinceLastStep = timeSinceAwoke;
    }

    return computeStandardError(fakedStats, desiredFps);
}

} // namespace Aya