#pragma once

#include "DataModel/DataModelJob.hpp"

namespace Aya
{

class SleepingJob : public DataModelJob
{
private:
    Aya::atomic<int> isAwake;
    const double desiredFps;
    Aya::Time lastWakeTime;

public:
    SleepingJob(const char* name, TaskType taskType, bool isPerPlayer, shared_ptr<Aya::DataModelArbiter> arbiter, Aya::Time::Interval stepBudget,
        double desiredFps);

    void wake();
    void sleep();

    virtual Aya::Time::Interval sleepTime(const Stats&);

    virtual Aya::TaskScheduler::Job::Error error(const Stats& stats);
};

} // namespace Aya
