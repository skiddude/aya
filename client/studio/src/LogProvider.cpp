/*
 *  LogProvider.cpp
 *  RobloxStudio
 *
 */

// Roblox Headers
#include "Utility/FileSystem.hpp"

#include "Utility/Guid.hpp"

#include "boost.hpp"

// Roblox Studio Headers
#include "LogProvider.hpp"

#include <locale>
#include <codecvt>
#include <string>
#include <boost/filesystem.hpp>
// #endif


Aya::mutex LogProvider::fastLogChannelsLock;
LogProvider* LogProvider::mainLogManager;

LogProvider::LogProvider()
{
    logDir = "logs/";
    Aya::Guid::generateRBXGUID(logGuid);
    mainLogManager = this;
    FLog::SetExternalLogFunc(LogProvider::FastLogMessage);
}

void LogProvider::FastLogMessage(FLog::Channel id, const char* message)
{

    Aya::mutex::scoped_lock lock(fastLogChannelsLock);

    if (mainLogManager)
    {
        if (id >= mainLogManager->fastLogChannels.size())
            mainLogManager->fastLogChannels.resize(id + 1, NULL);

        if (mainLogManager->fastLogChannels[id] == NULL)
        {
#ifdef WIN32
            char temp[20];
            snprintf(temp, 19, "log_%s_%u.txt", mainLogManager->logGuid.substr(3, 6).c_str(), id);
            temp[19] = 0;

            boost::filesystem::path logFile = mainLogManager->logDir / temp;

            std::wstring wideLogFilePath = logFile.wstring();

            std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
            std::string narrowLogFilePath = converter.to_bytes(wideLogFilePath);

            mainLogManager->fastLogChannels[id] = new Aya::Log(narrowLogFilePath.c_str(), "Log Channel");
#endif
        }

        mainLogManager->fastLogChannels[id]->writeEntry(Aya::Log::Information, message);
    }
}

LogProvider::~LogProvider() {}

Aya::Log* LogProvider::provideLog()
{
    Aya::Log* result = log.get();
    if (!result)
    {
        std::string name = Aya::get_thread_name();
        boost::filesystem::path logFile = logDir / ("log_" + logGuid.substr(3, 6) + ".txt");

        std::wstring wideLogFilePath = logFile.wstring();

        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::string narrowLogFilePath = converter.to_bytes(wideLogFilePath);

        result = new Aya::Log(narrowLogFilePath.c_str(), name.c_str());
        log.reset(result);
    }
    return result;
}