#include <boost/program_options.hpp>
#include <generated/RCCServiceSoap.nsmap>
#include <generated/soapRCCServiceSoapService.h>

#include "DataModel/GameBasicSettings.hpp"
#include "Windows/Information.h"

#include "Script/ScriptContext.hpp"
#include "DataModel/ContentProvider.hpp"

#include "DataModel/DataModel.hpp"

#include "DataModel/DebugSettings.hpp"

#include "Utility/ContentId.hpp"

#include "Utility/Http.hpp"

#include "Utility/ProtectedString.hpp"

#include "Utility/StandardOut.hpp"

#include "Utility/Statistics.hpp"

#include "API.hpp"

#include "boost.hpp"
#include "TaskScheduler.hpp"

#include "linenoise.hpp"
#include "Lua/lua.h"

#include "Utility/AyaService.hpp"

#ifdef __linux
#include <iostream>
#include <string>
#include <mutex>
#include <boost/signals2.hpp>
#include <boost/bind/bind.hpp>
#include <SDL3/SDL.h>
#endif

#include <boost/algorithm/string.hpp>

namespace po = boost::program_options;

#if defined(_WIN32) || defined(_WIN64)
class PrintfLogger
{
    Aya::signals::scoped_connection messageConnection;
    HANDLE handle;
    Aya::spin_mutex mutex;

public:
    PrintfLogger()
        : handle(GetStdHandle(STD_OUTPUT_HANDLE))
    {
        messageConnection = Aya::StandardOut::singleton()->messageOut.connect(boost::bind(&PrintfLogger::onMessage, this, _1));
    }

protected:
    void onMessage(const Aya::StandardOutMessage& message)
    {
        Aya::spin_mutex::scoped_lock lock(mutex);

        switch (message.type)
        {
        case Aya::MESSAGE_OUTPUT:
            SetConsoleTextAttribute(handle, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            break;
        case Aya::MESSAGE_INFO:
            SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            break;
        case Aya::MESSAGE_WARNING:
            SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN);
            break;
        case Aya::MESSAGE_ERROR:
            SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_INTENSITY);
            break;
        }

        printf("%s\n", message.message.c_str());
        SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }
};
#else
class PrintfLogger
{
    Aya::signals::scoped_connection messageConnection;
    std::mutex mutex;

public:
    PrintfLogger()
    {
        messageConnection = Aya::StandardOut::singleton()->messageOut.connect(boost::bind(&PrintfLogger::onMessage, this, boost::placeholders::_1));
    }

protected:
    void onMessage(const Aya::StandardOutMessage& message)
    {
        std::lock_guard<std::mutex> lock(mutex);

        // ANSI escape codes for color output
        const char* colorCode = "\033[0m";
        switch (message.type)
        {
        case Aya::MESSAGE_OUTPUT:
            colorCode = "\033[34m";
            break;
        case Aya::MESSAGE_INFO:
            colorCode = "\033[37m";
            break;
        case Aya::MESSAGE_WARNING:
            colorCode = "\033[33m";
            break;
        case Aya::MESSAGE_ERROR:
            colorCode = "\033[31m";
            break;
        default:
            break;
        }

        std::cout << colorCode << message.message << "\033[0m" << std::endl;
    }
};
#endif

template<class Soap>
class ExceptionAwareSoap : public Soap
{
public:
    virtual int dispatch()
    {
        try
        {
            return Soap::dispatch();
        }
        catch (std::exception& e)
        {
            return soap_receiver_fault(this, e.what(), NULL); // return fault to sender
        }
        catch (std::string& s)
        {
            AYACRASH();
            return soap_receiver_fault(this, s.c_str(), NULL); // return fault to sender
        }
        catch (...)
        {
            AYACRASH();
            return soap_receiver_fault(this, "Unexpected C++ exception type", NULL); // return fault to sender
        }
    }

    virtual RCCServiceSoapService* copy()
    {
        ExceptionAwareSoap<Soap>* dup = new ExceptionAwareSoap<Soap>();
        soap_copy_context(dup, this);
        return dup;
    }
};

ExceptionAwareSoap<RCCServiceSoapService> service;

// TODO: Move this into some sort of RCCServiceSoap.hpp?
void start_CWebService();
void stop_CWebService();

static void startupRCC(int port)
{
#ifdef USE_BLOXBLOX_BRANDING
    printf("Starting Bloxblox.Aya.Server v%s...\n", VERSION_MAJOR_MINOR_PATCH_STR);
#else
    printf("Starting Aya.Server v%s...\n", VERSION_MAJOR_MINOR_PATCH_STR);
#endif

    start_CWebService();

    if (port != -1)
    {
        service.accept_timeout = 1; // server stops after 1 second of client inactivity
        // service.send_timeout = 60; // 60 seconds
        // service.recv_timeout = 60; // 60 seconds
        // soap.max_keep_alive = 100; // max keep-alive sequence
        SOAP_SOCKET m = service.bind(NULL, port, 100);

        if (!soap_valid_socket(m))
            throw std::runtime_error(*soap_faultstring(&service));
    }
    printf("Started Aya.Server!\n");

    if (port != -1)
    {
        Aya::StandardOut::singleton()->printf(Aya::MESSAGE_SENSITIVE, "Now listening for incoming SOAP requests on port TCP/%d", port);
    }
}

static void stepRCC()
{
    SOAP_SOCKET s = service.accept();

    if (!soap_valid_socket(s))
    {
        if (service.errnum)
            throw std::runtime_error(*soap_faultstring(&service));

        return;
    }

    RCCServiceSoapService* copy = service.copy(); // make a safe copy

    if (!copy)
        throw std::runtime_error(*soap_faultstring(&service));

    try
    {
        copy->serve();
        delete copy;
    }
    catch (...)
    {
        AYACRASH();
    }
}

static void shutdownRCC()
{
    printf("Shutting down...\n");
    stop_CWebService();

    // if on linux, SDL_Quit() for DummyWindow
#ifdef __linux
    SDL_Quit();
#endif
}


static void handleScriptCommand(std::string& execCommand)
{
    if (execCommand.empty())
        return;

    ns1__ScriptExecution se;
    // d9mz - error: taking the address of a temporary object of type 'std::string'
    std::string serverName = "Command Line";
    se.name = &serverName;
    se.script = &execCommand;

    _ns1__Execute execute;
    _ns1__ExecuteResponse response;
    execute.script = &se;
    execute.jobID = "Test";

    RCCServiceSoapService* copy = service.copy();
    copy->Execute(&execute, response);
}

static void escapeLuaString(std::string& string)
{
    boost::replace_all(string, "[", "\\[");
    boost::replace_all(string, "]", "\\]");
    boost::replace_all(string, "\"", "\\\"");
}

int main(int argc, const char* argv[])
{
    // Pipe Aya::StandardOut to printf
    static boost::scoped_ptr<PrintfLogger> standardOutLog(new PrintfLogger());

    // Parse arguments
    std::string evaluateScript;
    int port = 64989;
    std::string contentPath = "content";
    std::string httpAccessKey;
    std::string baseUrl;
    std::string testScript;
    std::string password;
    std::string masterServerAuthorization;
    std::string masterServerUrl;
    std::string masterServerHost;
    std::string masterServerName;
    std::string masterServerMotdPreview;
    std::string masterServerMotdFile;
    int localServerPort;
    int virtualVersion;
    std::string localServerPlace;

    po::options_description desc("Aya options");

    desc.add_options()("help,?", "Usage help")("version,V", "Print version and exit")("eval,E", po::value<std::string>(&evaluateScript),
        "Lua script to run")("port", po::value<int>(&port)->default_value(64989), "SOAP port to listen on. Set to -1 to disable")("contentPath",
        po::value<std::string>(&contentPath)->default_value(DEFAULT_CONTENT_PATH),
        "Path to the content directory")("httpAccessKey", po::value<std::string>(&httpAccessKey)->default_value(""), "HTTP access key")("baseUrl",
        po::value<std::string>(&baseUrl)->default_value("http://www.kiseki.lol"),
        "Base URL")("test", po::value<std::string>(&testScript)->implicit_value("test.lua"), "Runs a test job with the given Lua script")(
        "insecure", "Disables anti-cheat & authentication checks")("verbose", "Enable verbose logging")("noconsole,x", "Disable command line REPL")(
        "password,K", po::value<std::string>(&password), "Set password")("masterServerAuthorization,A",
        po::value<std::string>(&masterServerAuthorization))("masterServerUrl,a", po::value<std::string>(&masterServerUrl))(
        "localServer", "Aya local server flag")("msHost,H", po::value<std::string>(&masterServerHost)->default_value("Bloxhead"))(
        "msName,N", po::value<std::string>(&masterServerName)->default_value("Aya Server"))(
        "msMotdPreview,m", po::value<std::string>(&masterServerMotdPreview)->default_value("Set -m to set a preview"))(
        "msMotdFile,M", po::value<std::string>(&masterServerMotdFile)->default_value("motd.htm"))("localServerPort,P",
        po::value<int>(&localServerPort)->default_value(53640))("virtualVersion,vv", po::value<int>(&virtualVersion)->default_value(0))(
        "localServerPlace,p", po::value<std::string>(&localServerPlace)->default_value("ayaasset://test.rbxl"))(
        "httpClientSettings,S", "use website for clientsettings");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    Aya::GameBasicSettings::VirtualVersion vv = static_cast<Aya::GameBasicSettings::VirtualVersion>(virtualVersion);
    Aya::GameBasicSettings::singleton().setVirtualVersionInternal(vv);

    bool isTest = vm.count("test") > 0;
    bool isInsecure = vm.count("insecure") > 0;
    bool isVerbose = vm.count("verbose") > 0;
    bool noConsole = vm.count("noconsole") > 0;
    bool isLocalServer = vm.count("localServer") > 0;

    SetFetchLocalClientSettings(!vm.count("httpClientSettings"));

    if (vm.count("help"))
    {
        std::cout << desc << "\n";
        return 0;
    }

    if (vm.count("version"))
    {
        std::cout << VERSION_FULL_STR << "\n";
        return 0;
    }

    if (vm.count("eval"))
    {
        bool success = true;

        try
        {
            std::string code = vm["eval"].as<std::string>();

            std::ifstream file(code);
            if (file)
            {
                std::stringstream buffer;
                buffer << file.rdbuf();
                code = buffer.str();
            }

            // TODO: Account for ayaasset://, ayaassetid://, etc.
            if (Aya::ContentProvider::isHttpUrl(code))
            {
                Aya::Http http(code);
                http.get(code);
            }

            Aya::GameBasicSettings::VirtualVersion vv = static_cast<Aya::GameBasicSettings::VirtualVersion>(virtualVersion);

            shared_ptr<Aya::DataModel> dm = Aya::DataModel::createDataModel(false, new Aya::NullVerb(NULL, ""), false, vv);
            Aya::ScriptContext* sc = Aya::ServiceProvider::create<Aya::ScriptContext>(dm.get());
            Aya::AyaService* ayaService = Aya::ServiceProvider::create<Aya::AyaService>(dm.get());
            ayaService->setInitialVersion(vv);
            sc->executeInNewThread(Aya::Security::WebService, Aya::ProtectedString::fromTrustedSource(code), "Script");
        }
        catch (std::exception& e)
        {
            Aya::StandardOut::singleton()->print(Aya::MESSAGE_ERROR, e);
            success = false;
        }

        return success ? 0 : 1;
    }

    // Main startup
    try
    {
        Aya::ContentProvider::setAssetFolder(contentPath.c_str());
        SetBaseURL(baseUrl.c_str());

        startupRCC(port);

        if (vm.count("password"))
        {
            Aya::Network::setPassword(vm["password"].as<std::string>());
            Aya::AyaService::passwordProtected = true;
        }

        if (isVerbose)
        {
            // TODO: Pipe FastLog to console
        }

        if (isInsecure)
        {
            // TODO: Disable all security checks
        }

        bool testJob = false;

        if (isLocalServer)
        {
            Aya::StandardOut::singleton()->print(Aya::MESSAGE_WARNING, "Running an Aya Local Server.");
            Aya::AyaService::masterServerUrl = masterServerUrl;
            Aya::AyaService::masterServerAuthorization = masterServerAuthorization;
            Aya::AyaService::localServer = true;
        }

        if (isTest || isLocalServer)
        {

            if (!isTest)
            {
                testScript = "content/scripts/Host.lua";
            }

            std::ifstream file(testScript);
            std::string script;
            if (!file)
            {
                std::cout << "Could not open file " << testScript << std::endl;
                return 1;
            }

            std::stringstream buffer;
            buffer << file.rdbuf();

            if (isLocalServer)
            {
                // TODO: escape all the text so it doesn't break Lua

                std::string motd;
                std::ifstream file(masterServerMotdFile);
                if (!file)
                {
                    std::cout << "Could not open file " << masterServerMotdFile << std::endl;
                }
                else
                {
                    std::stringstream motdBuffer;
                    motdBuffer << file.rdbuf();
                    motd = motdBuffer.str();

                    escapeLuaString(motd);

                    buffer << "\ngame:GetService(\"AyaService\").ServerMotdContent = [[" << motd << "]]";
                }

                escapeLuaString(masterServerMotdPreview);
                buffer << "\ngame:GetService(\"AyaService\").ServerMotdPreview = \"" << masterServerMotdPreview << "\"";
                escapeLuaString(masterServerHost);
                buffer << "\ngame:GetService(\"AyaService\").Host = \"" << masterServerHost << "\"";
                escapeLuaString(masterServerName);
                buffer << "\ngame:GetService(\"AyaService\").ServerName = \"" << masterServerName << "\"";

                escapeLuaString(localServerPlace);
                buffer << "\ngame:GetService(\"AyaService\"):AnnounceMasterServer()";
                buffer
                    << "\ngame:GetService(\"Players\").PlayerAdded:connect(function() game:GetSerivce(\"AyaService\"):AnnounceMasterServer() "
                       "end)"
                    << "\ngame:GetService(\"Players\").PlayerRemoving:connect(function() game:GetSerivce(\"AyaService\"):AnnounceMasterServer() end)";

                char startLine[2048];
                snprintf(startLine, 2048, "Start(%i, \"%s\")", localServerPort, localServerPlace.c_str());

                if (masterServerUrl == "")
                {
                    Aya::StandardOut::singleton()->printf(
                        Aya::MESSAGE_ERROR, "Master server ping will be disabled. Specify a master server with -a <url> -A <master server key>");
                }

                buffer << "\n" << startLine;
            }
            script = buffer.str();

            testJob = true;

            RCCServiceSoapService* copy = service.copy(); // make a safe copy

            if (!copy)
                throw std::runtime_error(*soap_faultstring(&service));

            ns1__Job job;
            job.id = "Test";
            job.expirationInSeconds = 60 * 60 * 24;

            ns1__ScriptExecution se;
            // d9mz - error: taking the address of a temporary object of type 'std::string'
            std::string serverName = "Start Server";
            se.name = &serverName;
            se.script = &script;
            _ns1__OpenJob openJob;
            _ns1__OpenJobResponse response;
            openJob.script = &se;
            openJob.job = &job;

            copy->OpenJob(&openJob, response);
        }

        std::thread heartbeat(
            []
            {
                while (true)
                {
                    stepRCC();
                }
            });

        if (!testJob && !noConsole)
        {
            Aya::StandardOut::singleton()->print(Aya::MESSAGE_ERROR,
                "Im sorry but I will not let you use the console unless if you are using --test. Specify --noconsole to suppress this message");
            noConsole = true;
        }

        if (!noConsole)
        {
            std::cout << "Lua interactive command prompt" << std::endl;

            linenoise::SetHistoryMaxLen(100);
        }

        while (true)
        {
            if (noConsole)
            {
                std::string toExecute;

                std::getline(std::cin, toExecute);
            }
            else
            {
                std::string toExecute;
                auto quit = linenoise::Readline(LUA_RELEASE "> ", toExecute);

                if (quit)
                    break;

                linenoise::AddHistory(toExecute.c_str());

                try
                {
                    int errorLine = 0;
                    std::string errorMessage;
                    if (Aya::ScriptContext::checkSyntax(toExecute.c_str(), errorLine, errorMessage))
                    {
                        Aya::StandardOut::singleton()->printf(Aya::MESSAGE_OUTPUT, "%s", toExecute.c_str());
                        handleScriptCommand(toExecute);
                    }
                    else
                    {
                        Aya::StandardOut::singleton()->printf(Aya::MESSAGE_ERROR, "Error in script: %s", errorMessage.c_str());
                        continue;
                    }
                }
                catch (std::exception& e)
                {
                }
            }
        }

        heartbeat.detach();

        shutdownRCC();
    }
    catch (std::exception& e)
    {
        Aya::StandardOut::singleton()->print(Aya::MESSAGE_ERROR, e);
    }
}