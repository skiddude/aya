// TaskScheduler.cpp : Defines the entry point for the console application.
//

#include "TaskScheduler.hpp"
#include "TaskScheduler.Job.hpp"
#include "Debug.hpp"
#include "FastLog.hpp"
#include "boost.hpp"
#include "boost/scoped_ptr.hpp"
#include "ProcessPerfCounter.hpp"
#include "Profiler.hpp"
#include "DataModel/GameBasicSettings.hpp"

using namespace Aya;
using boost::shared_ptr;

// #define TASKSCHEDULAR_PROFILING

LOGVARIABLE(TaskSchedulerTiming, 0)

FASTFLAGVARIABLE(TaskSchedulerCyclicExecutive, true)
FASTFLAGVARIABLE(DebugTaskSchedulerProfiling, false)
DYNAMIC_FASTINTVARIABLE(TaskSchedularBatchErrorCalcFPS, 300)
DYNAMIC_FASTFLAGVARIABLE(CyclicExecutiveForServerTweaks, false)

Aya::TaskScheduler::PriorityMethod TaskScheduler::priorityMethod = AccumulatedError;
// Aya::TaskScheduler::PriorityMethod TaskScheduler::priorityMethod = FIFO;

#ifdef AYA_TEST_BUILD
int TaskScheduler::findJobFPS = 100;
bool TaskScheduler::updateJobPriorityOnWake = false;
#endif

double TaskScheduler::getSchedulerDutyCyclePerThread() const
{
    return threads.size() ? schedulerDutyCycle.dutyCycle() / threads.size() : 0;
}

Aya::thread_specific_reference<TaskScheduler::Job> TaskScheduler::currentJob;

struct PrintTaskSchedulerItem
{
    int count;
    double averageError;
    double averagePriority;
    double dutyCycle;
    double averageStepsPerSecond;
    double coefficient_of_variation;
    double averageStepTime;
    PrintTaskSchedulerItem()
        : count(0)
        , averageError(0)
        , averagePriority(0)
        , dutyCycle(0)
        , averageStepsPerSecond(0)
        , coefficient_of_variation(0)
        , averageStepTime(0)
    {
    }
};

static std::string computeKey(const Aya::TaskScheduler::Job* job)
{
    std::string key = job->name;
    size_t index = key.find(':');
    if (index == std::string::npos)
        return key;

    index = key.find_last_of(' ');
    key = job->name.substr(0, index);

    return key;
}

void PrintArbiters(std::vector<boost::shared_ptr<const Aya::TaskScheduler::Job>>& jobs)
{
    std::set<shared_ptr<Aya::TaskScheduler::Arbiter>> arbiters;
    for (std::vector<boost::shared_ptr<const Aya::TaskScheduler::Job>>::iterator iter = jobs.begin(); iter != jobs.end(); ++iter)
    {
        if ((*iter)->getArbiter())
            arbiters.insert((*iter)->getArbiter());
    }

    double total = 0;
    for (std::set<shared_ptr<Aya::TaskScheduler::Arbiter>>::iterator iter = arbiters.begin(); iter != arbiters.end(); ++iter)
    {
        double activity = (*iter)->getAverageActivity();
        printf("Arbiter %s\t%.1f%%\t%s\n", (*iter)->arbiterName().c_str(), 100.0 * activity, (*iter)->isThrottled() ? "throttled" : "");
        total += (*iter)->getAverageActivity();
    }
    printf("Total activity\t%.1f%%\n", 100.0 * total);
}

void PrintTasks(std::vector<boost::shared_ptr<const Aya::TaskScheduler::Job>>& jobs)
{
    printf("%25.25s\tSleep\tPrior\t%%\tSteps\tCV\tStep\n", "Arbiter:TaskName");
    for (std::vector<boost::shared_ptr<const Aya::TaskScheduler::Job>>::iterator iter = jobs.begin(); iter != jobs.end(); ++iter)
    {
        if ((*iter)->getSleepingTime() > Time::Interval(2))
            printf("%25.25s\tAsleep for %.1fs\n", (*iter)->getDebugName().c_str(), (*iter)->getSleepingTime().seconds());
        else
            printf("%25.25s\t%d%%\t%.1g\t%.1f%%\t%.1f/s\t%.1f%%\t%.3fs\n", (*iter)->getDebugName().c_str(), int(100.0 * (*iter)->averageSleepRate()),
                (*iter)->getPriority(), 100.0 * (*iter)->averageDutyCycle(), (*iter)->averageStepsPerSecond(),
                100.0 * (*iter)->getStepStats().stepInterval().coefficient_of_variation(), (*iter)->averageStepTime());
    }
}

void PrintAggregatedTasks(std::vector<boost::shared_ptr<const Aya::TaskScheduler::Job>>& jobs)
{
    std::map<std::string, PrintTaskSchedulerItem> items;
    for (std::vector<boost::shared_ptr<const Aya::TaskScheduler::Job>>::iterator iter = jobs.begin(); iter != jobs.end(); ++iter)
    {
        std::string key = computeKey(iter->get());
        if ((*iter)->getSleepingTime() < Time::Interval(5))
        {
            PrintTaskSchedulerItem& item(items[key]);
            item.count++;
            item.averageError += (*iter)->averageError();
            item.averagePriority += (*iter)->getPriority();
            item.dutyCycle += (*iter)->averageDutyCycle();
            item.averageStepsPerSecond += (*iter)->averageStepsPerSecond();
            item.coefficient_of_variation += (*iter)->getStepStats().stepInterval().coefficient_of_variation();
            item.averageStepTime += (*iter)->averageStepTime();
        }
    }

    printf("%15.15s\tCount\tError\tPrior\t%%\tSteps\tCV\tStep\n", "Task");
    for (std::map<std::string, PrintTaskSchedulerItem>::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        PrintTaskSchedulerItem& item(iter->second);
        printf("%15.15s\t%d\t%.2f\t%.1f\t%.1f%%\t%.1f/s\t%.1f%%\t%.3fs\n", iter->first.c_str(), item.count, item.averageError / (double)item.count,
            item.averagePriority / (double)item.count, 100.0 * item.dutyCycle, item.averageStepsPerSecond / (double)item.count,
            100.0 * item.coefficient_of_variation / (double)item.count, item.averageStepTime / (double)item.count);
    }
}

void TaskScheduler::printDiagnostics(bool aggregateJobs)
{
    std::vector<boost::shared_ptr<const Aya::TaskScheduler::Job>> jobs;
    getJobsInfo(jobs);

    if (aggregateJobs)
        PrintAggregatedTasks(jobs);
    else
        PrintTasks(jobs);

    PrintArbiters(jobs);

    printf("sleep %.1f, wait %.1f, run %.2f, affinity %.2f, scheduling %.1f/s (%.2g%%)\n", numSleepingJobs(), numWaitingJobs(), numRunningJobs(),
        threadAffinity(), schedulerRate(), getSchedulerDutyCyclePerThread() * 100);

    printf("\n");
}



Aya::ExclusiveArbiter Aya::ExclusiveArbiter::singleton;

bool Aya::ExclusiveArbiter::areExclusive(TaskScheduler::Job* task1, TaskScheduler::Job* task2)
{
    AYAASSERT(task1->hasArbiter(this));
    AYAASSERT(task2->hasArbiter(this));
    return true;
}

static TaskScheduler* sing;
void TaskScheduler::static_init()
{
    static TaskScheduler s;
    s.setThreadCount(TaskScheduler::Auto); // Use auto by default
    sing = &s;
}
TaskScheduler& TaskScheduler::singleton()
{
    static boost::once_flag flag = BOOST_ONCE_INIT;
    boost::call_once(static_init, flag);
    return *sing;
}

const static double lerpTaskScheduler = 0.05;

TaskScheduler::TaskScheduler()
    : sampleRunningJobCountEvent(true)
    , threadCount(0)
    , threadAffinityPreference(1.5) // TODO: Come up with a good number here
    , runningJobCount(0)
    , waitingJobCount(lerpTaskScheduler)
    , sleepingJobCount(lerpTaskScheduler)
    , averageRunningJobCount(lerpTaskScheduler)
    , schedulerDutyCycle(lerpTaskScheduler)
    , averageThreadAffinity(lerpTaskScheduler, 1)
    , nextWakeTime(Time::max())
    , lastSortTime(Time())
    , desiredThreadCount(0)
    , cyclicExecutiveLoopId(0)
    , DataModel30fpsThrottle(true)
    , cyclicExecutiveWaitForNextFrame(false)
    , nonCyclicJobsToDo(0)
    , lastCyclcTimestamp(Time::now<Time::Fast>())
{
    runningJobCounterThread.reset(
        new boost::thread(Aya::thread_wrapper(boost::bind(&TaskScheduler::sampleRunningJobCount, this), "Roblox sampleRunningJobCount")));

    // Publish the fast flag out to the schedulers API so that it can be overridden by specific clients.
    // E.g. this is not yet something to be done on the server.
    cyclicExecutiveEnabled = FFlag::TaskSchedulerCyclicExecutive;
}

TaskScheduler::~TaskScheduler()
{
    FASTLOG(FLog::TaskSchedulerInit, "Destroying TaskScheduler");
    sampleRunningJobCountEvent.Set();
    runningJobCounterThread->join();

    endAllThreads();
}

void TaskScheduler::remove(boost::shared_ptr<TaskScheduler::Job> task, bool joinTask, boost::function<void()> callbackPing)
{
    if (!task)
        return;

    // You can't join to yourself, but logically
    // there should be no concurrency risk to not join
    joinTask &= task.get() != currentJob.get();

    shared_ptr<CEvent> joinEvent(joinTask ? new CEvent(true) : NULL);

    remove(task, joinEvent);

    if (joinTask)
    {
        if (callbackPing)
        {
            for (int i = 0; i < 10 * 60 * 5; ++i)
            {
                callbackPing();
                if (joinEvent->Wait(100))
                    return;
            }
#ifdef _DEBUG
            AYAASSERT(false); // Why did it take so long to join?
#endif
            AYACRASH(); // We want to learn about this. Blocking a long time means a thread in the pool is locked up!
        }
        else
        {
            if (!joinEvent->Wait(1000 * 60 * 5))
            {
#ifdef _DEBUG
                AYAASSERT(false); // Why did it take so long to join?
#endif
                AYACRASH(); // We want to learn about this. Blocking a long time means a thread in the pool is locked up!
            }
        }
    }
}



void TaskScheduler::reschedule(boost::shared_ptr<Job> job)
{
    {
        Aya::mutex::scoped_lock lock(mutex);
        if (job->SleepingHook::is_linked())
        {
            AYAASSERT(!cyclicExecutiveEnabled || job->cyclicExecutive == false);

            sleepingJobs.erase(sleepingJobs.iterator_to(*job));
            scheduleJob(*job);
        }
        else if (cyclicExecutiveEnabled && job->cyclicExecutive)
        {
            CyclicExecutiveJobs::iterator i = std::find(cyclicExecutiveJobs.begin(), cyclicExecutiveJobs.end(), job);
            AYAASSERT(i != cyclicExecutiveJobs.end());
            if (i != cyclicExecutiveJobs.end())
            {
                // Reschedule immediately without waiting for other jobs.  Prevents theoretical deadlocks.
                i->cyclicExecutiveExecuted = false;
            }
        }
    }
}

bool TaskScheduler::jobCompare(const CyclicExecutiveJob& jobA, const CyclicExecutiveJob& jobB)
{
    return (jobA.job->cyclicPriority < jobB.job->cyclicPriority);
}

void TaskScheduler::add(boost::shared_ptr<Job> job)
{
    Aya::mutex::scoped_lock lock(mutex);
    AYAASSERT(allJobs.find(job) == allJobs.end());
    allJobs.insert(job);

    if (cyclicExecutiveEnabled)
    {
        if (job->cyclicExecutive)
        {
            AYAASSERT(std::find(cyclicExecutiveJobs.begin(), cyclicExecutiveJobs.end(), job) == cyclicExecutiveJobs.end());
            cyclicExecutiveJobs.push_back(job);
            std::sort(cyclicExecutiveJobs.begin(), cyclicExecutiveJobs.end(), jobCompare);
        }
        else
        {
            scheduleJob(*job);
        }
    }
    else
    {
        job->cyclicExecutive = false;
        scheduleJob(*job);
    }
}

template<class List, class Item, class IsLess>
void insert_from_back(List& list, Item& f, IsLess isLess)
{
    for (typename List::reverse_iterator iter = list.rbegin(); iter != list.rend(); ++iter)
        if (!isLess(f, *iter))
        {
            list.insert(iter.base(), f);
            return;
        }
    list.push_front(f);
}

void TaskScheduler::scheduleJob(Job& job)
{
    if (cyclicExecutiveEnabled && job.cyclicExecutive)
    {
        CyclicExecutiveJobs::iterator i = std::find(cyclicExecutiveJobs.begin(), cyclicExecutiveJobs.end(), job);
        AYAASSERT(i != cyclicExecutiveJobs.end());
        if (i != cyclicExecutiveJobs.end())
        {
            i->isRunning = false;
        }
        return;
    }

    job.updateWakeTime();
    const Time now = Time::now<Time::Fast>();

    if (job.wakeTime <= now)
    {
#ifdef AYA_TEST_BUILD
        errorCalculationPerSec.sample();
#endif
        job.updateError(now);
        if (job.currentError.error > 0)
        {
            job.sleepRate.sample(0);
            job.startWaiting();

#ifdef AYA_TEST_BUILD
            if (updateJobPriorityOnWake)
#endif
            {
                job.updatePriority();
            }
            enqueueWaitingJob(job);
            return;
        }
    }

    job.sleepRate.sample(1);
    job.startSleeping();
    insert_from_back(sleepingJobs, job, Job::isLowerWakeTime);
}


void TaskScheduler::remove(const boost::shared_ptr<TaskScheduler::Job>& job, boost::shared_ptr<CEvent> joinEvent)
{
    Aya::mutex::scoped_lock lock(mutex);
    AYAASSERT(allJobs.find(job) != allJobs.end());
    FASTLOG1(FLog::TaskSchedulerRun, "Removing job %p from allJobs (::remove)", job.get());
    allJobs.erase(job);

    if (job->SleepingHook::is_linked())
    {
        AYAASSERT(!cyclicExecutiveEnabled || job->cyclicExecutive == false);
        FASTLOG1(FLog::TaskSchedulerRun, "Removing job %p from sleepingJobs (::remove)", job.get());
        sleepingJobs.erase(sleepingJobs.iterator_to(*job));
        if (joinEvent)
            joinEvent->Set();
    }
    else if (job->WaitingHook::is_linked())
    {
        AYAASSERT(!cyclicExecutiveEnabled || job->cyclicExecutive == false);
        FASTLOG1(FLog::TaskSchedulerRun, "Removing job %p from waitingJobs (::remove)", job.get());

        waitingJobs.erase(waitingJobs.iterator_to(*job));

        if (joinEvent)
            joinEvent->Set();
    }
    else
    {
        if (cyclicExecutiveEnabled && job->cyclicExecutive)
        {
            CyclicExecutiveJobs::iterator i = std::find(cyclicExecutiveJobs.begin(), cyclicExecutiveJobs.end(), job);
            AYAASSERT(i != cyclicExecutiveJobs.end());
            if (i != cyclicExecutiveJobs.end())
            {
                if (!i->isRunning)
                {
                    cyclicExecutiveJobs.erase(i);
                    if (joinEvent)
                        joinEvent->Set();
                    return;
                }
            }
        }

        if (joinEvent)
        {
            AYAASSERT(!job->joinEvent); // Did somebody else already try to remove this task????
            // TODO: Support multiple joins by using a collection?

            // We must wait for the task to be completed before signaling the joinEvent
            job->joinEvent = joinEvent;
        }

        // We can't remove it yet. Wait for it to finish stepping
        job->isRemoveRequested = true;
    }
}

void TaskScheduler::incrementThreadCount()
{
    ++threadCount;
}
void TaskScheduler::decrementThreadCount()
{
    --threadCount;
}

bool TaskScheduler::areExclusive(TaskScheduler::Job* task1, TaskScheduler::Job* task2, const shared_ptr<Arbiter>& arbiterHint)
{
    if (Job::haveDifferentArbiters(task1, task2))
        return false; // different Arbiter domains can run concurrently

    if (!arbiterHint)
        return false; // No Arbiter means the task can run concurrently

    return arbiterHint->areExclusive(task1, task2);
}

void TaskScheduler::sampleRunningJobCount()
{
    while (!sampleRunningJobCountEvent.Wait(71))
    {
        waitingJobCount.sample(int(waitingJobs.size() + cyclicExecutiveJobs.size()));
        sleepingJobCount.sample(int(sleepingJobs.size()));
        averageRunningJobCount.sample(int(runningJobCount));
    }
}


void TaskScheduler::enqueueWaitingJob(Job& job)
{
    FASTLOG1(FLog::TaskSchedulerFindJob, "Adding job %p to waitingJobs (::enqueueWaitingJob)", &job);

    AYAASSERT(!cyclicExecutiveEnabled || job.cyclicExecutive == false);

    waitingJobs.push_back(job);
}


Time::Interval TaskScheduler::getShortestSleepTime() const
{
    return nextWakeTime - Time::now<Time::Fast>();
}


void TaskScheduler::wakeSleepingJobs()
{
    Time now = Time::now<Time::Fast>();

    SleepingJobs::iterator iter = sleepingJobs.begin();
    while (iter != sleepingJobs.end())
    {
        Job& job(*iter);
        AYAASSERT(!cyclicExecutiveEnabled || job.cyclicExecutive == false);

        if (job.wakeTime > now)
        {
            nextWakeTime = job.wakeTime;
            return;
        }
        else
        {
            iter = sleepingJobs.erase(iter);
#ifdef AYA_TEST_BUILD
            errorCalculationPerSec.sample();
#endif
            job.updateError(now);

#ifdef AYA_TEST_BUILD
            if (updateJobPriorityOnWake)
#endif
            {
                job.updatePriority();
            }
            enqueueWaitingJob(job);
        }
    }
    nextWakeTime = Time::max();
}

void TaskScheduler::checkStillWaitingNextFrame(Time now)
{
    float fpsCap = 60.0f;

    switch (Aya::GameBasicSettings::singleton().getMaxFramerate())
    {
    case Aya::GameBasicSettings::MaxFramerate::FPS_30:
        fpsCap = 30.0f;
        break;
    case Aya::GameBasicSettings::MaxFramerate::FPS_60:
        fpsCap = 60.0f;
        break;
    case Aya::GameBasicSettings::MaxFramerate::FPS_75:
        fpsCap = 75.0f;
        break;
    case Aya::GameBasicSettings::MaxFramerate::FPS_120:
        fpsCap = 120.0f;
        break;
    case Aya::GameBasicSettings::MaxFramerate::FPS_144:
        fpsCap = 144.0f;
        break;
    case Aya::GameBasicSettings::MaxFramerate::FPS_200:
        fpsCap = 200.0f;
        break;
    case Aya::GameBasicSettings::MaxFramerate::FPS_240:
        fpsCap = 240.0f;
        break;
    case Aya::GameBasicSettings::MaxFramerate::FPS_360:
        fpsCap = 360.0f;
        break;
    case Aya::GameBasicSettings::MaxFramerate::FPS_UNCAPPED:
        fpsCap = 9999.0f;
        break;
    }

    float minTimespan = 1.0 / fpsCap;
    if ((now - lastCyclcTimestamp).seconds() < minTimespan)
    {
        // Do nothing
    }
    else
    {
        lastCyclcTimestamp = now;
        cyclicExecutiveWaitForNextFrame = false;
    }
}

boost::shared_ptr<TaskScheduler::Job> TaskScheduler::findJobToRun(shared_ptr<Thread> requestingThread)
{
#ifdef TASKSCHEDULAR_PROFILING
    int theCount = 0;
    Aya::Timer<Aya::Time::Precise> timer;
#endif
    Time now = Time::now<Time::Fast>();

    if (cyclicExecutiveEnabled)
    {
        // Once all "cyclic jobs" have been done, this code falls through to the low priority queue one time.
        bool hasRemaingJobs = false;

        if ((nonCyclicJobsToDo == 0) && cyclicExecutiveWaitForNextFrame && DataModel30fpsThrottle)
        {
            checkStillWaitingNextFrame(now);
        }

        if (!cyclicExecutiveWaitForNextFrame || !DataModel30fpsThrottle)
        {
            for (CyclicExecutiveJobs::iterator iter = cyclicExecutiveJobs.begin(); iter != cyclicExecutiveJobs.end(); ++iter)
            {
                Job& job(*iter->job);
                if (iter->cyclicExecutiveExecuted)
                {
                    continue;
                }

                if (job.isDisabled())
                    continue;

                hasRemaingJobs = true;

                if (iter->isRunning)
                {
                    continue;
                }

                if (conflictsWithScheduledJob(&job))
                    continue;

                if (job.tryJobAgain())
                {
                    break;
                }

                iter->cyclicExecutiveExecuted = true;

                job.updateError(now);
                if (job.currentError.isDefault())
                {
                    // Various jobs, including all the rendering tasks, signal they do not want
                    // to run by setting their error to 0.
                    continue;
                }

                iter->isRunning = true;

                shared_ptr<Job> result = job.shared_from_this();
                FASTLOG2(FLog::TaskSchedulerFindJob, "CyclicExecutive, job: %p, arbiter %p", result.get(), result->getArbiter().get());
                return result;
            }

            if (!hasRemaingJobs)
            {
                for (CyclicExecutiveJobs::iterator iter = cyclicExecutiveJobs.begin(); iter != cyclicExecutiveJobs.end(); ++iter)
                {
                    iter->cyclicExecutiveExecuted = false;
                }
                cyclicExecutiveWaitForNextFrame = true;
                nonCyclicJobsToDo = numNonCyclicJobsWithWork();
                FASTLOG1F(FLog::TaskSchedulerTiming, "Finished all jobs in Cycle in: %4.7f",
                    (float)(Time::now<Time::Fast>() - lastCyclcTimestamp).seconds());
            }
        }

        if (nonCyclicJobsToDo > 0)
        {
            // Run nonCyclicExecutive jobs that have been in the Queue before moving on to
            // the next CyclicExecutive frame
            shared_ptr<Job> result = findJobToRunNonCyclicJobs(requestingThread, now);
            if (result)
            {
                // At end of Cyclic RenderJob will have DataModel Lock
                // We must make sure that we only decrement jobs to do when
                // we do them, because we are almost guaranteed that result
                // will be null the first few checks.
                nonCyclicJobsToDo--;
            }
            else if (numNonCyclicJobsWithWork() == 0)
            {
                // In case something modifies the jobs after end of cycle
                // we want to be able to verify that job's error didn't
                // return to 0
                nonCyclicJobsToDo = 0;
            }
            return result;
        }
        else
        {
            // If tryJobAgain breaks out of the loop, we want the opportunity to do
            // work on Non-Cyclic jobs while we wait for job to be ready.
            shared_ptr<Job> result = findJobToRunNonCyclicJobs(requestingThread, now);
            return result;
        }
    }
    else // !cyclicExecutiveEnabled
    {
        // nextScheduledJob is an optimization. Under heavy load, we cut the time for the scheduler
        // in half because it buffers a job from the last go-around.
        // TODO: Buffer n jobs instead?
        // TODO: Buffer one job per thread to help with thread affinity?
        if (nextScheduledJob)
        {
            shared_ptr<Job> result(nextScheduledJob);
            nextScheduledJob.reset();

            if (result->WaitingHook::is_linked())
                if (!result->isDisabled())
                    if (!conflictsWithScheduledJob(result.get()))
                    {
                        FASTLOG1(FLog::TaskSchedulerFindJob, "Removing nextScheduledJob %p from waitingJobs (::findJobToRun)", result.get());
                        waitingJobs.erase(waitingJobs.iterator_to(*result));
                        averageThreadAffinity.sample(result->lastThreadUsed.lock() == requestingThread);
                        return result;
                    }
        }

        return findJobToRunNonCyclicJobs(requestingThread, now);
    }
}

int TaskScheduler::numNonCyclicJobsWithWork()
{
    int jobCount = 0;
    for (WaitingJobs::iterator iter = waitingJobs.begin(); iter != waitingJobs.end(); ++iter)
    {
        Job& job(*iter);
        if (job.currentError.error > 0 && !job.isDisabled())
        {
            // We only add jobs that have work to do!
            jobCount++;
        }
    }

    return jobCount;
}

shared_ptr<TaskScheduler::Job> TaskScheduler::findJobToRunNonCyclicJobs(boost::shared_ptr<Thread> requestingThread, Aya::Time now)
{
    shared_ptr<TaskScheduler::Job> result;
    wakeSleepingJobs();

    WaitingJobs::iterator bestJob = waitingJobs.end();

    // Note: These definitions don't strictly need to be initialized, but it eliminates pesky compiler warnings
    bool bestHasAffinity = false;
    bool bestIsThrottled = false;

    bool shouldCalcError;
    Time::Interval timeSinceLastSorting = now - lastSortTime;
    int fps = DFInt::TaskSchedularBatchErrorCalcFPS;
    if (fps < 10)
    {
        fps = 10;
    }
    double errorCalcInterval = 1.f / (double)fps;

#ifdef AYA_TEST_BUILD
    errorCalcInterval = 1.f / findJobFPS;
#endif
    shouldCalcError = (timeSinceLastSorting.seconds() > errorCalcInterval);
    if (shouldCalcError)
    {
#ifdef AYA_TEST_BUILD
        sortFrequency.sample();
#endif
        lastSortTime = now;
    }

    FASTLOG1(FLog::TaskSchedulerFindJob, "Starting to iterate through waitingJobs. Size = %d", waitingJobs.size());

    for (WaitingJobs::iterator iter = waitingJobs.begin(); iter != waitingJobs.end(); ++iter)
    {
#ifdef TASKSCHEDULAR_PROFILING
        theCount++;
#endif
        Job& job(*iter);

        if (job.isDisabled())
            continue;

        if (priorityMethod != FIFO)
        {
            if (shouldCalcError)
            {
#ifdef AYA_TEST_BUILD
                errorCalculationPerSec.sample();
#endif
                job.updateError(now);
            }

            if (job.currentError.error <= 0)
            {
                continue;
            }
        }

        if (conflictsWithScheduledJob(&job))
        {
            continue;
        }

        if (shouldCalcError)
        {
            job.updatePriority();
        }

        const bool hasAffinity = job.lastThreadUsed.lock() == requestingThread;
        bool match = false;

        shared_ptr<Arbiter> const arbiter(job.getArbiter());

        if (bestJob == waitingJobs.end())
        {
            match = true;
        }
        else if (job.currentError.urgent != bestJob->currentError.urgent)
        {
            if (job.currentError.urgent)
                match = true;
        }
        else if (bestIsThrottled && (!arbiter || !arbiter->isThrottled()))
        {
            match = true;
        }
        else if (hasAffinity == bestHasAffinity)
        {
            if (job.priority > bestJob->priority)
                match = true;
        }
        else if (hasAffinity)
        {
            if (job.priority * threadAffinityPreference > bestJob->priority)
                match = true;
        }
        else
        {
            if (job.priority > bestJob->priority * threadAffinityPreference)
                match = true;
        }

        if (match)
        {
            if (!cyclicExecutiveEnabled)
            {
                // Cache off old best job
                if (bestJob != waitingJobs.end())
                {
                    if (!areExclusive(&job, &*bestJob, arbiter))
                    {
                        nextScheduledJob = bestJob->shared_from_this();
                    }
                    else if (nextScheduledJob && areExclusive(&job, &*nextScheduledJob, arbiter))
                    {
                        nextScheduledJob.reset();
                    }
                }
            }

            bestJob = iter;
            bestHasAffinity = hasAffinity;

            if (priorityMethod == FIFO)
                break;

            bestIsThrottled = arbiter && arbiter->isThrottled();
        }
    }

    FASTLOG(FLog::TaskSchedulerFindJob, "Finished iterating through waitingJobs");

    if (bestJob != waitingJobs.end())
    {
        result = bestJob->shared_from_this();
        AYAASSERT(bestJob->WaitingHook::is_linked());
        waitingJobs.erase(waitingJobs.iterator_to(*bestJob));
        averageThreadAffinity.sample(bestHasAffinity);
        FASTLOG3(FLog::TaskSchedulerFindJob, "RunJob, job: %p, arbiter %p error: %u", result.get(), result->getArbiter().get(),
            (unsigned)result->currentError.error);
    }


#ifdef TASKSCHEDULAR_PROFILING
    if (FFlag::DebugTaskSchedulerProfiling)
    {
        static Aya::Timer<Aya::Time::Fast> totalTimer;
        static int totalCount = 0;
        static double taskSchedulerTime = 0.f;
        static int lastTotalCount = 0;

        double timeElapsed = timer.delta().seconds();
        taskSchedulerTime += timeElapsed;
        totalCount += theCount;
        if (totalCount - lastTotalCount >= 1000000)
        {
            lastTotalCount = totalCount;
            double totalTimeElapsed = totalTimer.delta().seconds();
            printf("Jobs: %d, time used: %f, total time: %f, job/s: %d, percent: %.2f%%\n", totalCount, taskSchedulerTime, totalTimeElapsed,
                (int)((double)totalCount / totalTimeElapsed), taskSchedulerTime / totalTimeElapsed * 100.f);
        }
    }
#endif
    return result;
}

Aya::atomic<int> Aya::SimpleThrottlingArbiter::arbiterCount;
bool Aya::SimpleThrottlingArbiter::isThrottlingEnabled = false;
