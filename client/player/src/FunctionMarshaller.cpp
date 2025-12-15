// THIS FILE PURPOSE BUILT FOR SDL AND TRUCKS

#include "FunctionMarshaller.hpp"

#undef min
#undef max

#include "Utility/StandardOut.hpp"

#include "boost.hpp"

#include <QApplication>
#include <QWidget>

// ----------------------------------------------------------------------------

using namespace Aya;

const QEvent::Type Aya::TYPE_FUNCTION_MARSHALLER = static_cast<QEvent::Type>(QEvent::registerEventType());

FunctionMarshallerEvent::FunctionMarshallerEvent(void* closure)
    : QEvent(TYPE_FUNCTION_MARSHALLER)
{
    this->closure = closure;
}

FunctionMarshaller::FunctionMarshaller(DWORD threadID)
    : refCount(0)
{
    this->threadID = threadID;
}

FunctionMarshaller::~FunctionMarshaller()
{
    boost::function<void()>* f;
    while (asyncCalls.pop_if_present(f))
        delete f;

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
        //		window->DestroyWindow();
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
    catch (Aya::base_exception& e)
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

extern QApplication* qtAppPtr;

void FunctionMarshaller::Execute(boost::function<void()> job, CEvent* waitEvent)
{
    if (threadID == GetCurrentThreadId())
    {
        job();
    }
    else
    {
        Closure* pClosure = new Closure;
        pClosure->f = new boost::function<void()>(job);
        pClosure->waitEvent = waitEvent;

        /*SDL_Event event;
        event.type = SDL_EVENT_USER;
        event.user.code = 0xc10554e;
        event.user.data1 = pClosure;
        SDL_PushEvent(&event);*/

        const QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
        qtAppPtr->postEvent(topLevelWidgets.at(0), new FunctionMarshallerEvent(pClosure));
    }
}

void FunctionMarshaller::Submit(boost::function<void()> job)
{
    Closure* pClosure = new Closure;
    pClosure->f = new boost::function<void()>(job);
    pClosure->waitEvent = nullptr;

    /*SDL_Event event;
    event.type = SDL_EVENT_USER;
    event.user.code = 0xc10554e;
    event.user.data1 = pClosure;
    SDL_PushEvent(&event);*/

    const QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
    qtAppPtr->postEvent(topLevelWidgets.at(0), new FunctionMarshallerEvent(pClosure));
}

void FunctionMarshaller::ProcessMessages() {}

/*
 void FunctionMarshaller::OnFinalMessage(HWND hWnd)
{
        delete this;
}
*/

FunctionMarshaller::StaticData::~StaticData()
{
    //	for (std::map<DWORD, FunctionMarshaller*>::iterator iter = windows.begin(); iter != windows.end(); ++iter)
    //		iter->second->DestroyWindow();
}