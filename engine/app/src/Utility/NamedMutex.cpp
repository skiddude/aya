#ifdef _WIN32


#include "Utility/NamedMutex.hpp"
#include "FastLog.hpp"
#include "AyaFormat.hpp"
#include "Debug.hpp"

#include <boost/system/error_code.hpp>
#include <sstream>

#include <Windows.h>

using namespace Aya;

DYNAMIC_LOGVARIABLE(NamedMutex, 0)

ScopedNamedMutex::ScopedNamedMutex(const char* name)
{
    if (DFLog::NamedMutex)
    {
        std::stringstream ss;
        ss << reinterpret_cast<void*>(this) << "): " << name;
        FASTLOGS(DFLog::NamedMutex, "ScopedNamedMutex(%s", ss.str().c_str());
    }

    if (NULL == (hMutex = CreateMutexA(NULL, false, name)))
    {
        boost::system::error_code ec(GetLastError(), boost::system::system_category());
        throw Aya::runtime_error("ScopedNamedMutex: CreateMutex failed: %s", ec.message().c_str());
    }
    if (WAIT_FAILED == WaitForSingleObject(hMutex, INFINITE))
    {
        boost::system::error_code ec(GetLastError(), boost::system::system_category());
        throw Aya::runtime_error("ScopedNamedMutex: WaitForSingleObject failed: %s", ec.message().c_str());
    }
}

ScopedNamedMutex::~ScopedNamedMutex()
{
    if (hMutex && !ReleaseMutex(hMutex) && DFLog::NamedMutex)
    {
        boost::system::error_code ec(GetLastError(), boost::system::system_category());
        std::stringstream ss;
        ss << reinterpret_cast<void*>(this) << "): " << ec.message();
        FASTLOGS(DFLog::NamedMutex, "~ScopedNamedMutex(%s", ss.str().c_str());
    }
    else
    {
        FASTLOG1(DFLog::NamedMutex, "~ScopedNamedMutex(%p)", this);
    }
}
#endif