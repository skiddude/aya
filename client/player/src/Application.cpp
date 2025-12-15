#include "Application.hpp"
#include "DataModel/GameBasicSettings.hpp"
#include "Reflection/Reflection.hpp"
#include "Utility/AyaService.hpp"
#include "Utility/StandardOut.hpp"
#include "View.hpp"
#include "DataModel/Game.hpp"
#include "FunctionMarshaller.hpp"
#include "Document.hpp"
#include "Utility/Statistics.hpp"
#include "DataModel/ContentProvider.hpp"
#include "Render/VisualEngine.hpp"
#include "Xml/XmlSerializer.hpp"
#include "Reflection/ReflectionMetadata.hpp"

#include "boost/any.hpp"
#include "boost/intrusive/list.hpp"
#include "Xml/WebParser.hpp"
#include "DataModel/Stats.hpp"
#include "DataModel/TeleportService.hpp"
#include "DataModel/DebugSettings.hpp"
#include "API.hpp"
#include "Utility/HttpAsync.hpp"
#include "AvatarViewService.hpp"
#include "Script/CoreScript.hpp"
#include "Script/ScriptContext.hpp"

#include <filesystem>
#include <QMessageBox>
#include <QSettings>

#include "AppSettings.hpp"
#include "winrc.h"

FASTFLAG(PlaceLauncherUsePOST)

namespace Aya
{

AYA_REGISTER_CLASS(AvatarViewService);

Application::Application()
{
    gameReady = false;
    launchMode = Play;
}

#ifdef _WIN32
class PrintfLogger
{
    Aya::signals::scoped_connection messageConnection;
    HANDLE handle;
    Aya::spin_mutex mutex;

public:
    PrintfLogger()
        : handle(GetStdHandle(STD_OUTPUT_HANDLE))
    {
        messageConnection = Aya::StandardOut::singleton()->messageOut.connect(boost::bind(&PrintfLogger::onMessage, this, boost::placeholders::_1));
    }

protected:
    void onMessage(const Aya::StandardOutMessage& message)
    {
        Aya::spin_mutex::scoped_lock lock(mutex);

        time_t now = time(NULL);
        struct tm* timeinfo = localtime(&now);
        char buffer[30];
        strftime(buffer, sizeof(buffer), "[%m/%d/%Y %I:%M:%S %p]", timeinfo);

        SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        printf("%s ", buffer);

        const char* levelStr = "UNKNOWN";
        WORD levelColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

        switch (message.type)
        {
        case Aya::MESSAGE_OUTPUT:
            levelColor = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            levelStr = "OUTPUT";
            break;
        case Aya::MESSAGE_INFO:
            levelColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
            levelStr = "INFO";
            break;
        case Aya::MESSAGE_WARNING:
            levelColor = FOREGROUND_RED | FOREGROUND_GREEN;
            levelStr = "WARNING";
            break;
        case Aya::MESSAGE_ERROR:
            levelColor = FOREGROUND_RED | FOREGROUND_INTENSITY;
            levelStr = "ERROR";
            break;
        }

        SetConsoleTextAttribute(handle, levelColor);
        printf("[%s] ", levelStr);

        // white
        SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        printf("%s\n", message.message.c_str());

        // reset
        SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }
};
#endif

static std::string readStringValue(shared_ptr<const Reflection::ValueTable> jsonResult, std::string name)
{
    Reflection::ValueTable::const_iterator itData = jsonResult->find(name);
    if (itData != jsonResult->end())
    {
        return itData->second.get<std::string>();
    }
    else
    {
        throw std::runtime_error(Aya::format("Unexpected string result for %s", name.c_str()));
    }
}

static int readIntValue(shared_ptr<const Reflection::ValueTable> jsonResult, std::string name)
{
    Reflection::ValueTable::const_iterator itData = jsonResult->find(name);
    if (itData != jsonResult->end())
    {
        return itData->second.get<int>();
    }
    else
    {
        throw std::runtime_error(Aya::format("Unexpected int result for %s", name.c_str()));
    }
}

Application::RequestPlaceInfoResult Application::requestPlaceInfo(
    const std::string url, std::string& authenticationUrl, std::string& ticket, std::string& scriptUrl) const
{
    try
    {
        std::string response;
        if (FFlag::PlaceLauncherUsePOST)
        {
            std::istringstream input("");
            Aya::Http(url).post(input, Aya::Http::kContentTypeDefaultUnspecified, false, response);
        }
        else
        {
            Aya::Http(url).get(response);
        }

        std::stringstream jsonStream;
        jsonStream << response;
        shared_ptr<const Reflection::ValueTable> jsonResult(Aya::make_shared<const Reflection::ValueTable>());
        bool parseResult = WebParser::parseJSONTable(jsonStream.str(), jsonResult);
        if (parseResult)
        {
            int status = readIntValue(jsonResult, "status");
            if (status == 2)
            {
                authenticationUrl = readStringValue(jsonResult, "authenticationUrl");
                ticket = readStringValue(jsonResult, "authenticationTicket");
                scriptUrl = readStringValue(jsonResult, "joinScriptUrl");
                return SUCCESS;
            }
            else if (status == 6)
                return GAME_FULL;
            else if (status == 10)
                return USER_LEFT;
            else
            {
                // 0 or 1 is not an error - it is a sign that we should wait
                if (status == 0 || status == 1)
                    return RETRY;
            }
        }
    }
    catch (Aya::base_exception& e)
    {
        StandardOut::singleton()->printf(MESSAGE_ERROR, "Exception when requesting place info: %s. ", e.what());
    }

    return FAILED;
}

static HttpFuture fetchJoinScriptAsync(const std::string& url)
{
    if (ContentProvider::isUrl(url) && Aya::Network::isTrustedContent(url.c_str()))
    {
        return HttpAsync::getWithRetries(url, 5);
    }
    else
    {
        // silent error is harder to hack
        return boost::shared_future<std::string>();
    }
}

void Application::Initialize(OgreWidget* window)
{
    mainWindow = window;
    Aya::StandardOut::singleton()->printf(Aya::MESSAGE_SYSTEM, "Welcome to %s Player v%s!", AYA_PROJECT_NAME, VERSION_FULL_STR);

    // should set to governor baseurl, initial setup ask for master server url?
    QSettings settings;
    std::string url = settings.value("JsHelpers/masterServerUrl").toString().toStdString();

    Game::globalInit(false);

    std::string authenticationUrl;
    std::string authenticationTicket;
    std::string scriptUrl;
    bool scriptIsPlaceLauncher = false;

    if (vm.count("authenticationUrl") > 0 && vm.count("authenticationTicket") > 0 && vm.count("joinScriptUrl") > 0)
    {
        authenticationUrl = vm["authenticationUrl"].as<std::string>();
        authenticationTicket = vm["authenticationTicket"].as<std::string>();
        scriptUrl = vm["joinScriptUrl"].as<std::string>();

        if (vm.find("browserTrackerId") != vm.end())
        {
            Stats::setBrowserTrackerId(vm["browserTrackerId"].as<std::string>());

            TeleportService::SetBrowserTrackerId(vm["browserTrackerId"].as<std::string>());
        }

        std::string lowerScriptUrl = scriptUrl;
        std::transform(lowerScriptUrl.begin(), lowerScriptUrl.end(), lowerScriptUrl.begin(), tolower);
        if (lowerScriptUrl.find("placelauncher.ashx") != std::string::npos)
        {
            launchMode = Play_Protocol;
            scriptIsPlaceLauncher = true;
        }
    }

    marshaller = FunctionMarshaller::GetWindow();
    TaskScheduler::singleton().setThreadCount(TaskSchedulerSettings::singleton().getThreadPoolConfig());

    if (scriptUrl != "")
    {
        HttpFuture joinScriptResult;
        if (!scriptIsPlaceLauncher)
            joinScriptResult = fetchJoinScriptAsync(scriptUrl);

        InitializeNewGame(joinScriptResult, GameBasicSettings::VirtualVersion::VERSION_2016);
    }
}

shared_ptr<DataModel> Application::getDM()
{
    return currentDocument->getGame()->getDataModel();
}

void Application::InitializeNewGame(HttpFuture& scriptResult, GameBasicSettings::VirtualVersion vv)
{
    gameReady = true;
    InitializeNewGame(vv);

    if (boost::shared_ptr<Aya::DataModel> datamodel = currentDocument->getGame()->getDataModel())
    {
        currentDocument->startedSignal.connect(boost::bind(&Application::onDocumentStarted, this, _1));
        datamodel->submitTask(boost::bind(&Document::StartResult, currentDocument.get(), scriptResult, launchMode, false), DataModelJob::Write);
    }
}

void Application::SendScript(std::string script, GameBasicSettings::VirtualVersion virtualVersion)
{
    gameReady = true;
    if (boost::shared_ptr<Aya::DataModel> datamodel = currentDocument->getGame()->getDataModel())
    {
        currentDocument->startedSignal.connect(boost::bind(&Application::onDocumentStarted, this, _1));
        datamodel->submitTask(boost::bind(&Document::Start, currentDocument.get(), script, launchMode, false), DataModelJob::Write);
        datamodel->setInitialVersion(virtualVersion);
    }
}

void Application::InitializeNewGame(GameBasicSettings::VirtualVersion vv)
{
    mainWindow->setVisible(true);

    if (currentDocument)
    {
        currentDocument->PrepareShutdown();
        mainView->Stop();
        currentDocument->Shutdown();
        currentDocument.reset();
    }

    currentDocument.reset(new Document());
    currentDocument->Initialize(mainWindow, true, true, true, vv);
    mainView.reset(new View(mainWindow));
    mainView->setOgreWindow(dynamic_cast<OgreWindow*>(mainWindow->getOgreWindow()));
    mainView->Start(currentDocument->getGame());

    initVerbs();
}

void Application::InitializeNewEmptyGame()
{
    mainWindow->setVisible(true);

    if (currentDocument)
    {
        currentDocument->PrepareShutdown();
        mainView->Stop();
        currentDocument->Shutdown();
        currentDocument.reset();
    }

    Document* document = new Document();

    currentDocument.reset(document);
    currentDocument->Initialize(mainWindow, true, false, false, GameBasicSettings::VirtualVersion::VERSION_2012);
    mainView.reset(new View(mainWindow));
    mainView->Start(currentDocument->getGame());

    shared_ptr<Aya::DataModel> dataModel = currentDocument->getGame()->getDataModel();

    try
    {
        if (boost::optional<ProtectedString> source = CoreScript::fetchSource("AvatarView"))
        {
            if (ScriptContext* sc = Aya::ServiceProvider::create<ScriptContext>(dataModel.get()))
            {
                sc->executeInNewThread(Aya::Security::RobloxGameScript_, *source, "AvatarView");
            }
        }
    }
    catch (std::exception& e)
    {
        StandardOut::singleton()->printf(MESSAGE_ERROR, "AvatarView Error: %s", e.what());
    }

    initVerbs();
}

void Application::sendInputEvent(shared_ptr<InputObject> obj)
{
    currentDocument->sendInputObject(obj);
}

void Application::onResize(int x, int y)
{
    mainView->onResize(x, y);
}

void Application::loadAppSettings()
{
    this->settings = new AppSettings(QCoreApplication::applicationDirPath().toStdString());

    if (!settings->load())
    {
        // show error
        QMessageBox::warning(
            nullptr, "Error", "Failed to load application settings from AppSettings.ini. Make sure that the file exists and is free of any errors.");
        this->shutdown();
    }
}

void Application::parseCommandLine(int argc, char** argv)
{
    std::vector<std::string> args;
    for (int i = 0; i < argc; i++)
        args.push_back(argv[i]);

    namespace po = boost::program_options;

    po::options_description desc("Options");
    desc.add_options()("help,?", "produce help message")("globalBasicSettingsPath,g", po::value<std::string>(),
        "path to GlobalBasicSettings_(n).xml")("version,v", "print version string")("id", po::value<int>(), "id of the place to join")("content,c",
        po::value<std::string>(), "path to the content directory")("authenticationUrl,a", po::value<std::string>(), "authentication url from server")(
        "authenticationTicket,t", po::value<std::string>(), "game session ticket from server")("joinScriptUrl,j", po::value<std::string>(),
        "url of launch script from server")("browserTrackerId,b", po::value<std::string>(), "browser tracking id from website")(
        "waitEvent,w", po::value<std::string>(), "window is invisible until this named event is signaled")(
        "API", po::value<std::string>(), "output API file")("dmp,d", "upload crash dmp")("play", "specifies the launching of a game")(
        "app", "specifies the launching of an app")("fast", "uses fast startup path")("console", "developer console output")(
        "nochromium", "disable cef initialization")("httpClientSettings,S", "use website for clientsettings")("vite", "use local vite server")

        ;

    po::store(po::command_line_parser(args).options(desc).run(), vm);

    if (vm.count("console") > 0)
    {
#ifdef _WIN32
        AllocConsole();
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        freopen("CONIN$", "r", stdin);

        static boost::scoped_ptr<PrintfLogger> standardOutLog(new PrintfLogger());
#else
        StandardOut::singleton()->printf(MESSAGE_WARNING, "--console is not currently available on non-Windows platforms.");
#endif
    }

    if (vm.count("help") || args.size() == 0)
    {
        std::basic_stringstream<char> options;
        desc.print(options);
        printf("%s", options.str().c_str());
        exit(-1);
    }

    if (vm.count("API") > 0)
    {
        std::string fileName = vm["API"].as<std::string>();
        std::ofstream stream(fileName.c_str());
        Aya::Reflection::Metadata::writeEverything(stream);
        exit(-1);
    }
    if (vm.count("content"))
    {
        std::string contentDir(vm["content"].as<std::string>());
        ContentProvider::setAssetFolder(contentDir.c_str());
    }

    // used to determine how we will initialize datamodel
    if (vm.count("play"))
        launchMode = Play;
}

void Application::onDocumentStarted(bool isTeleport)
{
    printf("onDocumentStarted\n");
}

void Application::initVerbs()
{
    DataModel* dm = currentDocument->getGame()->getDataModel().get();
    leaveGameVerb.reset(new LeaveGameVerb(*mainView, dm));
    Aya::ViewBase* gfx = mainView->GetGfxView();
    toggleFullscreenVerb.reset(new ToggleFullscreenVerb(*mainView, dm));
}

void Application::shutdownVerbs()
{
    toggleFullscreenVerb.reset();
    leaveGameVerb.reset();
}

void Application::prepareToShutdown()
{
    if (currentDocument) // KILL
        currentDocument->PrepareShutdown();

    if (mainView)
        mainView->AboutToShutdown();
}

void Application::shutdown()
{
    shutdownVerbs();

    if (mainView)
    { // STOP
        mainView->Stop();
        mainView.reset();
    }

    if (currentDocument)
    {
        currentDocument->Shutdown();
        currentDocument.reset();
    }

    Aya::GlobalBasicSettings::singleton()->saveState();
    Game::globalExit();
}
} // namespace Aya
