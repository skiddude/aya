

#include "DataModel/Game.hpp"
#include "DataModel/factoryregistration.hpp"
#include "DataModel/GameSettings.hpp"
#include "DataModel/GameBasicSettings.hpp"
#include "DataModel/DebugSettings.hpp"
#include "DataModel/PhysicsSettings.hpp"
#include "DataModel/Workspace.hpp"
#include "DataModel/CommonVerbs.hpp"
#include "DataModel/ContentProvider.hpp"
#include "DataModel/FastLogSettings.hpp"
#include "DataModel/MegaCluster.hpp"
#include "DataModel/UserInputService.hpp"
#include "World/World.hpp"
#include "DataModel/Teams.hpp"
#include "DataModel/SpawnLocation.hpp"
#include "World/ContactManager.hpp"
#include "DataModel/ChangeHistory.hpp"
#include "DataModel/Lighting.hpp"
#include "DataModel/Test.hpp"
#include "DataModel/JointsService.hpp"
#include "DataModel/HttpRbxApiService.hpp"

#include "World/Block.hpp"

#include "Utility/ScriptInformationProvider.hpp"
#include "Utility/Profiling.hpp"
#include "Utility/Statistics.hpp"

#include "Script/LuaSettings.hpp"
#include "API.hpp"
#include "GameConfigurer.hpp"

#include "DataModel/TextButton.hpp"
#include "DataModel/ImageButton.hpp"
#include "DataModel/PlayerGui.hpp"
#include "DataModel/UserController.hpp"
#include "Script/ScriptContext.hpp"
#include "FastLog.hpp"
DYNAMIC_FASTFLAGVARIABLE(PersistenceCurlCookies, false)

namespace Aya
{


SecurePlayerGame::SecurePlayerGame(
    Verb* lockVerb, const char* baseUrl, bool shouldShowLoadingScreen, bool shouldShowCorescripts, GameBasicSettings::VirtualVersion vv)
    : Game(lockVerb, baseUrl, shouldShowLoadingScreen, vv)
{
    static boost::once_flag flag = BOOST_ONCE_INIT;
    boost::call_once(&Network::init, flag);
}

UnsecuredStudioGame::UnsecuredStudioGame(Verb* lockVerb, const char* baseUrl, bool isNetworked, bool showLoadingScreen)
    : Game(lockVerb, baseUrl, showLoadingScreen)
{
    if (isNetworked)
    {
        static boost::once_flag flag = BOOST_ONCE_INIT;
        boost::call_once(&Network::init, flag);
    }
}

void Game::globalInit(bool isStudio)
{
    Aya::Http::CookieSharingPolicy cookieSharingPolicy;
#if defined(AYA_PLATFORM_IOS) || defined(__ANDROID__) || defined(__linux) || defined(__APPLE__)
    cookieSharingPolicy = Aya::Http::CookieSharingSingleProcessMultipleThreads;
#elif defined(AYA_PLATFORM_DURANGO)
    cookieSharingPolicy = Aya::Http::CookieSharingSingleProcessMultipleThreads;
#elif defined(_WIN32) || defined(__APPLE__)
    if (DFFlag::PersistenceCurlCookies)
    {
        cookieSharingPolicy = Aya::Http::CookieSharingMultipleProcessesWrite;
        if (isStudio)
        {
            cookieSharingPolicy |= Aya::Http::CookieSharingMultipleProcessesRead;
        }
    }
    else
        cookieSharingPolicy = Aya::Http::CookieSharingSingleProcessMultipleThreads;


#else
#error Unsupported platform.
#endif

#if defined(AYA_PLATFORM_DURANGO)
    Http::init(Http::XboxHttp, cookieSharingPolicy);
#elif defined(_WIN32)
    Http::init(Http::WinInet, cookieSharingPolicy);
#else
    Http::init(Http::WinHttp, cookieSharingPolicy);
#endif

    Profiling::init(false);
    static FactoryRegistrator registerFactoryObjects;

    GlobalAdvancedSettings::singleton();
    GameSettings::singleton();
    LuaSettings::singleton();
    DebugSettings::singleton();
    PhysicsSettings::singleton();

    // Initialize Block's static data as soon as possible
    // to make sure it doesn't get destroyed before other static
    // objects try to use it.
    Block::init();
}

void Game::globalExit()
{
    //
}

void Game::setupDataModel(const std::string& baseUrl)
{
    dataModel->create<ScriptInformationProvider>()->setAssetUrl(baseUrl + "/asset/");

    ContentProvider* contentProvider = dataModel->create<ContentProvider>();
    contentProvider->setBaseUrl(baseUrl);
    contentProvider->setInstanceName(GetInstanceName());
    contentProvider->setInstanceCurrency(GetInstanceCurrency());
    contentProvider->setInstanceMotd(GetInstanceMotd());

    dataModel->setGame(this);

    commonVerbs.reset(new CommonVerbs(dataModel.get()));

    // verb container will destroy
    verbs.push_back(new CameraPanLeftCommand(dataModel->getWorkspace()));
    verbs.push_back(new CameraPanRightCommand(dataModel->getWorkspace()));
    verbs.push_back(new CameraTiltUpCommand(dataModel->getWorkspace()));
    verbs.push_back(new CameraTiltDownCommand(dataModel->getWorkspace()));
    verbs.push_back(new CameraZoomInCommand(dataModel->getWorkspace()));
    verbs.push_back(new CameraZoomOutCommand(dataModel->getWorkspace()));
    verbs.push_back(new CameraCenterCommand(dataModel->getWorkspace()));
    verbs.push_back(new CameraZoomExtentsCommand(dataModel->getWorkspace()));
    verbs.push_back(new ToggleViewMode(dataModel.get()));

    // record settings
    UserInputService* uiService = dataModel->create<UserInputService>();
    bool touchEnabled = false;
    if (uiService)
    {
        switch (uiService->getPlatform())
        {
        case UserInputService::PLATFORM_XBOXONE:
        case UserInputService::PLATFORM_WINDOWS:
        case UserInputService::PLATFORM_OSX:
        case UserInputService::PLATFORM_NONE:
        default:
            touchEnabled = false;
            break;
        case UserInputService::PLATFORM_IOS:
        case UserInputService::PLATFORM_ANDROID:
            touchEnabled = true;
            break;
        }
    }
}

Game::Game(Verb* lockVerb, const char* baseUrl, bool shouldShowLoadingScreen, GameBasicSettings::VirtualVersion vv)
    : hasShutdown(false)
{
    dataModel = DataModel::createDataModel(true, lockVerb, shouldShowLoadingScreen, vv);
    dataModel->submitTask(boost::bind(&Game::setupDataModel, this, std::string(baseUrl)), DataModelJob::Write);
}

Game::~Game(void)
{
    shutdown();
}

void Game::doClearVerbs()
{
    if (!dataModel)
        return;

    Security::Impersonator impersonate(Security::COM);
    {
        commonVerbs.reset();

        std::vector<Verb*>::iterator iter = verbs.begin();
        std::vector<Verb*>::iterator end = verbs.end();
        while (iter != end)
        {
            delete *iter;
            ++iter;
        }
        verbs.clear();
    }
}

void Game::clearVerbs(bool needsLock)
{
    if (needsLock)
    {
        DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
        doClearVerbs();
    }
    else
        doClearVerbs();
}

template<class T>
void shutdownDM(shared_ptr<T>& dataModelToShutdown)
{
    if (!dataModelToShutdown)
        return;

    {
        // Ensure that all content is wiped out
        DataModel::closeDataModel(dataModelToShutdown);
    }

    // Now release the DataModel
    dataModelToShutdown.reset();
}

void Game::shutdown()
{
    if (hasShutdown)
        return;

    hasShutdown = true;

    clearVerbs(true);

    shutdownDM(dataModel);
}

bool Game::getSuppressNavKeys()
{
    bool suppress = false;

    if (dataModel)
        suppress = dataModel->getSuppressNavKeys();

    return suppress;
}

void Game::configurePlayer(Aya::Security::Identities identity, const std::string& params, int launchMode)
{
    gameConfigurer.reset(new PlayerConfigurer());
    gameConfigurer->configure(identity, dataModel.get(), params, launchMode);
}
} // namespace Aya