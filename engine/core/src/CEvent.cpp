
#include "Debug.hpp"
#include "CEvent.hpp"

#ifndef _WIN32
const int Aya::CEvent::cWAIT_OBJECT_0;
const int Aya::CEvent::cWAIT_TIMEOUT;
const int Aya::CEvent::cINFINITE;
#endif

#ifdef _WIN32
#include <Windows.h>
#endif


void Aya::CEvent::Wait()
{
    WaitForSingleObject(*this, cINFINITE);
}

bool Aya::CEvent::Wait(int milliseconds)
{
    return WaitForSingleObject(*this, milliseconds) == cWAIT_OBJECT_0;
}

#ifdef AYA_CEVENT_BOOST

Aya::CEvent::~CEvent() throw() {}

Aya::CEvent::CEvent(bool bManualReset)
    : isSet(false)
    , manualReset(bManualReset)
{
}

void Aya::CEvent::Set() throw()
{
    boost::unique_lock<boost::mutex> lock(mut);
    if (manualReset)
    {
        isSet = true;
        cond.notify_all();
    }
    else
    {
        isSet = true;
        cond.notify_one();
    }
}

int Aya::CEvent::WaitForSingleObject(CEvent& event, int milliseconds)
{
    if (milliseconds == cINFINITE)
    {
        boost::unique_lock<boost::mutex> lock(event.mut);
        if (!event.isSet)
            event.cond.wait(lock);
        if (!event.manualReset)
            event.isSet = false;
        return cWAIT_OBJECT_0;
    }
    else
    {
        boost::system_time const time = boost::get_system_time() + boost::posix_time::milliseconds(milliseconds);
        boost::unique_lock<boost::mutex> lock(event.mut);
        bool result = event.isSet || event.cond.timed_wait(lock, time);
        if (result && !event.manualReset)
            event.isSet = false;
        return result ? cWAIT_OBJECT_0 : cWAIT_TIMEOUT;
    }
}


#else

static void WINAPI RbxThrowLastWin32()
{
    DWORD dwError = ::GetLastError();
    HRESULT hr = HRESULT_FROM_WIN32(dwError);
    throw Aya::runtime_error("HRESULT = 0x%.8X", hr);
}


Aya::CEvent::~CEvent() throw()
{
    if (m_h != NULL)
    {
        BOOL result = ::CloseHandle(m_h);
        AYAASSERT(result != 0);
        m_h = NULL;
    }
}


Aya::CEvent::CEvent(bool bManualReset)
    : m_h(NULL)
{
    m_h = ::CreateEvent(NULL, bManualReset ? TRUE : FALSE, FALSE, NULL);
    if (!m_h)
        RbxThrowLastWin32();
}



void Aya::CEvent::Set() throw()
{
    if (m_h && !::SetEvent(m_h))
        RbxThrowLastWin32();
}

int Aya::CEvent::WaitForSingleObject(CEvent& event, int milliseconds)
{
    return ::WaitForSingleObject(event.m_h, milliseconds);
}

#endif
