#include "Utility/MemoryStats.hpp"

#if defined(_WIN32) // should only be used with Microsoft platforms
#include <Windows.h>

#if !defined(AYA_PLATFORM_DURANGO)
#include <psapi.h>
#endif
#pragma comment(lib, "psapi.lib")

using namespace Aya;
using namespace Aya::MemoryStats;

namespace Aya
{
namespace MemoryStats
{
#if !defined(AYA_PLATFORM_DURANGO)
MEMORYSTATUSEX globalMemoryStatusEx()
{
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    return statex;
}
#endif

DWORDLONG usedMemoryBytes()
{
#if !defined(AYA_PLATFORM_DURANGO)
    PROCESS_MEMORY_COUNTERS pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
    return pmc.WorkingSetSize;
#elif defined(AYA_PLATFORM_WIN_PHONE)
    return Windows::System::MemoryManager::AppMemoryUsage;
#else
    // TODO:WinRT Windows Surface App
    return 0;
#endif
}

DWORDLONG freeMemoryBytes()
{
#if !defined(AYA_PLATFORM_DURANGO)
    MEMORYSTATUSEX statex = globalMemoryStatusEx();
    return statex.ullAvailPhys;
#elif defined(AYA_PLATFORM_WIN_PHONE)
    return Windows::System::MemoryManager::AppMemoryUsageLimit - Windows::System::MemoryManager::AppMemoryUsage;
#else
    // TODO:WinRT Windows Store App
    return 0;
#endif
}

DWORDLONG totalMemoryBytes()
{
#if defined(AYA_PLATFORM_DURANGO)
    _TITLEMEMORYSTATUS status;
    status.dwLength = sizeof(TITLEMEMORYSTATUS);
    TitleMemoryStatus(&status);
    return status.ullTotalMem;
#elif defined(AYA_PLATFORM_WIN_PHONE)
    return Windows::System::MemoryManager::AppMemoryUsageLimit;
#else
    MEMORYSTATUSEX statex = globalMemoryStatusEx();
    return statex.ullTotalPhys;
#endif
}
} // namespace MemoryStats
} // namespace Aya
#endif // defined(_WIN32)
