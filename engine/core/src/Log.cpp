#include "Log.hpp"
#include "AyaFormat.hpp"
#ifdef _WIN32
#include <Windows.h>
#define sprints sprintf_s
#define wcstombs wcstombs_s
#else
#endif

#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/gregorian/greg_year.hpp"

using namespace Aya;

#include "time.hpp"

ILogProvider* Log::provider = NULL;

Log::Severity Log::aggregateWorstSeverity = Log::Information;

void Log::timeStamp(std::ofstream& stream, bool includeDate)
{
    /*
    boost::posix_time::ptime stime(boost::posix_time::second_clock::local_time());
    char s[256];

    if (includeDate)
    {
            boost::gregorian::date date(stime.date());
            snprintf(s, ARRAYSIZE(s), "%02u.%02u.%u ", date.day().as_number(), date.month().as_number(), (unsigned short)date.year());
            stream << s;
    }
    boost::posix_time::time_duration dur(stime.time_of_day());
    snprintf(s, ARRAYSIZE(s), "%02u:%02u:%02u.%03u (%03.07f)", (unsigned int)dur.hours(), (unsigned int)dur.minutes(), (unsigned int)dur.seconds(),
    (unsigned short)dur.total_milliseconds(), Time::nowFastSec()); stream << s; stream.flush();

    // d9mz - why is it still trying to do log shit after i disabled it?
    */
}

Log::Log(const char* logFile, const char* name)
    : stream(logFile)
    , logFile(logFile)
    , worstSeverity(Information)
    , name(name)
{
    Log::timeStamp(stream, true);
    stream << "Log \"" << name << "\"\n";
    stream.flush();
}

Log::~Log(void)
{
    Log::timeStamp(stream, true);
    stream << "End Log\n";
}

void Log::setLogProvider(ILogProvider* provider)
{
    Log::provider = provider;
}

void Log::writeEntry(Severity severity, const wchar_t* message)
{
    // Convert to a char*
    size_t origsize = wcslen(message) + 1;
    const size_t newsize = origsize + 100;
    size_t convertedChars = 0;
    char* nstring = new char[newsize];
#ifdef _WIN32
    wcstombs_s(&convertedChars, nstring, origsize, message, _TRUNCATE);
#else
    convertedChars = wcstombs(nstring, message, origsize);
#endif
    if (convertedChars >= origsize - 1)
        nstring[origsize - 1] = '\0';
    writeEntry(severity, nstring);
    delete[] nstring;
}


void Log::writeEntry(Severity severity, const char* message)
{
    /*
    static const char* error = " Error:   ";
    static const char* warning = " Warning: ";
    static const char* information = "          ";
    Log::timeStamp(stream, false);
    switch (severity)
    {
    case Log::Error:
            stream << error;
            break;
    case Log::Warning:
            stream << warning;
            break;
    case Log::Information:
            stream << information;
            break;
    }
    stream << message;
    stream << '\n';
    stream.flush();
    */
}

std::string Log::formatMem(uint64_t bytes)
{
    char* suffix[] = {"B", "KB", "MB", "GB", "TB"};
    char length = sizeof(suffix) / sizeof(suffix[0]);

    int i = 0;
    double dblBytes = bytes;

    if (bytes > 1024)
    {
        for (i = 0; (bytes / 1024) > 0 && i < length - 1; i++, bytes /= 1024)
            dblBytes = bytes / 1024.0;
    }

    static char output[200];
    sprintf(output, "%.02lf %s", dblBytes, suffix[i]);

    return std::string(output);
}

std::string Log::formatTime(double time)
{
    char buffer[64];
    if (time == 0.0)
        snprintf(buffer, ARRAYSIZE(buffer), "0s");
    if (time < 0.0)
        snprintf(buffer, ARRAYSIZE(buffer), "%.3gs", time);
    else if (time >= 0.1)
        snprintf(buffer, ARRAYSIZE(buffer), "%.3gs", time);
    else
        snprintf(buffer, ARRAYSIZE(buffer), "%.3gms", time * 1000.0);
    return buffer;
}



void Log::timeStamp(bool includeDate)
{
    Log::timeStamp(Log::currentStream(), includeDate);
}

LOGVARIABLE(Crash, 1)
LOGVARIABLE(HangDetection, 0)
LOGVARIABLE(ContentProviderCleanup, 0)
LOGVARIABLE(ISteppedLifetime, 0)
LOGVARIABLE(MutexLifetime, 0)
LOGVARIABLE(TaskScheduler, 0)
LOGVARIABLE(TaskSchedulerInit, 0)
LOGVARIABLE(TaskSchedulerRun, 0)
LOGVARIABLE(TaskSchedulerFindJob, 0)
LOGVARIABLE(TaskSchedulerSteps, 0)
LOGVARIABLE(Asserts, 0)
LOGVARIABLE(FWLifetime, 0)
LOGVARIABLE(FWUpdate, 0)
LOGVARIABLE(KernelStats, 0)

void initBaseLog() {}
