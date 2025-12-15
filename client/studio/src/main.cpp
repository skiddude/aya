

// Precompiled Header


// Qt Headers
#include <QApplication>
#include <QFile>
#include <QStringList>
#include <QMap>
#include <QApplication>

// Roblox Headers
#include "FastLog.hpp"
#include "Utility/RobloxServicesTools.hpp"


// Roblox Studio Headers
#include "RobloxApplicationManager.hpp"
#include "RobloxMainWindow.hpp"
#include "RobloxSettings.hpp"
#include "StudioUtilities.hpp"
#include "Utility/StandardOut.hpp"

#include "winrc.h"

#ifdef ENABLE_APPLE_CRASH_REPORTER
#include "AppleCrashReporter.hpp"
#endif

#include "Profiler.hpp"

#if ENABLE_CHROMIUM_FRAMES
#include "Utility/CefIntegration.hpp"
#endif

LOGGROUP(RobloxWndInit)

static bool UploadDmp = false;
static bool showVersion = false;

/**
 * Given an argument name, get the parameter for the argument.
 */
QString getArgParameter(const QStringList& args, const QString& arg)
{
    const int index = args.indexOf(arg) + 1;
    return index > 0 && index < args.size() ? args[index] : "";
}

/**
 * Gets the file name and script name from the commandline arguments.
 */
void parseCommandLineArgs(QMap<QString, QString>& argMap)
{
    const QStringList args = QApplication::arguments();

    // Helper lambda to check for an argument and optionally get its next value
    auto getArgValue = [&args](const QString& arg, bool nextValue = true) -> QString
    {
        int index = args.indexOf(arg);
        if (index != -1 && nextValue && index + 1 < args.size())
        {
            return args.at(index + 1);
        }
        return index != -1 ? "TRUE" : nullptr;
    };

#ifdef ENABLE_CHROMIUM_FRAMES
    if (args.indexOf("--nochromium") == -1)
    {
        Aya::CefIntegration::initialize();
    }
#endif

#ifdef WIN32
    // Parsing arguments and filling the map
    argMap[FileLocationArgument] = getArgValue(FileLocationArgument);
    argMap[ScriptArgument] = getArgValue(ScriptArgument);
    argMap[BrowserTrackerId] = getArgValue(BrowserTrackerId);
    argMap[TestModeArgument] = getArgValue(TestModeArgument, false);
    argMap[IDEArgument] = getArgValue(IDEArgument, false);
    argMap[BuildArgument] = getArgValue(BuildArgument, false);
    argMap[AvatarModeArgument] = getArgValue(AvatarModeArgument, false);
    argMap[DebuggerArgument] = getArgValue(DebuggerArgument, false);
    argMap[StartEventArgument] = getArgValue(StartEventArgument);
    argMap[ReadyEventArgument] = getArgValue(ReadyEventArgument);
    argMap[ShowEventArgument] = getArgValue(ShowEventArgument);
    argMap[AuthUrlArgument] = getArgValue(AuthUrlArgument);
    argMap[AuthTicketArgument] = getArgValue(AuthTicketArgument);

    // Special handling for cases with one or two arguments which might be a file
    if (args.size() == 2 && QFile::exists(args[1]))
    {
        argMap[IDEArgument] = "TRUE";
        argMap[FileLocationArgument] = args[1];
    }
    else if (args.size() == 3 && (args[1] == IDEArgument || args[1] == FileLocationArgument) && QFile::exists(args[2]))
    {
        argMap[IDEArgument] = "TRUE";
        argMap[FileLocationArgument] = args[2];
    }
#endif
}

#ifdef _WIN32

bool attachCurrentProcessToDebugger()
{
    return true;
}

#endif

class RobloxApplication : public QApplication
{
public:
    RobloxApplication(int& argc, char** argv)
        : QApplication(argc, argv)
    {
        QPalette pal = this->palette();
        pal.setColor(QPalette::Window, Qt::white);
        this->setPalette(pal);

        QFont font = this->font();
        font.setHintingPreference(QFont::PreferNoHinting);
        setFont(font);
    }

    bool notify(QObject* receiver, QEvent* event) override
    {
        AYAPROFILER_SCOPE("Qt", "$Event");
        AYAPROFILER_LABELF("Qt", "%s::%d", receiver->objectName().toStdString().c_str(), event->type());

        return QApplication::notify(receiver, event);
    }
};

int main(int argc, char* argv[])
{
#ifdef _WIN32
    // This was compiled with sse2.  If this is placed too late, there is a good
    // chance some float operation will occur.
    if (!G3D::System::hasSSE2())
    {
        MessageBoxA(NULL, "This platform lacks SSE2 support.", "Aya", MB_OK);
        return false;
    }
    // CoInitialize(NULL);
#endif

    RobloxApplication app(argc, argv);

    // Platform independent settings
    QApplication::setApplicationName(AYA_PROJECT_NAME " Studio");
    QApplication::setApplicationVersion(VERSION_FULL_STR);

    // add plugins path for loading images in webpage
#ifdef _WIN32
    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath() + "/imageformats");
#endif

    int retVal = -1;

    try
    {
        QMap<QString, QString> argMap;
        parseCommandLineArgs(argMap);

        RobloxApplicationManager::instance().registerApplication();
        RobloxMainWindow rbxMainWin(argMap);

        // Following is required so that when we launch from QProcess new instance of RobloxStudio should come to the top of Z-Order
        rbxMainWin.activateWindow();
        rbxMainWin.raise();

        retVal = app.exec();
    }

    catch (std::runtime_error const& exp)
    {
        FASTLOG1(FLog::RobloxWndInit, "Application closed due to an exception: '%s'", exp.what());
    }

#if defined(ENABLE_CHROMIUM_FRAMES)
    Aya::CefIntegration::shutdown();
#endif

    RobloxApplicationManager::instance().unregisterApplication();
    return retVal;
}
