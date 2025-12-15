// clang-format off

#include <QApplication>
#include <boost/program_options.hpp>
#include <string>

#include "DataModel/ContentProvider.hpp"
#include "DataModel/DataModel.hpp"
#include "DataModel/DebugSettings.hpp"
#include "DataModel/GameBasicSettings.hpp"

#include "Script/ScriptContext.hpp"
#include "Utility/StandardOut.hpp"
#include "Utility/Statistics.hpp"

#include "AppSettings.hpp"
#include "MainWindow.hpp"
#include "server.hpp"
#include "winrc.h"

namespace po = boost::program_options;

bool tryRead(const std::string& input, std::string& output)
{
    std::ifstream file(input);
    if (file)
    {
        std::stringstream buffer;
        buffer << file.rdbuf();
        output = buffer.str();
        return true;
    }

    if (Aya::ContentProvider::isHttpUrl(input))
    {
        try
        {
            Aya::Http http(input);
            http.get(output);
            return true;
        }
        catch (const std::exception& e)
        {
            Aya::StandardOut::singleton()->printf(Aya::MESSAGE_ERROR, "Error trying to fetch '%s': %s", input.c_str(), e.what());
            return false;
        }
    }

    output = input;
    return true;
}

QCoreApplication* createApplication(int &argc, const char *argv[])
{
    if (!argc)
        return new QApplication(argc, (char**)argv);

    return new QCoreApplication(argc, (char**)argv);
}

int main(int argc, const char* argv[])
{
    QScopedPointer<QCoreApplication> app(createApplication(argc, argv));
    bool ui = qobject_cast<QApplication*>(app.data()) != nullptr;

    QCoreApplication::setOrganizationName(AYA_PROJECT_NAME);
    QCoreApplication::setApplicationName(AYA_PROJECT_NAME " Server");
    QCoreApplication::setApplicationVersion(VERSION_FULL_STR);

    po::options_description desc("Aya Server options");

    std::string testScript, evaluateScript;
    std::string instanceUrl, instanceAccessKey;
    std::string trustCheckUrl;
    std::string masterServerUrl, masterServerKey, masterServerHost, masterServerName, masterServerDescription;
    std::string localServerPlace, localServerPassword;
    std::string contentPath;

    int gridPort, localServerPort;

    desc.add_options()
        ("help,?", "Usage help")
        ("version,V", "Print version and exit")
        ("verbose", "Enable verbose logging (prints all output messages including FastLogs)")
        ("lan,L", "For local network or single-player servers (disables all online interaction)")
        ("insecure,I", "Disables anti-cheat & authentication checks")
        ("test", po::value<std::string>(&testScript)->implicit_value("ayaasset://scripts/ServerCoreScripts/Test.lua"), "Runs a test job with the given Lua script")
        ("eval,E", po::value<std::string>(&evaluateScript), "Lua script to run")
        ("port,G", po::value<int>(&gridPort)->implicit_value(64989), "Runs the Grid HTTP service on the given port")
        ("instance-url,U", po::value<std::string>(&instanceUrl)/*->default_value("http://www.kiseki.lol")*/, "Instance URL (leave blank for no connected instance)")
        ("instance-access-key,k", po::value<std::string>(&instanceAccessKey), "Instance access key (for sensitive instance actions)")
        ("trust-check-url,T", po::value<std::string>(&trustCheckUrl), "Only allow asset requests from the given domain (leave blank for no trust check)")
        ("ms-url,a", po::value<std::string>(&masterServerUrl), "Master server URL for broadcasting server details (leave blank for no master server connection)")
        ("ms-key,A", po::value<std::string>(&masterServerKey), "Master server authorization key (if the master server requires it)")
        ("ms-host,H", po::value<std::string>(&masterServerHost)->default_value("Bloxhead"), "Name of the host to be broadcasted")
        ("ms-name,N", po::value<std::string>(&masterServerName)->default_value("Aya Server"), "Name of the server to be broadcasted")
        ("ms-description,d", po::value<std::string>(&masterServerDescription)->default_value("Welcome to my server!"), "Short description of the server to be broadcasted")
        ("server-port,P", po::value<int>(&localServerPort)->implicit_value(53640), "Runs a server on the given port")
        ("server-place,f", po::value<std::string>(&localServerPlace)->default_value("ayaasset://place.rbxl"), "Path to place file for local server")
        ("server-password,K", po::value<std::string>(&localServerPassword), "Local server password for direct connections")
        ("content-path", po::value<std::string>(&contentPath)->default_value(GetAssetFolder()), "Path to the content directory")
        ("no-repl", "Disable terminal REPL");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return 0;
    }

    if (vm.count("version"))
    {
        std::cout << VERSION_FULL_STR << std::endl;
        return 0;
    }

    AppSettings settings(QCoreApplication::applicationDirPath().toStdString());
    if (!settings.load())
    {
        Aya::StandardOut::singleton()->printf(Aya::MESSAGE_ERROR, "Failed to load AppSettings.ini - please make sure it exists with a valid ContentPath under the Aya group, and make sure that it is free of any errors.");
        return 0;
    }

    // Override AppSettings loaded settings with anything from the command line:
    if (!IsVerboseLogging())
        SetVerboseLogging(vm.count("verbose") > 0);

    if (!IsInsecureMode())
        SetInsecureMode(vm.count("insecure") > 0);

    if (!instanceUrl.empty())
    {
        SetBaseURL(instanceUrl.c_str());
        if (!instanceAccessKey.empty())
        {
            SetInstanceAccessKey(instanceAccessKey.c_str());
        }
    }

    if (!trustCheckUrl.empty())
    {
        SetTrustCheckURL(trustCheckUrl.c_str());
    }

    if (!masterServerUrl.empty())
    {
        SetMasterServerURL(masterServerUrl.c_str());
        if (!masterServerKey.empty())
        {
            SetMasterServerKey(masterServerKey.c_str());
        }
    }

    if (vm.count("test"))
    {
        std::string script;
        if (!tryRead(testScript, script))
            return 1;

        // queue_job("Test Job", script, 60 * 60 * 24);
    }

    if (!evaluateScript.empty())
    {
        // Evaluate script and exit
        // May be a Lua script or a path to a file or a path to an asset
        bool success = true;

        try
        {
            auto dm = Aya::DataModel::createDataModel(false, new Aya::NullVerb(NULL, ""), false);
            Aya::DataModel::LegacyLock lock(dm, Aya::DataModelJob::Write);

            std::string script;
            if (Aya::ContentProvider::isUrl(evaluateScript))
            {
                std::auto_ptr<std::istream> stream = Aya::ServiceProvider::create<Aya::ContentProvider>(dm.get())->getContent(Aya::ContentId(evaluateScript));
                script = std::string(static_cast<std::stringstream const&>(std::stringstream() << stream->rdbuf()).str());
            }
            else
            {
                success = tryRead(evaluateScript, script);
            }

            if (success)
            {
                if (script.empty())
                {
                    throw std::runtime_error("Could not load script '" + evaluateScript + "'");
                }

                Aya::ScriptContext* sc = Aya::ServiceProvider::create<Aya::ScriptContext>(dm.get());
                sc->executeInNewThread(Aya::Security::WebService, Aya::ProtectedString::fromTrustedSource(script), "Script");
            }
        }
        catch (std::exception& e)
        {
            Aya::StandardOut::singleton()->print(Aya::MESSAGE_ERROR, e);
            success = false;
        }

        return success ? 0 : 1;
    }

    bool isLAN = vm.count("lan") > 0;
    if (true)
    {
        MainWindow window(NULL);
        window.setWindowTitle("Aya Server");
        window.show();
        window.setFocus();
    }

    start_aya_server(
        ui,
        isLAN,
        gridPort,
        localServerPort,
        localServerPlace,
        localServerPassword,
        masterServerHost,
        masterServerName,
        masterServerDescription
    );

    return app->exec();
}