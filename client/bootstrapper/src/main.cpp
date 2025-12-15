// clang-format off

#include <QApplication>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/join.hpp>
#include <string>
#include <iostream>

#include "AppSettings.hpp"
#include "Bootstrapper.hpp"
#include "winrc.h"

namespace po = boost::program_options;

QCoreApplication* createApplication(int &argc, const char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (!qstrcmp(argv[i], "--no-gui"))
            return new QCoreApplication(argc, (char**)argv);
    }

    return new QApplication(argc, (char**)argv);
}

int main(int argc, const char* argv[])
{
    QScopedPointer<QCoreApplication> app(createApplication(argc, argv));
    bool showUI = qobject_cast<QApplication*>(app.data()) != nullptr;

    QCoreApplication::setOrganizationName(AYA_PROJECT_NAME);
    QCoreApplication::setApplicationName(AYA_PROJECT_NAME);
    QCoreApplication::setApplicationVersion(VERSION_FULL_STR);

    po::options_description desc(AYA_PROJECT_NAME " options");

    std::string mode = "player";
    bool isUsingInstance = false; // we will determine this through our bespoke methods
    bool forceSkipUpdates = false;

    std::string appSettingsPath;
    std::string instanceUrl, instanceAccessKey;

    desc.add_options()
        ("help,?", "Usage help")
        ("version,V", "Print version and exit")
        ("player", "Launch player (default)")
        ("studio", "Launch studio")
        ("server", "Launch server")
        ("skip-updates,F", "Skip update check")
        ("no-gui", "Run in no-GUI mode (server only)")
        ("instance-url,U", po::value<std::string>(&instanceUrl), "Instance URL override")
        ("instance-access-key,k", po::value<std::string>(&instanceAccessKey), "Instance access key")
        ("app-settings,S", po::value<std::string>(&appSettingsPath)->default_value("AppSettings.ini"), "Path to AppSettings.ini");

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

    AppSettings settings(appSettingsPath);
    if (!settings.load())
    {
        std::cout << "Failed to load AppSettings.ini - please make sure it exists with a valid ContentPath under the Aya group, and make sure that it is free of any errors.";
        return 0;
    }

    if (settings.hasGroup("Instance"))
    {
        if (settings.has("Instance", "Domain"))
            if (instanceUrl.empty())
                instanceUrl = settings.get("Instance", "Domain").value();

        if (settings.has("Instance", "AccessKey"))
            if (instanceAccessKey.empty())
                instanceAccessKey = settings.get("Instance", "AccessKey").value();
    }

    isUsingInstance = !instanceUrl.empty();

    // legacy holdover
    if (isUsingInstance)
        if (instanceUrl.rbegin() != instanceUrl.rend() && *instanceUrl.rbegin() != '/')
            instanceUrl = instanceUrl + "/";

    if (vm.count("studio"))
        mode = "studio";
    else if (vm.count("server"))
        mode = "server";

    if (vm.count("skip-updates"))
        forceSkipUpdates = true;

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        
        // Skip bootstrapper-specific arguments
        if (arg == "--help" || arg == "-?" ||
            arg == "--version" || arg == "-V" ||
            arg == "--player" || arg == "--studio" || arg == "--server" ||
            arg == "--skip-updates" || arg == "-F")
        {

            continue;
        }
        
        args.push_back(arg);
    }

    Bootstrapper bootstrapper(mode, showUI, forceSkipUpdates, isUsingInstance, instanceUrl, instanceAccessKey);
    bootstrapper.start(boost::algorithm::join(args, " "));

    return app->exec();
}
