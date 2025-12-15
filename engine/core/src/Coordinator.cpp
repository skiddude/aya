
#include "Coordinator.hpp"

using namespace Aya;
using namespace Tasks;

bool Barrier::isInhibited(TaskScheduler::Job* job)
{
    Aya::mutex::scoped_lock lock(mutex);
    return jobs[job] > counter;
}

void Barrier::onPostStep(TaskScheduler::Job* job)
{
    Aya::mutex::scoped_lock lock(mutex);

    unsigned int& count(jobs[job]);

    // Note: we could almost assert that count==counter, but
    //       there may be edge cases where a step slips through
    //       the cracks
    if (count == counter)
    {
        count++;
        if (--remainingTasks == 0)
            releaseBarrier();
    }
}

void Barrier::releaseBarrier()
{
    remainingTasks = int(jobs.size());
    counter++;
}

void Barrier::onAdded(TaskScheduler::Job* job)
{
    Aya::mutex::scoped_lock lock(mutex);

    AYAASSERT(jobs.find(job) == jobs.end());

    jobs[job] = counter;
    remainingTasks++;

    AYAASSERT(remainingTasks <= jobs.size());
}

void Barrier::onRemoved(TaskScheduler::Job* job)
{
    Aya::mutex::scoped_lock lock(mutex);

    AYAASSERT(jobs.find(job) != jobs.end());

    if (jobs[job] <= counter)
        remainingTasks--;

    jobs.erase(job);

    AYAASSERT(remainingTasks <= jobs.size());
}




bool SequenceBase::isInhibited(TaskScheduler::Job* job)
{
    Aya::mutex::scoped_lock lock(mutex);

    if (nextJobIndex < jobs.size())
        return jobs[nextJobIndex] != job;
    else
        return true;
}

void SequenceBase::advance()
{
    Aya::mutex::scoped_lock lock(mutex);

    if (++nextJobIndex == jobs.size())
        nextJobIndex = 0;
}

void SequenceBase::onAdded(TaskScheduler::Job* job)
{
    Aya::mutex::scoped_lock lock(mutex);

    jobs.push_back(job);
}

void SequenceBase::onRemoved(TaskScheduler::Job* job)
{
    Aya::mutex::scoped_lock lock(mutex);

    for (size_t i = 0; i < jobs.size(); ++i)
        if (jobs[i] == job)
        {
            jobs.erase(jobs.begin() + i);
            if (nextJobIndex > i)
                --nextJobIndex;
            break;
        }
}


bool Exclusive::isInhibited(TaskScheduler::Job* job)
{
    AYAASSERT(job != runningJob);
    return runningJob != NULL;
}

void Exclusive::onPreStep(TaskScheduler::Job* job)
{
    AYAASSERT(runningJob == NULL);
    runningJob = job;
}

void Exclusive::onPostStep(TaskScheduler::Job* job)
{
    AYAASSERT(runningJob == job);
    runningJob = NULL;
}
