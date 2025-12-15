#pragma once

#include <string>
#include <fstream>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#include <locale>
#include <codecvt>
#include <combaseapi.h>
#else
#include <cstdarg>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

std::string vformat(const char* fmt, va_list argPtr);
std::wstring vformat(const wchar_t* fmt, va_list argPtr);

std::string format_string(const char* fmt, ...);
std::wstring format_string(const wchar_t* fmt, ...);

std::string convert_w2s(const std::wstring& str);
std::wstring convert_s2w(const std::string& str);

std::vector<std::string> splitOn(const std::string& str, const char& delimeter, const bool trimEmpty = true);

std::vector<std::wstring> splitOn(const std::wstring& str, const wchar_t& delimeter, const bool trimEmpty = true);

#ifdef UNICODE
#define CVTW2S(value) convert_w2s(value)
#else
#define CVTW2S(value) (value)
#endif

#ifdef UNICODE
#define CVTS2W(value) convert_s2w(value)
#else
#define CVTS2W(value) (value)
#endif

#ifdef UNICODE
#define MAKE_STRING(value) std::wstring(L##value)
#else
#define MAKE_STRING(value) std::string(value)
#endif

template<class CHARTYPE>
class simple_logger
{
    typedef std::basic_string<CHARTYPE, std::char_traits<CHARTYPE>, std::allocator<CHARTYPE>> STRING;

private:
    void write_logentry_raw(const char* entry)
    {
#ifdef _WIN32
        SYSTEMTIME sysTime = {};
        GetSystemTime(&sysTime);
        char timeBuf[128];
        sprintf_s(timeBuf, 128, "%02d-%02d-%d %02d:%02d:%02d.%d ", sysTime.wMonth, sysTime.wDay, sysTime.wYear, sysTime.wHour, sysTime.wMinute,
            sysTime.wSecond, sysTime.wMilliseconds);
        log << timeBuf << entry << "\n";
        log.flush();
#endif
    }

    STRING logFileName;
    std::ofstream log;

public:
    simple_logger()
        : logFileName(get_temp_filename(MAKE_STRING("log").c_str()))
        , log(logFileName.c_str())
    {
    }

    simple_logger(const CHARTYPE* fileName)
        : logFileName(fileName)
        , log(logFileName.c_str())
    {
    }

    static STRING get_tmp_path()
    {
#ifdef _WIN32
        CHARTYPE szPath[_MAX_DIR];
        if (!GetTempPathA(_MAX_DIR, szPath))
        {
            throw std::runtime_error("GetTempPath in simple_logger::getTempFileName failed");
        }
        return STRING(szPath);
#else
        return STRING(MAKE_STRING("/tmp/"));
#endif
    }

    static STRING get_temp_filename(const CHARTYPE* ext)
    {
#ifdef _WIN32
        GUID g;
        ::CoCreateGuid(&g);
        char szFilePath[MAX_PATH];
        sprintf_s(szFilePath, MAX_PATH, "%SRBX-%08X.%S", get_tmp_path().c_str(), g.Data1, ext);

        return STRING(CVTS2W(szFilePath));
#else
        // Unix does not have GUIDs like Windows, so use a unique filename creation mechanism
        std::string tmpFileName = "/tmp/logXXXXXX"; // Template for mkstemp
        int tmpFd = mkstemp(&tmpFileName[0]);       // Creates a unique temporary file name and opens it
        if (tmpFd < 0)
        {
            throw std::runtime_error("Unable to create temp file in simple_logger::get_temp_filename");
        }
        close(tmpFd); // Close the file descriptor, we just want the name

        // Append the extension
        tmpFileName += ".";
        tmpFileName += ext;
        return STRING(tmpFileName.begin(), tmpFileName.end()); // Convert to the desired character type
#endif
    }

    void write_logentry(const char* format, ...)
    {
        va_list argList;
        va_start(argList, format); // Now va_start should be recognized
        std::string result = vformat(format, argList);
        write_logentry_raw(result.c_str());
        va_end(argList); // And va_end should be recognized as well
    }

    STRING& log_filename()
    {
        return logFileName;
    }
};