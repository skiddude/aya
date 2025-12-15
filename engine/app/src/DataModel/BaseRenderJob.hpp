#pragma once

#include <boost/weak_ptr.hpp>

#include "DataModel/DataModel.hpp"
#include "time.hpp"
#include "TaskScheduler.hpp"
#include "TaskScheduler.Job.hpp"
#include "Utility/IMetric.hpp"


namespace Aya
{
class View;

// This is the base class for all Rendering Jobs. All rendering jobs will inherit from this.

class BaseRenderJob : public DataModelJob
{
protected:
    Time lastRenderTime;
    volatile bool isAwake;
    double minFrameRate;
    double maxFrameRate;


public:
    BaseRenderJob(double minFrameRate, double maxFrameRate, boost::shared_ptr<DataModel> dataModel);

    virtual void wake();
    virtual bool tryJobAgain();
    virtual bool isCyclicExecutiveJob();

    virtual Time::Interval timeSinceLastRender() const;
    virtual Job::Error error(const Stats& stats);
    // virtual Aya::TaskScheduler::StepResult stepDataModelJob(const Stats&);

    virtual TaskScheduler::StepResult step(const Stats& stats);
};

} // namespace Aya