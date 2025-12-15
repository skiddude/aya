// This prevent inclusion of winsock.h in Windows.h, which prevents windows redifinition errors
// Look at winsock2.h for details, winsock2.h is #included from boost.hpp & other places.
#ifdef _WIN32
#define _WINSOCKAPI_
#endif

#if defined(__linux) || defined(__APPLE__)
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#endif

#include "time.hpp"
#include "Debug.hpp"
#include "atomic.hpp"
#include <stdexcept>
#include "FastLog.hpp"

#include <algorithm>

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

#ifdef __ANDROID__
#include <unistd.h>
#endif

#if defined(_WIN32) && !defined(AYA_PLATFORM_DURANGO)
#include <windows.h>
#include <timeapi.h>

#endif

FASTINTVARIABLE(SpeedTestPeriodMillis, 1000)
FASTINTVARIABLE(MaxSpeedDeltaMillis, 300)
FASTINTVARIABLE(SpeedCountCap, 5)

namespace Aya
{

long long Time::getTickCount()
{
#if defined(_WIN32)
    LARGE_INTEGER ticks;
    int rval = QueryPerformanceCounter(&ticks);
    return ticks.QuadPart;
#elif defined(__APPLE__)
    uint64_t ticks = mach_absolute_time();
    return ticks;
#elif defined(__ANDROID__) || defined(__linux)
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    return now.tv_sec * 1e9 + now.tv_nsec;
#endif
}

#if defined(_WIN32)
static double getPerformanceFrequency()
{
    static double frequency = 0.0;
    if (frequency == 0.0)
    {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        frequency = static_cast<double>(freq.QuadPart);
    }
    return frequency;
}
#endif

long long Time::getStart()
{
    // not worried about potential multi-threaded double-init.
    // assumptions: underlying type is long long.
    static const long long start = getTickCount();
    return start;
}


Time::SampleMethod Time::preciseOverride = Time::Precise;

template<>
Time Time::now<Time::Precise>()
{
    Time result;
#if defined(_WIN32)
    result.sec = static_cast<double>(getTickCount() - getStart()) / getPerformanceFrequency();
#else
    result.sec = (getTickCount() - getStart()) * 1e-9;
#endif
    return result;
}


template<>
Time Time::now<Time::Multimedia>()
{
#if defined(_WIN32) && !defined(AYA_PLATFORM_DURANGO)
    return Time(timeGetTime() / 1000.0);
#else
    // TODO: Is this fast enough on Mac?
    return now<Time::Precise>();
#endif
}


template<>
Time Time::now<Time::Fast>()
{
    return now<Time::Precise>();
}

Time Time::nowFast()
{
    return now<Time::Fast>();
}

double Time::nowFastSec()
{
    return Time::nowFast().timestampSeconds();
}

template<>
Time Time::now<Time::Benchmark>()
{
    if (preciseOverride <= Benchmark)
        return now<Precise>();
    else
        return now<Fast>();
}

Time Time::now(SampleMethod sampleMethod)
{
    switch (sampleMethod)
    {
    default:
    case Fast:
        return now<Fast>();

    case Precise:
        return now<Precise>();

    case Benchmark:
        return now<Benchmark>();

    case Multimedia:
        return now<Multimedia>();
    }
}

Time::Interval operator-(const Time& t1, const Time& t0)
{
    const double seconds = t1.sec - t0.sec;
    return Time::Interval(seconds);
}

void Time::Interval::sleep()
{
#ifdef _WIN32
    // Translate to milliseconds
    Sleep((int)(sec * 1e3));
#else
    // Translate to microseconds
    usleep((int)(sec * 1e6));
#endif
}

} // namespace Aya