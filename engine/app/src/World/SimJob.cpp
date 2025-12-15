


#include "World/SimJob.hpp"
#include "World/Primitive.hpp"
#include "World/Assembly.hpp"
#include "Utility/StlExtra.hpp"

namespace Aya
{

const SimJob* SimJob::getConstSimJobFromPrimitive(const Primitive* primitive)
{
    if (primitive)
    {
        if (const Assembly* assembly = primitive->getConstAssembly())
        {
            if (const SimJob* answer = assembly->getConstSimJob())
            {
                return answer;
            }
        }
    }
    return NULL;
}

SimJob* SimJob::getSimJobFromPrimitive(Primitive* primitive)
{
    return const_cast<SimJob*>(getConstSimJobFromPrimitive(primitive));
}

bool SimJobTracker::containedBy(SimJob* s)
{
    return (std::find(s->trackers.begin(), s->trackers.end(), this) != s->trackers.end());
}


void SimJobTracker::stopTracking()
{
    if (simJob)
    {

        AYAASSERT(this->containedBy(simJob));

        fastRemoveShort<SimJobTracker*>(simJob->trackers, this);

        AYAASSERT(!this->containedBy(simJob));

        simJob = NULL;
    }
}

bool SimJobTracker::tracking()
{
    if (simJob)
    {
        AYAASSERT(this->containedBy(simJob));
        return true;
    }
    else
    {
        return false;
    }
}

void SimJobTracker::setSimJob(SimJob* m)
{
    stopTracking();

    if (m)
    {
        AYAASSERT(!this->containedBy(m));

        m->trackers.push_back(this);

        AYAASSERT(this->containedBy(m));

        simJob = m;
    }
}

SimJob* SimJobTracker::getSimJob()
{
    AYAASSERT(simJob);
    AYAASSERT(this->containedBy(simJob));
    return simJob;
}

void SimJobTracker::transferTrackers(SimJob* from, SimJob* to)
{
    AYAASSERT(from);
    AYAASSERT(from != to);

    while (!from->trackers.empty())
    {
        SimJobTracker* transfer = from->trackers.back();

        transfer->setSimJob(to);
    }
}

///////////////////////////////////////////////////////////////////////////////

SimJob::SimJob(Assembly* _assembly)
    : assembly(_assembly)
    , useCount(0)
{
    AYAASSERT(assembly->getConstSimJob() == NULL);
}


SimJob::~SimJob()
{
    AYAASSERT(useCount == 0);
    AYAASSERT(assembly->getConstSimJob() == NULL);
}

} // namespace Aya
