#pragma once

#include <boost/weak_ptr.hpp>

#include "DataModel/BaseRenderJob.hpp"

#include "DataModel/DataModel.hpp"

#include "time.hpp"

#include "TaskScheduler.hpp"

#include "TaskScheduler.Job.hpp"

#include "Utility/IMetric.hpp"


namespace Aya
{

class FunctionMarshaller;
class View;
class ViewBase;

// This job calls ViewBase::render(), which needs to be done exclusive to the
// DataModel. This is why it has the DataModelJob::Render enum, which
// prevents concurrent writes to DataModel. It also needs to run in the view's
// thread for OpenGL.
// TODO: Can Ogre be modified to not require the thread?
class RenderJob
    : public BaseRenderJob
    , public IMetric
{
    FunctionMarshaller* marshaller;
    View* robloxView;
    volatile int stopped;

    CEvent prepareBeginEvent;
    CEvent prepareEndEvent;

    static void scheduleRender(weak_ptr<RenderJob> selfWeak, ViewBase* view, double timeJobStart);

public:
    RenderJob(View* robloxView, FunctionMarshaller* marshaller, boost::shared_ptr<DataModel> dataModel);

    Time::Interval timeSinceLastRender() const;
    Time::Interval sleepTime(const Stats& stats);

    virtual TaskScheduler::StepResult stepDataModelJob(const Stats& stats);

    virtual std::string getMetric(const std::string& metric) const;
    virtual double getMetricValue(const std::string& metric) const;

    void stop();
};

} // namespace Aya
