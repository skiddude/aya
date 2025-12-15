// This file is used for two targets:
// 1. Used in Mac Roblox Player app, this is the place where actual file resides
// 2. Used in Qt version of Roblox Studio as a soft link from above

#pragma once

#include <map>
#include "threadsafe.hpp"

#include "CEvent.hpp"
#include <QEvent>
namespace Aya
{

extern const QEvent::Type TYPE_FUNCTION_MARSHALLER;
class FunctionMarshallerEvent : public QEvent
{
    void* closure;

public:
    explicit FunctionMarshallerEvent(void* closure);
    void* getClosure()
    {
        return closure;
    }
};

// A very handy class for marshalling a function across Windows threads (sync and async)
class FunctionMarshaller
{
public:
private:
    struct StaticData
    {
        std::map<DWORD, FunctionMarshaller*> windows;
        boost::recursive_mutex windowsCriticalSection; // TODO: Would non-recursive be safe here?
        ~StaticData();
    };
    SAFE_STATIC(StaticData, staticData)

    Aya::safe_queue<boost::function<void()>*> asyncCalls;
    int refCount;
    DWORD threadID;
    FunctionMarshaller(DWORD threadID);


    ~FunctionMarshaller();

public:
    // TODO: Wrap with a reference counter and then remove ~StaticData() cleanup code and remove ReleaseWindow()
    static FunctionMarshaller* GetWindow();
    static void ReleaseWindow(FunctionMarshaller* window);

    static void handleAppEvent(void* pClosure);
    static void freeAppEvent(void* pClosure);

    struct Closure
    {
        boost::function<void()>* f;
        std::string errorMessage;
        Aya::CEvent* waitEvent;
    };

    void Execute(boost::function<void()> job, CEvent* waitEvent);
    void Submit(boost::function<void()> job);

    // Call this only from the Window's thread
    void ProcessMessages();

    // virtual void OnFinalMessage(HWND hWnd);

private:
    // LRESULT OnEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    // LRESULT OnAsyncEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};


} // namespace Aya