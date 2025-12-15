#pragma once

#include "DataModel/GlobalSettings.hpp"
#include "DataModel/DataModelJob.hpp"
#include "World/World.hpp"

namespace Aya
{



// A generic mechanism for displaying stats (like 3D FPS, network traffic, etc.)
extern const char* const sDebugSettings;
class DebugSettings : public GlobalAdvancedSettingsItem<DebugSettings, sDebugSettings>
{
private:
    bool stackTracingEnabled;

public:
    bool soundWarnings;
    bool fmodProfiling;
    bool enableProfiling;

    typedef enum
    {
        DontReport,
        Prompt,
        Report
    } ErrorReporting;
    ErrorReporting errorReporting;

    static std::string robloxVersion;
    static std::string robloxProductName;

    DebugSettings();
    static Reflection::BoundProp<bool> prop_stackTracingEnabled;
    static Reflection::BoundProp<bool> prop_ioEnabled;

    bool getStackTracingEnabled() const
    {
        return stackTracingEnabled;
    }

    int getLuaRamLimit() const;
    void setLuaRamLimit(int value);

    bool blockingRemove;
    void setBlockingRemove(bool value)
    {
        blockingRemove = value;
    }

    void noOpt() {}

    // "Errors"
    bool getIsProfilingEnabled() const;
    void setIsProfilingEnabled(bool value);
    double getProfilingWindow() const;
    void setProfilingWindow(double value);

    ErrorReporting getErrorReporting() const
    {
        return errorReporting;
    }
    void setErrorReporting(ErrorReporting value);

    Time::SampleMethod getTickCountPreciseOverride() const
    {
        return Time::preciseOverride;
    }
    void setTickCountPreciseOverride(Time::SampleMethod value)
    {
        Time::preciseOverride = value;
    }

    int videoMemory() const; // Mbytes
    std::string systemProductName() const;
    std::string getRobloxVersion() const
    {
        AYAASSERT(!robloxVersion.empty());
        return robloxVersion;
    }
    std::string getRobloxProductName() const
    {
        AYAASSERT(!robloxProductName.empty());
        return robloxProductName;
    }
    std::string osVer() const;
    int osPlatformId() const;
    std::string osPlatform() const;
    std::string deviceName() const;
    bool osIs64Bit() const;
    std::string gfxcard() const;
    std::string simd() const;

    // perf counters
    int nameDatabaseSize() const
    {
        return (int)Aya::Name::size();
    }
    int nameDatabaseBytes() const
    {
        return (int)Aya::Name::approximateMemoryUsage();
    }
    long instanceCount() const
    {
        return Diagnostics::Countable<Instance>::getCount();
    }
    long getPlayerCount() const;
    long getDataModelCount() const;
    long jobCount() const
    {
        return Diagnostics::Countable<TaskScheduler::Job>::getCount();
    }
};

extern const char* const sTaskSchedulerSettings;
class TaskSchedulerSettings : public GlobalAdvancedSettingsItem<TaskSchedulerSettings, sTaskSchedulerSettings>
{
    TaskScheduler::ThreadPoolConfig threadPoolConfig;

public:
    TaskSchedulerSettings();

    void addDummyJob(bool exclusive, double fps);

    unsigned int threadPoolSize() const
    {
        return TaskScheduler::singleton().threadPoolSize();
    }
    double threadAffinity() const
    {
        return TaskScheduler::singleton().threadAffinity();
    }
    double numSleepingJobs() const
    {
        return TaskScheduler::singleton().numSleepingJobs();
    }
    double numWaitingJobs() const
    {
        return TaskScheduler::singleton().numWaitingJobs();
    }
    double numRunningJobs() const
    {
        return TaskScheduler::singleton().numRunningJobs();
    }
    double schedulerRate() const
    {
        return TaskScheduler::singleton().schedulerRate();
    }
    double schedulerDutyCyclePerThread() const
    {
        return TaskScheduler::singleton().getSchedulerDutyCyclePerThread();
    }

    bool getIsArbiterThrottled() const
    {
        return SimpleThrottlingArbiter::isThrottlingEnabled;
    }
    void setIsArbiterThrottled(bool value);
    double getThrottledJobSleepTime() const
    {
        return TaskScheduler::Job::throttledSleepTime;
    }
    void setThrottledJobSleepTime(double value);

    TaskScheduler::PriorityMethod getPriorityMethod() const
    {
        return TaskScheduler::priorityMethod;
    }
    void setPriorityMethod(TaskScheduler::PriorityMethod value);

    TaskScheduler::Job::SleepAdjustMethod getSleepAdjustMethod() const
    {
        return TaskScheduler::Job::sleepAdjustMethod;
    }
    void setSleepAdjustMethod(TaskScheduler::Job::SleepAdjustMethod value);

    TaskScheduler::ThreadPoolConfig getThreadPoolConfig() const;
    void setThreadPoolConfig(TaskScheduler::ThreadPoolConfig value);

    void setThreadShare(double timeSlice, int divisor);

    DataModelArbiter::ConcurrencyModel getConcurrencyModel() const
    {
        return DataModelArbiter::concurrencyModel;
    }
    void setConcurrencyModel(DataModelArbiter::ConcurrencyModel value);
};


} // namespace Aya
