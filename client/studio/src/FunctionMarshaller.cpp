


#include "FunctionMarshaller.hpp"

// 3rd Party Headers
#include "boost/function.hpp"
#include "boost/thread/recursive_mutex.hpp"

// Roblox Headers
#include "Utility/StandardOut.hpp"

#include "boost.hpp"
#include "CEvent.hpp"


// Roblox Studio Headers
#include "Roblox.hpp"

// TODO Clean up this file

using namespace Aya;

FunctionMarshaller::FunctionMarshaller(DWORD threadID)
    : refCount(0)
{
    this->threadID = threadID;
}

FunctionMarshaller::~FunctionMarshaller()
{
    AYAASSERT(threadID == GetCurrentThreadId());

#ifdef _DEBUG
    {
        boost::recursive_mutex::scoped_lock lock(staticData().windowsCriticalSection);
        AYAASSERT(refCount == 0);
        // Nobody is using this window
        AYAASSERT(staticData().windows.find(threadID) == staticData().windows.end());
    }
#endif
}


FunctionMarshaller* FunctionMarshaller::GetWindow()
{
    // Share a common FunctionMarshaller in a given Thread
    boost::recursive_mutex::scoped_lock lock(staticData().windowsCriticalSection);
    DWORD threadID = GetCurrentThreadId();
    std::map<DWORD, FunctionMarshaller*>::iterator find = staticData().windows.find(threadID);
    if (find != staticData().windows.end())
    {
        // We already created a window, so use it again
        find->second->refCount++;
        return find->second;
    }
    else
    {
        // Create a new window
        FunctionMarshaller* window = new FunctionMarshaller(threadID);
        staticData().windows[threadID] = window;
        window->refCount++;
        return window;
    }
}

void FunctionMarshaller::ReleaseWindow(FunctionMarshaller* window)
{
    boost::recursive_mutex::scoped_lock lock(staticData().windowsCriticalSection);
    window->refCount--;
    if (window->refCount == 0)
    {
        // Nobody is using this window
        staticData().windows.erase(window->threadID);
        delete window;
    }
}
void FunctionMarshaller::handleAppEvent(void* pClosure)
{
    FunctionMarshaller::Closure* closure = (FunctionMarshaller::Closure*)pClosure;
    Aya::CEvent* pWaitEvent = closure->waitEvent;
    try
    {
        boost::function<void()>* pF = closure->f;
        (*pF)();

        delete pF;
        delete closure;
    }
    catch (std::exception& e)
    {
        StandardOut::singleton()->print(Aya::MESSAGE_ERROR, e);
        closure->errorMessage = e.what();
    }

    // If a task is waiting on an event, set it
    if (pWaitEvent)
    {
        pWaitEvent->Set();
    }
}

void FunctionMarshaller::freeAppEvent(void* pClosure)
{
    FunctionMarshaller::Closure* closure = (FunctionMarshaller::Closure*)pClosure;
    boost::function<void()>* pF = closure->f;
    delete pF;
    delete closure;
}

void FunctionMarshaller::Execute(boost::function<void()> job, CEvent* waitEvent)
{
    if (threadID == GetCurrentThreadId())
        job();
    else
    {
        Closure* pClosure = new Closure;
        pClosure->f = new boost::function<void()>(job);
        pClosure->waitEvent = waitEvent;
        Roblox::sendAppEvent(pClosure);
    }
}

void FunctionMarshaller::Submit(boost::function<void()> job)
{
    Closure* pClosure = new Closure;
    pClosure->f = new boost::function<void()>(job);
    pClosure->waitEvent = NULL;
    Roblox::postAppEvent(pClosure);
}

FunctionMarshaller::StaticData::~StaticData() {}
