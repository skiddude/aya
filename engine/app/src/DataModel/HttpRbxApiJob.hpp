#include "Tree/Instance.hpp"
#include "DataModel/DataModelJob.hpp"
#include "DataModel/DataModel.hpp"
#include "DataModel/HttpRbxApiService.hpp"

DYNAMIC_FASTINTVARIABLE(HttpRbxApiJobFrequencyInSeconds, 1);

namespace Aya
{


class HttpRbxApiJob : public DataModelJob
{
    shared_ptr<HttpRbxApiService> apiService;

    int lastJobFrequency;
    double desiredHz;

public:
    HttpRbxApiJob(HttpRbxApiService* owner)
        : DataModelJob("HttpRbxApiJob", DataModelJob::Write, false, shared_from(DataModel::get(owner)), Time::Interval(0.01))
        , apiService(shared_from(owner))
    {
        updateHz();
    }

    void updateHz()
    {
        desiredHz = 1.0f / DFInt::HttpRbxApiJobFrequencyInSeconds;
        lastJobFrequency = DFInt::HttpRbxApiJobFrequencyInSeconds;
    }

    /*override*/ Time::Interval sleepTime(const Stats& stats)
    {
        return computeStandardSleepTime(stats, desiredHz);
    }
    /*override*/ Job::Error error(const Stats& stats)
    {
        return computeStandardError(stats, desiredHz);
    }
    /*override*/ TaskScheduler::StepResult stepDataModelJob(const Stats& stats)
    {
        if (DFInt::HttpRbxApiJobFrequencyInSeconds != lastJobFrequency)
            updateHz();

        apiService->addThrottlingBudgets(DFInt::HttpRbxApiJobFrequencyInSeconds / 60.0f);
        apiService->executeThrottledRequests();
        apiService->executeRetryRequests();

        return TaskScheduler::Stepped;
    }
};

} // namespace Aya