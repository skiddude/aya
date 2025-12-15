


#include "World/SendPhysics.hpp"
#include "World/Assembly.hpp"
#include "World/Mechanism.hpp"
#include "World/Primitive.hpp"


namespace Aya
{


SendPhysics::SendPhysics() {}


SendPhysics::~SendPhysics()
{
    AYAASSERT(simJobs.empty());
}

void SendPhysics::buildSimJob(SimJob* job)
{
    if (job->getAssembly())
    {
        AYAASSERT(!job->getConstAssembly()->getConstSimJob());
        job->getAssembly()->setSimJob(job);
    }

    simJobs.push_back(*job);
}


void SendPhysics::destroySimJob(SimJob* job)
{
    AYAASSERT_SLOW(job->is_linked());

    SimJob* transferTo = (simJobs.size() > 1) ? nextSimJob(job) : NULL;

    SimJobTracker::transferTrackers(job, transferTo);
    simJobs.erase(simJobs.iterator_to(*job));

    if (job->getAssembly())
    {
        AYAASSERT(job->getAssembly()->getSimJob() == job);
        job->getAssembly()->setSimJob(NULL);
    }
}

void SendPhysics::onMovingAssemblyRootAdded(Assembly* a)
{
    WriteValidator writeValidator(concurrencyValidator);

    AYAASSERT(Mechanism::isMovingAssemblyRoot(a));
    if (!a->getSimJob())
    {
        SimJob* job = new SimJob(a);
        buildSimJob(job);

        assemblyPhysicsOnSignal(a->getAssemblyPrimitive());
    }
    a->getSimJob()->useCount++;
}


void SendPhysics::onMovingAssemblyRootRemoving(Assembly* a)
{
    WriteValidator writeValidator(concurrencyValidator);

    //	AYAASSERT_VERY_FAST(Mechanism::isMovingAssemblyRoot(a));					// todo - get rid of this assert by
    // refactoring primitive anchoring
    SimJob* job = a->getSimJob();
    job->useCount--;

    if (job->useCount == 0)
    {
        assemblyPhysicsOffSignal(a->getAssemblyPrimitive());

        destroySimJob(job);
        delete job;
    }
}


} // namespace Aya