#ifndef AYA_STUDIO

#undef min
#undef max

#include "format_string.hpp"
#include "AyaFormat.hpp"
#include "Debug.hpp"
#include "boost.hpp"
#include "Utility/StandardOut.hpp"
#include "Utility/FileSystem.hpp"
#include "Utility/Guid.hpp"
#include "Utility/Http.hpp"
#include "Utility/Statistics.hpp"

#include "debugAssert.hpp"
#include <direct.h>

#include "atltime.h"
#include "atlfile.h"

#include "TaskScheduler.hpp"
#include "DumpErrorUploader.hpp"
#include "Log.hpp"
#include "FastLog.hpp"
#include <Windows.h>
#include <DbgHelp.h>
#include <string.h>
#include <atlbase.h>
#include <boost/format.hpp>

LOGGROUP(CrashReporterInit)

bool LogManager::logsEnabled = false; // to be honest we only really need crash dmps & the logs outputted are not working

MainLogManager* LogManager::mainLogManager = NULL;

Aya::mutex MainLogManager::fastLogChannelsLock;

static const ATL::CPath& DoGetPath()
{
    static ATL::CPath path(CString(Aya::FileSystem::getUserDirectory(true, Aya::DirAppData, "logs").native().c_str()));
    return path;
}


void InitPath()
{
    DoGetPath();
}

std::string GetAppVersion()
{
    CVersionInfo vi;
    FASTLOG1(FLog::CrashReporterInit, "Getting app version, module handle: %p", _AtlBaseModule.m_hInst);
    vi.Load(_AtlBaseModule.m_hInst);
    return vi.GetFileVersionAsString();
}

const ATL::CPath& LogManager::GetLogPath() const
{
    static boost::once_flag flag = BOOST_ONCE_INIT;
    boost::call_once(&InitPath, flag);
    return DoGetPath();
}

const std::string LogManager::GetLogPathString() const
{
    CStringA path = (LPCTSTR)GetLogPath();
    return std::string(path.GetString());
}

void MainLogManager::fastLogMessage(FLog::Channel id, const char* message)
{
    Aya::mutex::scoped_lock lock(fastLogChannelsLock);

    if (mainLogManager)
    {
        if (id >= mainLogManager->fastLogChannels.size())
            mainLogManager->fastLogChannels.resize(id + 1, NULL);

        if (mainLogManager->fastLogChannels[id] == NULL)
        {

            mainLogManager->fastLogChannels[id] = new Aya::Log(mainLogManager->getFastLogFileName(id).c_str(), "Log Channel");
        }

        mainLogManager->fastLogChannels[id]->writeEntry(Aya::Log::Information, message);
    }
}

std::string MainLogManager::getSessionId()
{
    std::string id = guid;
    return id;
}

std::string MainLogManager::getCrashEventName()
{
#ifdef WIN32
    FASTLOG(FLog::CrashReporterInit, "Getting crash event name");
    std::string path = GetLogPathString();

    std::string fileName = "log_";
    fileName += getSessionId();
    fileName += " ";

    fileName += GetAppVersion();
    fileName += crashEventExtention;

    path.append(fileName);

    return path;
#endif

    return "";
}

std::string MainLogManager::getLogFileName()
{
#ifdef WIN32
    std::string path = GetLogPathString();

    std::string fileName = "log_";
    fileName += getSessionId();
    fileName += ".txt";

    path.append(fileName);

    return path;
#endif
    return "";
}

std::string MainLogManager::getFastLogFileName(FLog::Channel channelId)
{
#ifdef WIN32
    std::string path = GetLogPathString();
    std::string filename = Aya::format("log_%s_%d.txt", getSessionId().c_str(), channelId);

    path.append(filename);

    return path;
#endif

    return "";
}


std::string MainLogManager::MakeLogFileName(const char* postfix)
{
#ifdef WIN32
    std::string path = GetLogPathString();

    std::string fileName = "log_";
    fileName += getSessionId();
    fileName += postfix;
    fileName += ".txt";

    path.append(fileName);

    return path;
#endif
    return "";
}

std::string ThreadLogManager::getLogFileName()
{
#ifdef WIN32
    std::string fileName = mainLogManager->getLogFileName();
    std::string id = Aya::format("_%s_%d", name.c_str(), threadID);
    fileName.insert(fileName.size() - 4, id);
    return fileName;
#endif
    return "";
}

Aya::Log* LogManager::getLog()
{
    if (!logsEnabled)
        return NULL;
    if (log == NULL)
    {
        log = new Aya::Log(getLogFileName().c_str(), name.c_str());
        // TODO: delete an old log that isn't in use
    }
    return log;
}


Aya::Log* MainLogManager::provideLog()
{
    if (GetCurrentThreadId() == threadID)
        return this->getLog();

    return ThreadLogManager::getCurrent()->getLog();
}


#include <process.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>

#define MAX_CONSOLE_LINES 250;

HANDLE g_hConsoleOut; // Handle to debug console



RobloxCrashReporter::RobloxCrashReporter(const char* outputPath, const char* appName, const char* crashExtention)
{
    controls.minidumpType = MiniDumpWithDataSegs;

    controls.minidumpType |= MiniDumpWithIndirectlyReferencedMemory;

    // null terminate just in case long paths & make safe
    strncpy(controls.pathToMinidump, outputPath, sizeof(controls.pathToMinidump) - 1);
    controls.pathToMinidump[sizeof(controls.pathToMinidump) - 1] = '\0';

    strncpy(controls.appName, appName, sizeof(controls.appName) - 1);
    controls.appName[sizeof(controls.appName) - 1] = '\0';

    strncpy(controls.appVersion, GetAppVersion().c_str(), sizeof(controls.appVersion) - 1);
    controls.appVersion[sizeof(controls.appVersion) - 1] = '\0';

    strncpy(controls.crashExtention, crashExtention, sizeof(controls.crashExtention) - 1);
    controls.crashExtention[sizeof(controls.crashExtention) - 1] = '\0';
}

bool RobloxCrashReporter::silent;

LONG RobloxCrashReporter::ProcessException(struct _EXCEPTION_POINTERS* info, bool noMsg)
{
    LogManager::ReportEvent(EVENTLOG_INFORMATION_TYPE, "StartProcessException...");

    LONG result = __super::ProcessException(info, noMsg);
    static bool showedMessage = silent;
    if (!showedMessage && !noMsg)
    {
        showedMessage = true;
        ::MessageBoxA(NULL, "An unexpected error occurred and " AYA_PROJECT_NAME " needs to quit.  We're sorry!", AYA_PROJECT_NAME " Crash", MB_OK);
    }

    LogManager::ReportEvent(EVENTLOG_INFORMATION_TYPE, "DoneProcessException");

    LogManager::ReportEvent(EVENTLOG_INFORMATION_TYPE, "Uploading .crashevent...");
    DumpErrorUploader::UploadCrashEventFile(info);
    LogManager::ReportEvent(EVENTLOG_INFORMATION_TYPE, "Done uploading .crashevent...");

    return result;
}

void RobloxCrashReporter::logEvent(const char* msg)
{
    LogManager::ReportEvent(EVENTLOG_INFORMATION_TYPE, msg);
}

void MainLogManager::WriteCrashDump()
{
    std::string appName = "log_";
    appName += getSessionId();
    crashReporter.reset(new RobloxCrashReporter(GetLogPathString().c_str(), appName.c_str(), crashExtention));
    crashReporter->Start();
};

bool MainLogManager::CreateFakeCrashDump()
{
    if (!crashReporter)
    {
        // start the service if not started.
        WriteCrashDump();
    }

    // First, write FastLog
    char dumpFilepath[_MAX_PATH];
    if (FAILED(crashReporter->GenerateDmpFileName(dumpFilepath, _MAX_PATH, true)))
    {
        return false;
    }

    FLog::WriteFastLogDump(dumpFilepath, 2000);

    if (FAILED(crashReporter->GenerateDmpFileName(dumpFilepath, _MAX_PATH)))
    {
        return false;
    }

    HANDLE hFile = CreateFileA(dumpFilepath, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    DWORD cb;
    WriteFile(hFile, "Fake", 5, &cb, NULL);

    CloseHandle(hFile);
    return true;
}

void MainLogManager::EnableImmediateCrashUpload(bool enabled)
{
    if (crashReporter)
    {
        crashReporter->EnableImmediateUpload(enabled);
    }
}

void MainLogManager::DisableHangReporting()
{
    if (crashReporter)
    {
        crashReporter->DisableHangReporting();
    }
}

void MainLogManager::NotifyFGThreadAlive()
{
    if (crashReporter)
    {
#if 0
        //  for debugging only:
        static int alivecount = 0;
        if(alivecount++ % 60 == 0)
        {
            CString eventMessage;
            eventMessage.Format("FGAlive %d", alivecount);
            LogManager::ReportEvent(EVENTLOG_INFORMATION_TYPE, eventMessage);
        }
#endif

        crashReporter->NotifyAlive();
    }
}


static void purecallHandler(void)
{
#ifdef _DEBUG
    _CrtDbgBreak();
#endif
    // Cause a crash
    AYACRASH();
}

MainLogManager::MainLogManager(LPCTSTR productName, const char* crashExtention, const char* crashEventExtention)
    : LogManager("Aya")
    , crashExtention(crashExtention)
    , crashEventExtention(crashEventExtention)
    , gameState(MainLogManager::GameState::UN_INITIALIZED)
{
    Aya::Guid::generateRBXGUID(guid);

    AYAASSERT(mainLogManager == NULL);
    mainLogManager = this;

    Aya::Log::setLogProvider(this);

    Aya::setAssertionHook(&MainLogManager::handleDebugAssert);
    Aya::setFailureHook(&MainLogManager::handleFailure);

    _set_purecall_handler(purecallHandler);

    FLog::SetExternalLogFunc(fastLogMessage);
}

MainLogManager* LogManager::getMainLogManager()
{
    return mainLogManager;
}


ThreadLogManager::ThreadLogManager()
    : LogManager(Aya::get_thread_name())
{
}

ThreadLogManager::~ThreadLogManager() {}

static float getThisYearTimeInMinutes(SYSTEMTIME time)
{
    return (time.wMonth * 43829.0639f) + (time.wDay * 1440) + (time.wHour * 60) + time.wMinute;
}

MainLogManager::~MainLogManager()
{
    Aya::mutex::scoped_lock lock(fastLogChannelsLock);

    FLog::SetExternalLogFunc(NULL);

    for (std::size_t i = 0; i < fastLogChannels.size(); i++)
        delete fastLogChannels[i];

    mainLogManager = NULL;
}


LogManager::~LogManager()
{
    if (log != NULL)
    {
        std::string logFile = log->logFile;
        delete log; // this will close the file so that we can move it
        log = NULL;
    }
}

inline HRESULT WINAPI RbxReportError(
    const CLSID& clsid, LPCSTR lpszDesc, DWORD dwHelpID, LPCSTR lpszHelpFile, const IID& iid = GUID_NULL, HRESULT hRes = 0)
{
    ATLASSERT(lpszDesc != NULL);
    if (lpszDesc == NULL)
        return E_POINTER;

    USES_CONVERSION_EX;
    CString strDesc(lpszDesc);
    CComBSTR desc = strDesc.AllocSysString(); // Convert CString to BSTR
    if (desc == NULL)
        return E_OUTOFMEMORY;

    CComBSTR helpFile = NULL;
    if (lpszHelpFile != NULL)
    {
        CString strHelpFile(lpszHelpFile);
        helpFile = strHelpFile.AllocSysString(); // Convert CString to BSTR
        if (helpFile == NULL)
            return E_OUTOFMEMORY;
    }

    return AtlSetErrorInfo(clsid, desc.Detach(), dwHelpID, helpFile.Detach(), iid, hRes, NULL);
}

inline HRESULT WINAPI RbxReportError(
    const CLSID& clsid, UINT nID, const IID& iid = GUID_NULL, HRESULT hRes = 0, HINSTANCE hInst = _AtlBaseModule.GetResourceInstance())
{
    return AtlSetErrorInfo(clsid, (LPCOLESTR)MAKEINTRESOURCE(nID), 0, NULL, iid, hRes, hInst);
}

inline HRESULT WINAPI RbxReportError(const CLSID& clsid, UINT nID, DWORD dwHelpID, LPCOLESTR lpszHelpFile, const IID& iid = GUID_NULL,
    HRESULT hRes = 0, HINSTANCE hInst = _AtlBaseModule.GetResourceInstance())
{
    return AtlSetErrorInfo(clsid, (LPCOLESTR)MAKEINTRESOURCE(nID), dwHelpID, lpszHelpFile, iid, hRes, hInst);
}

inline HRESULT WINAPI RbxReportError(const CLSID& clsid, LPCSTR lpszDesc, const IID& iid = GUID_NULL, HRESULT hRes = 0)
{
    return RbxReportError(clsid, lpszDesc, 0, NULL, iid, hRes);
}

inline HRESULT WINAPI RbxReportError(const CLSID& clsid, LPCOLESTR lpszDesc, const IID& iid = GUID_NULL, HRESULT hRes = 0)
{
    return AtlSetErrorInfo(clsid, lpszDesc, 0, NULL, iid, hRes, NULL);
}

inline HRESULT WINAPI RbxReportError(
    const CLSID& clsid, LPCOLESTR lpszDesc, DWORD dwHelpID, LPCOLESTR lpszHelpFile, const IID& iid = GUID_NULL, HRESULT hRes = 0)
{
    return AtlSetErrorInfo(clsid, lpszDesc, dwHelpID, lpszHelpFile, iid, hRes, NULL);
}

HRESULT LogManager::ReportCOMError(const CLSID& clsid, LPCOLESTR lpszDesc, HRESULT hRes)
{
    return RbxReportError(clsid, lpszDesc, GUID_NULL, hRes);
}

HRESULT LogManager::ReportCOMError(const CLSID& clsid, LPCSTR lpszDesc, HRESULT hRes)
{
    return RbxReportError(clsid, lpszDesc, GUID_NULL, hRes);
}

HRESULT LogManager::ReportCOMError(const CLSID& clsid, HRESULT hRes)
{
    std::string message = Aya::format("HRESULT 0x%X", hRes);
    LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, message.c_str());
    return RbxReportError(clsid, message.c_str(), GUID_NULL, hRes);
}

#ifdef _MFC_VER
HRESULT LogManager::ReportCOMError(const CLSID& clsid, CException* exception)
{
    CString fullError;
    HRESULT hr = COleException::Process(exception);
    CString sError;
    if (exception->GetErrorMessage(sError.GetBuffer(1024), 1023))
    {
        sError.ReleaseBuffer();
        fullError.Format("%s (0x%X)", sError, hr);
    }
    else
        fullError.Format("Error 0x%X", hr);

    LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, fullError);
    return RbxReportError(clsid, fullError, GUID_NULL, hr);
}
#endif

bool MainLogManager::handleG3DDebugAssert(
    const char* _expression, const std::string& message, const char* filename, int lineNumber, bool useGuiPrompt)
{
    return handleDebugAssert(_expression, filename, lineNumber);
}

bool MainLogManager::handleDebugAssert(const char* expression, const char* filename, int lineNumber)
{
#ifdef _DEBUG
    LogManager::ReportEvent(EVENTLOG_WARNING_TYPE,
        std::string("Assertion failed: " + std::string(expression) + "\n" + std::string(filename) + "(" + std::to_string(lineNumber) + ")").c_str());
    AYACRASH();
    return true;
#else
    return false;
#endif
}


bool MainLogManager::handleG3DFailure(const char* _expression, const std::string& message, const char* filename, int lineNumber, bool useGuiPrompt)
{
    return handleFailure(_expression, filename, lineNumber);
}
bool MainLogManager::handleFailure(const char* expression, const char* filename, int lineNumber)
{
#ifdef _DEBUG
    _CrtDbgBreak();
#endif
    // Cause a crash
    AYACRASH();
    return false;
}

HRESULT LogManager::ReportExceptionAsCOMError(const CLSID& clsid, std::exception const& exp)
{
    return ReportCOMError(clsid, exp.what());
}

void LogManager::ReportException(std::exception const& exp)
{
    Aya::StandardOut::singleton()->print(Aya::MESSAGE_ERROR, exp);
}

void LogManager::ReportLastError(LPCSTR message)
{
    DWORD error = GetLastError();
    Aya::StandardOut::singleton()->printf(Aya::MESSAGE_ERROR, "%s, GetLastError=%d", message, error);
}

void LogManager::ReportEvent(WORD type, LPCSTR message)
{
    switch (type)
    {
    case EVENTLOG_SUCCESS:
        Aya::StandardOut::singleton()->printf(Aya::MESSAGE_INFO, "%s", message);
        break;
    case EVENTLOG_ERROR_TYPE:
        Aya::StandardOut::singleton()->printf(Aya::MESSAGE_ERROR, "%s", message);
        break;
    case EVENTLOG_INFORMATION_TYPE:
        Aya::StandardOut::singleton()->printf(Aya::MESSAGE_INFO, "%s", message);
        break;
    case EVENTLOG_AUDIT_SUCCESS:
        Aya::StandardOut::singleton()->printf(Aya::MESSAGE_INFO, "%s", message);
        break;
    case EVENTLOG_AUDIT_FAILURE:
        Aya::StandardOut::singleton()->printf(Aya::MESSAGE_ERROR, "%s", message);
        break;
    }
#ifdef _DEBUG
    switch (type)
    {
    case EVENTLOG_SUCCESS:
        ATLTRACE("EVENTLOG_SUCCESS %s\n", message);
        break;
    case EVENTLOG_ERROR_TYPE:
        ATLTRACE("EVENTLOG_ERROR_TYPE %s\n", message);
        break;
    case EVENTLOG_INFORMATION_TYPE:
        ATLTRACE("EVENTLOG_INFORMATION_TYPE %s\n", message);
        break;
    case EVENTLOG_AUDIT_SUCCESS:
        ATLTRACE("EVENTLOG_AUDIT_SUCCESS %s\n", message);
        break;
    case EVENTLOG_AUDIT_FAILURE:
        ATLTRACE("EVENTLOG_AUDIT_FAILURE %s\n", message);
        break;
    }
#endif
}

void LogManager::ReportEvent(WORD type, LPCSTR message, LPCSTR fileName, int lineNumber)
{
    // CString m;
    // m.Format(convert_s2w("%s\n%s(%d)"), message, fileName, lineNumber);
    // LogManager::ReportEvent(type, m);
}

#ifdef _MFC_VER
void LogManager::ReportEvent(WORD type, HRESULT hr, LPCSTR fileName, int lineNumber)
{
    COleException e;
    e.m_sc = hr;
    TCHAR s[1024];
    e.GetErrorMessage(s, 1024);

    CString m;
    m.Format("HRESULT = %d: %s\n%s(%d)", hr, s, fileName, lineNumber);
    LogManager::ReportEvent(type, m);
}

#endif

namespace log_detail
{
boost::once_flag once_init = BOOST_ONCE_INIT;
static boost::thread_specific_ptr<ThreadLogManager>* ts;
void init(void)
{
    static boost::thread_specific_ptr<ThreadLogManager> value;
    ts = &value;
}
} // namespace log_detail

ThreadLogManager* ThreadLogManager::getCurrent()
{
    boost::call_once(log_detail::init, log_detail::once_init);
    ThreadLogManager* logManager = log_detail::ts->get();
    if (!logManager)
    {
        logManager = new ThreadLogManager();
        log_detail::ts->reset(logManager);
    }
    return logManager;
}

#endif