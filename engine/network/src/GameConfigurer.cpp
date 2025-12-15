#include "DataModel/GameBasicSettings.hpp"
#include "Players.hpp"

#include "GameConfigurer.hpp"

#include "DataModel/DataModel.hpp"

#include "DataModel/UserInputService.hpp"

#include "DataModel/Workspace.hpp"

#include "DataModel/PhysicsSettings.hpp"


#include "DataModel/DebugSettings.hpp"

#include "DataModel/ChangeHistory.hpp"

#include "DataModel/InsertService.hpp"


#include "DataModel/SocialService.hpp"

#include "DataModel/GamePassService.hpp"

#include "DataModel/MarketplaceService.hpp"

#include "DataModel/TimerService.hpp"

#include "DataModel/Visit.hpp"

#include "DataModel/DebugSettings.hpp"

#include "DataModel/GuiService.hpp"

#include "DataModel/Value.hpp"

#include "DataModel/PlayerGui.hpp"

#include "DataModel/HttpService.hpp"

#include "DataModel/Folder.hpp"

#include "DataModel/ScreenGui.hpp"

#include "DataModel/ContentProvider.hpp"

#include "DataModel/MegaCluster.hpp"

#include "DataModel/TeleportService.hpp"

#include "DataModel/HttpRbxApiService.hpp"

#include "Utility/AyaService.hpp"
#include "Utility/Statistics.hpp"

#include "Utility/RobloxServicesTools.hpp"
#include "Script/script.hpp"
#include "Script/ScriptContext.hpp"
#include "Script/ModuleScript.hpp"
#include "Script/CoreScript.hpp"
#include "Utility/StandardOut.hpp"
#include "Xml/WebParser.hpp"
#include "Utility/ScriptInformationProvider.hpp"


#include "Client.hpp"
#include "ClientReplicator.hpp"
#include "Marker.hpp"
#include "GamePerfMonitor.hpp"
#include "Players.hpp"

#include "Utility/rbxrandom.hpp"

#include <vector>

#include "StringConv.hpp"

#include "Script/LuaVM.hpp"

#include "boost/tokenizer.hpp"
#include "boost/algorithm/string.hpp"
#include "XStudioBuild.hpp"

FASTSTRINGVARIABLE(SocialServiceFriendUrl, "Game/LuaWebService/HandleSocialRequest.ashx?method=IsFriendsWith&playerid=%d&userid=%d")
FASTSTRINGVARIABLE(SocialServiceBestFriendUrl, "Game/LuaWebService/HandleSocialRequest.ashx?method=IsBestFriendsWith&playerid=%d&userid=%d")
FASTSTRINGVARIABLE(SocialServiceGroupUrl, "Game/LuaWebService/HandleSocialRequest.ashx?method=IsInGroup&playerid=%d&groupid=%d")
FASTSTRINGVARIABLE(SocialServiceGroupRankUrl, "Game/LuaWebService/HandleSocialRequest.ashx?method=GetGroupRank&playerid=%d&groupid=%d")
FASTSTRINGVARIABLE(SocialServiceGroupRoleUrl, "Game/LuaWebService/HandleSocialRequest.ashx?method=GetGroupRole&playerid=%d&groupid=%d")
FASTSTRINGVARIABLE(GamePassServicePlayerHasPassUrl, "Game/GamePass/GamePassHandler.ashx?Action=HasPass&UserID=%d&PassID=%d")
FASTSTRINGVARIABLE(MobileJoinRateFormatUrl, "Game/JoinRate.ashx?st=%d&i=%d&p=%d&c=%s&r=%s&d=%d&b=%d&platform=%s")
FASTFLAG(UseBuildGenericGameUrl)


FASTFLAGVARIABLE(ClientABTestingEnabled, true)
FASTFLAG(LegacyMessageBox)

DYNAMIC_FASTFLAG(UseR15Character)

using namespace Aya;

void GameConfigurer::parseArgs(const std::string& args)
{
    parameters = Aya::make_shared<const Reflection::ValueTable>();
    WebParser::parseJSONTable(args, parameters);
}

int GameConfigurer::getParamInt(const std::string& key)
{
    Aya::Reflection::ValueTable::const_iterator iter = parameters->find(key);
    if (iter != parameters->end())
        return iter->second.get<int>();

    return 0;
}

std::string GameConfigurer::getParamString(const std::string& key)
{
    Aya::Reflection::ValueTable::const_iterator iter = parameters->find(key);
    if (iter != parameters->end())
        return iter->second.get<std::string>();

    return std::string();
}

bool GameConfigurer::getParamBool(const std::string& key)
{
    Aya::Reflection::ValueTable::const_iterator iter = parameters->find(key);
    if (iter != parameters->end())
        return iter->second.get<bool>();

    return false;
}

void GameConfigurer::registerPlay(const std::string& key, int userId, int placeId)
{
    if (!getParamBool("CookieStoreEnabled"))
        return;
}

void GameConfigurer::setupUrls()
{
    std::string baseUrl = getParamString("BaseUrl");
    dataModel->create<InsertService>();

    if (FFlag::UseBuildGenericGameUrl)
    {
        SocialService* socialService = dataModel->create<SocialService>();
        socialService->setFriendUrl(BuildGenericGameUrl(baseUrl, FString::SocialServiceFriendUrl));
        socialService->setBestFriendUrl(BuildGenericGameUrl(baseUrl, FString::SocialServiceBestFriendUrl));
        socialService->setGroupUrl(BuildGenericGameUrl(baseUrl, FString::SocialServiceGroupUrl));
        socialService->setGroupRankUrl(BuildGenericGameUrl(baseUrl, FString::SocialServiceGroupRankUrl));
        socialService->setGroupRoleUrl(BuildGenericGameUrl(baseUrl, FString::SocialServiceGroupRoleUrl));
        dataModel->create<GamePassService>()->setPlayerHasPassUrl(BuildGenericGameUrl(baseUrl, FString::GamePassServicePlayerHasPassUrl));
    }
    else
    {
        SocialService* socialService = dataModel->create<SocialService>();
        socialService->setFriendUrl(baseUrl + FString::SocialServiceFriendUrl);
        socialService->setBestFriendUrl(baseUrl + FString::SocialServiceBestFriendUrl);
        socialService->setGroupUrl(baseUrl + FString::SocialServiceGroupUrl);
        socialService->setGroupRankUrl(baseUrl + FString::SocialServiceGroupRankUrl);
        socialService->setGroupRoleUrl(baseUrl + FString::SocialServiceGroupRoleUrl);
        dataModel->create<GamePassService>()->setPlayerHasPassUrl(baseUrl + FString::GamePassServicePlayerHasPassUrl);
    }
}

PlayerConfigurer::PlayerConfigurer()
    : testing(true)
    , isTouchDevice(false)
    , connectResolved(false)
    , connectionFailed(false)
    , loadResolved(false)
    , joinResolved(false)
    , playResolved(true)
    , waitingForCharacter(false)
    , startTime(0)
    , playStartTime(0)
    , launchMode(-1)
{
}

PlayerConfigurer::~PlayerConfigurer()
{
    for (auto& c : connections)
        c.disconnect();
}

void PlayerConfigurer::ifSeleniumThenSetCookie(const std::string& key, const std::string& value) {}

void PlayerConfigurer::showErrorWindow(const std::string& message, const std::string& errorType, const std::string& errorCategory)
{
    if (GuiService* gs = dataModel->create<GuiService>())
    {
        if (errorType != "Kick" || gs->getErrorMessage() == "")
            gs->setErrorMessage(message);
    }

    dataModel->setUiMessage(message);
}

static void ignoreResponse(std::string*, std::exception*) {}

void PlayerConfigurer::reportError(const std::string& error, const std::string& msg)
{
    StandardOut::singleton()->printf(MESSAGE_INFO, "***ERROR*** %s %s", error.c_str(), msg.c_str());
    if (!testing)
    {
        Aya::Visit* visit = dataModel->create<Aya::Visit>();
        visit->setUploadUrl("");
    }

    Network::Client* client = dataModel->create<Network::Client>();
    client->disconnect();

    if (TimerService* timer = dataModel->create<TimerService>())
    {
        std::string errorMsg = Aya::format("Error: %s", error.c_str());
        timer->delay(boost::bind(&PlayerConfigurer::showErrorWindow, this, errorMsg, msg, "Other"), 4.0);
    }
}

void PlayerConfigurer::reportCounter(const std::string& counterNamesCSV, bool blocking) {}

void PlayerConfigurer::requestCharacter(shared_ptr<Network::Replicator> replicator, shared_ptr<bool> isWaiting)
{
    (*isWaiting) = false;

    dataModel->setUiMessage("Requesting character");

    if (!loadResolved)
    {
        loadResolved = true;
        double duration = G3D::System::time() - startTime;
    }

    try
    {
        replicator->requestCharacter();
        dataModel->setUiMessage("Waiting for character");
        waitingForCharacter = true;
    }
    catch (Aya::base_exception& e)
    {
        reportError(e.what(), "W4C");
    }
}

void periodicallyZoomExtents(weak_ptr<DataModel> dmWeak, shared_ptr<bool> isWaiting)
{
    shared_ptr<DataModel> dm = dmWeak.lock();

    if (dm && *isWaiting)
    {
        dm->getWorkspace()->zoomToExtents();

        dm->create<TimerService>()->delay(boost::bind(&periodicallyZoomExtents, dmWeak, isWaiting), 0.5);
    }
}

void PlayerConfigurer::onGameClose()
{

    //
}

void PlayerConfigurer::onDisconnection(const std::string& peer, bool lostConnection)
{
    if (peer.length() > 0)
    {
        if (lostConnection)
        {
            if (!connectionFailed)
                showErrorWindow("You have lost the connection to the game", "LostConnection", "LostConnection");
        }
        else
        {
            if (!connectionFailed)
            {
                showErrorWindow("This game has shut down", "Kick", "Kick");
                if (dataModel)
                {
                    Network::Players* players = dataModel->find<Network::Players>();
                    if (players)
                    {
                        Network::Player* p = players->getLocalPlayer();
                        if (p)
                        {
                            Instance* ps = p->findFirstChildByName("PlayerScripts");
                            if (ps)
                                ps->destroy();
                        }
                    }
                }
            }
        }
    }

    try
    {
        std::string url = Aya::format("%s&disconnect=true", getParamString("PingUrl").c_str());
#if defined(AYA_PLATFORM_DURANGO)
        HttpAsync::get(url);
#else
        dataModel->httpGet(url, true);
#endif
    }
    catch (Aya::base_exception&)
    {
        // don't care
    }
}

void PlayerConfigurer::onConnectionAccepted(std::string url, shared_ptr<Instance> replicator)
{
    connectResolved = true;

    shared_ptr<bool> waitingForMarker(new bool(true));

    try
    {
        if (!testing)
        {
            Aya::Visit* visit = dataModel->create<Aya::Visit>();
            visit->setPing(getParamString("PingUrl"), getParamInt("PingInterval"));
        }

        if (!getParamBool("GenerateTeleportJoin"))
            dataModel->setUiMessageBrickCount();
        else
            dataModel->setUiMessage("Teleporting...");

        boost::shared_ptr<Network::ClientReplicator> rep = Instance::fastSharedDynamicCast<Network::ClientReplicator>(replicator);

        connections.push_back(rep->disconnectionSignal.connect(boost::bind(&PlayerConfigurer::onDisconnection, this, _1, _2)));
        connections.push_back(rep->receivedGlobalsSignal.connect(boost::bind(&PlayerConfigurer::onReceivedGlobals, this)));

        connections.push_back(rep->gameLoadedSignal.connect(boost::bind(&PlayerConfigurer::onGameLoaded, this, waitingForMarker)));
    }
    catch (Aya::base_exception& e)
    {
        StandardOut::singleton()->printf(Aya::MESSAGE_ERROR, "onConnectionAccepted failed: %s", e.what());
        reportError(e.what(), "ConnectionAccepted");
    }

    dataModel->create<TimerService>()->delay(boost::bind(&periodicallyZoomExtents, weak_from(dataModel), waitingForMarker), 0);
}

void PlayerConfigurer::onConnectionFailed(const std::string& remoteAddress, int errorCode, const std::string& errorMsg)
{
    connectionFailed = true;
    std::string msg;
    if (FFlag::LegacyMessageBox)
    {
        msg = Aya::format("Failed to connect to the Game. (ID=%d)", errorCode);
    }
    else
    {
        msg = Aya::format("Failed to connect to the Game. (ID = %d: %s)", errorCode, errorMsg.c_str());
    }

    std::string errorType = Aya::format("ID%d", errorCode);
    showErrorWindow(msg, errorType, "Other");
}

void PlayerConfigurer::onConnectionRejected()
{
    showErrorWindow("This game is not available. Please try another", "WrongVersion", "WrongVersion");
}

void PlayerConfigurer::onReceivedGlobals() {}

void PlayerConfigurer::onGameLoaded(boost::shared_ptr<bool> isWaiting)
{
    *isWaiting = false;

    if (!loadResolved)
    {
        loadResolved = true;
        double duration = G3D::System::time() - startTime;
    }

    try
    {
        dataModel->setUiMessage("Waiting for character");
        waitingForCharacter = true;

        // see if our virtualversion is within bounds for the place
        StarterPlayerService* sps = dataModel->find<StarterPlayerService>();
        GameBasicSettings::VirtualVersion placeMinVersion = static_cast<GameBasicSettings::VirtualVersion>(sps->getMinVirtualVersion());
        GameBasicSettings::VirtualVersion placeMaxVersion = static_cast<GameBasicSettings::VirtualVersion>(sps->getMaxVirtualVersion());
        GameBasicSettings::VirtualVersion clientVersion = Aya::GameBasicSettings::singleton().getVirtualVersion();

        if (clientVersion < placeMinVersion)
        {
            clientVersion = placeMinVersion;
            Aya::GameBasicSettings::singleton().setVirtualVersionInternal(clientVersion);
        }
        else if (clientVersion > placeMaxVersion)
        {
            clientVersion = placeMaxVersion;
            Aya::GameBasicSettings::singleton().setVirtualVersionInternal(clientVersion);
        }

        if (Network::Players::frontendProcessing(dataModel))
        {
            std::string starterScript = "StarterScript";
            if (ScriptContext* scriptContext = dataModel->create<ScriptContext>())
            {
                scriptContext->addCoreScriptLocal(starterScript, shared_ptr<Instance>());
            }
        }
    }
    catch (Aya::base_exception& e)
    {
        reportError(e.what(), "W4C");
    }
}

void PlayerConfigurer::onPlayerIdled(double time)
{
    if (time > 1200)
    {
        showErrorWindow(Aya::format("You were disconnected for being idle %d minutes", (int)(time / 60)), "Idle", "Idle");

        if (Network::Client* client = dataModel->find<Network::Client>())
            client->disconnect();
    }
}

void PlayerConfigurer::setMessage(const std::string& msg)
{
    if (!getParamBool("GenerateTeleportJoin"))
        dataModel->setUiMessage(msg);
    else
        dataModel->setUiMessage("Teleporting...");
}

void PlayerConfigurer::configure(Aya::Security::Identities identity, DataModel* dm, const std::string& args, int lm)
{
    startTime = G3D::System::time();

    dataModel = dm;
    launchMode = lm;

    // Client-side A/B testing setup
    // urls to try:
    // https://api.gametest1.robloxlabs.com/users/get-experiment-enrollments
    // https://api.gametest1.robloxlabs.com/users/get-studio-experiment-enrollments

    std::string baseUrl = Aya::ContentProvider::getUnsecureApiBaseUrl(GetBaseURL());

    // begin fetching now
    Aya::HttpFuture abTest1, abTest2;
    if (FFlag::ClientABTestingEnabled)
    {
        abTest1 = FetchABTestDataAsync(baseUrl + "users/get-experiment-enrollments");
        abTest2 = FetchABTestDataAsync(baseUrl + "users/get-studio-experiment-enrollments");
    }

    Aya::Security::Impersonator impersonate(identity);

    parseArgs(args);

    // virtual version
    int virtualVersion = getParamInt("VirtualVersion");
    GameBasicSettings::VirtualVersion vv = static_cast<GameBasicSettings::VirtualVersion>(virtualVersion);
    dataModel->setInitialVersion(vv);

    Aya::GameBasicSettings::singleton().setVirtualVersionInternal(vv);

    Http::gameID = getParamString("GameId");

    testing = false; // (getParamString("ClientTicket").length() == 0);

    if (DFFlag::UseR15Character)
        dataModel->create<Network::Client>();

    dataModel->setPlaceID(getParamInt("PlaceId"), getParamBool("IsRobloxPlace"));
    int universeId = getParamInt("UniverseId");
    dataModel->setUniverseId(universeId);
    dataModel->create<HttpService>();
    isTouchDevice = dm->create<UserInputService>()->getTouchEnabled();

    StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "! Joining game '%s' place %d at %s", getParamString("GameId").c_str(),
        getParamInt("PlaceId"), getParamString("MachineAddress").c_str());

    connections.push_back(dataModel->closingSignal.connect(boost::bind(&PlayerConfigurer::onGameClose, this)));

    dataModel->create<ChangeHistoryService>()->setEnabled(false);
    dataModel->create<ContentProvider>()->setThreadPool(16);

    setupUrls();

    DataModel::CreatorType creatorType;
    Reflection::EnumDesc<DataModel::CreatorType>::singleton().convertToValue(getParamString("CreatorTypeEnum").c_str(), creatorType);
    dataModel->setCreatorID(getParamInt("CreatorId"), creatorType);

    Network::Players* players = dataModel->create<Network::Players>();
    Network::Players::ChatOption chatOption;
    Reflection::EnumDesc<Network::Players::ChatOption>::singleton().convertToValue(getParamString("ChatStyle").c_str(), chatOption);
    players->setChatOption(chatOption);

    if (!DFFlag::UseR15Character)
        dataModel->create<Network::Client>();
    dataModel->create<Visit>();

    ifSeleniumThenSetCookie("SeleniumTest1", "Started join script");

    try
    {
        setMessage("Connecting to Server");

        Network::Client* client = dm->create<Network::Client>();

        connections.push_back(client->connectionAcceptedSignal.connect(boost::bind(&PlayerConfigurer::onConnectionAccepted, this, _1, _2)));
        connections.push_back(client->connectionRejectedSignal.connect(boost::bind(&PlayerConfigurer::onConnectionRejected, this)));
        connections.push_back(client->connectionFailedSignal.connect(boost::bind(&PlayerConfigurer::onConnectionFailed, this, _1, _2, _3)));

        client->setTicket(getParamString("ClientTicket"));

        ifSeleniumThenSetCookie("SeleniumTest2", "Successfully connected to server");

        client->setGameSessionID(getParamString("SessionId"));

        shared_ptr<Network::Player> player = Instance::fastSharedDynamicCast<Network::Player>(
            client->playerConnect(getParamInt("UserId"), getParamString("MachineAddress"), getParamInt("ServerPort"), getParamInt("ClientPort"), -1));

        // prepare callback for when the Character appears
        playerChangedConnection = player->propertyChangedSignal.connect(boost::bind(&PlayerConfigurer::onPlayerChanged, this, _1));

        registerPlay(getParamString("CookieStoreFirstTimePlayKey"), getParamInt("UserId"), getParamInt("PlaceId"));
        if (TimerService* timer = dataModel->create<TimerService>())
        {
            timer->delay(boost::bind(&PlayerConfigurer::registerPlay, this, getParamString("CookieStoreFiveMinutePlayKey"), getParamInt("UserId"),
                             getParamInt("PlaceId")),
                60 * 5.0);
        }

        player->setSuperSafeChat(getParamBool("SuperSafeChat"));
        player->setUnder13(getParamBool("IsUnknownOrUnder13"));
        Network::Player::MembershipType membershipType;
        Reflection::EnumDesc<Network::Player::MembershipType>::singleton().convertToValue(getParamString("MembershipType").c_str(), membershipType);
        player->setMembershipType(membershipType);
        player->setAccountAge(getParamInt("AccountAge"));

        connections.push_back(player->idledSignal.connect(boost::bind(&PlayerConfigurer::onPlayerIdled, this, _1)));


        try
        {
            player->setName(getParamString("UserName"));
        }
        catch (Aya::base_exception&)
        {
            // don't care, happens when called from studio cmd bar
        }

        // I don't know why UserName needs to do this, so I'm copying this behavior for DisplayName -sorket
        try
        {
            player->setDisplayName(getParamString("DisplayName"));
        }
        catch (Aya::base_exception&)
        {
            // don't care, happens when called from studio cmd bar
        }

        player->setCharacterAppearance(getParamString("CharacterAppearance"));
        player->setFollowUserId(getParamInt("FollowUserId"));

        if (!testing)
        {
            Aya::Visit* visit = dataModel->create<Aya::Visit>();
            visit->setUploadUrl("");
        }
    }
    catch (Aya::base_exception& e)
    {
        reportError(e.what(), "CreatePlayer");
    }


    if (!testing)
    {
        gamePerfMonitor.reset(
            new GamePerfMonitor(getParamString("BaseUrl"), getParamString("GameId"), getParamInt("PlaceId"), getParamInt("UserId")));
        gamePerfMonitor->start(dataModel);
    }


    if (FFlag::ClientABTestingEnabled)
    {
        try
        {
            LoadABTestFromString(abTest1.get());
        }
        catch (const std::exception& e1)
        {
            try
            {
                LoadABTestFromString(abTest2.get());
            }
            catch (const std::exception& e2)
            {
                FASTLOG2(FLog::Error, "Failed to load AB test data from both URLS: [%s] [%s]", e1.what(), e2.what());
            }
        }
    }
}

void PlayerConfigurer::onPlayerChanged(const Reflection::PropertyDescriptor* propertyDescriptor)
{
    if (propertyDescriptor->name == "Character")
    {
        dataModel->clearUiMessage();
        waitingForCharacter = false;

        playerChangedConnection.disconnect();

        if (!joinResolved)
        {
            joinResolved = true;

            MegaClusterInstance* megaCluster = Instance::fastDynamicCast<MegaClusterInstance>(dataModel->getWorkspace()->getTerrain());
            bool hasTerrain = megaCluster && megaCluster->isAllocated();

            playStartTime = G3D::System::time();
            playResolved = false;
        }

        if (gamePerfMonitor)
        {
            // delay collecting network stats by 2 mins after character is resolved
            if (TimerService* timer = dataModel->create<TimerService>())
            {
                timer->delay(boost::bind(&GamePerfMonitor::setPostDiagStats, gamePerfMonitor, true), 2 * 60);
            }
        }
    }
}

bool isHidden(const boost::filesystem::path& p)
{
    boost::filesystem::path name = p.filename();
    if (name != ".." && name != "." && name.c_str()[0] == '.')
    {
        return true;
    }

    return false;
}


bool StudioConfigurer::findModulesAndLoad(
    const std::string& baseModulePath, const boost::filesystem::path& dir_path, boost::unordered_map<std::string, ProtectedString>& coreModules)
{
    if (!boost::filesystem::exists(dir_path))
        return false;

    boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
    for (boost::filesystem::directory_iterator itr(dir_path); itr != end_itr; ++itr)
    {
        if (is_directory(itr->status()) && !isHidden(itr->path()))
        {
            if (findModulesAndLoad(baseModulePath, itr->path(), coreModules))
                return true;
        }

        else if (is_regular_file(itr->status()))
        {
            boost::filesystem::path currentPath = itr->path();
            std::string filePath = currentPath.string();

            if (filePath.find(".lua") != std::string::npos)
            {
                size_t pathLocation = filePath.find(baseModulePath);
                if (pathLocation != std::string::npos)
                {
                    filePath = filePath.substr(pathLocation + baseModulePath.length() + 1);
                    filePath = filePath.substr(0, filePath.length() - 4);
                }

                // load module
                if (!filePath.empty())
                {
                    ProtectedString source = Aya::CoreScript::fetchSource("/Modules/" + filePath).get();
                    if (!source.empty())
                    {
                        coreModules[filePath] = source;
                    }
                }
            }
        }
    }
    return false;
}

shared_ptr<Aya::Folder> moduleScriptFolder;

void StudioConfigurer::loadCoreModules()
{
    Aya::CoreGuiService* coreGuiService = Aya::ServiceProvider::find<Aya::CoreGuiService>(dataModel);

    if (!coreGuiService)
    {
        return;
    }

    shared_ptr<Aya::ScreenGui> robloxScreenGui = coreGuiService->getRobloxScreenGui();
    if (!robloxScreenGui)
    {
        return;
    }

    moduleScriptFolder = Aya::Creatable<Instance>::create<Aya::Folder>();
    moduleScriptFolder->setName("Modules");
    moduleScriptFolder->setRobloxLocked(true);
    moduleScriptFolder->setParent(Aya::Instance::fastDynamicCast<Aya::Instance>(robloxScreenGui.get()));

    boost::unordered_map<std::string, ProtectedString> coreModules;

    if (LuaVM::canCompileScripts())
    {
        const std::string path = Aya::BaseScript::hasCoreScriptReplacements() ? Aya::BaseScript::adminScriptsPath + "/Modules"
                                                                              : ContentProvider::assetFolder() + "scripts/Modules";

        boost::filesystem::path filePath(path);
        if (!boost::filesystem::exists(filePath))
        {
            return;
        }

        findModulesAndLoad(filePath.string(), filePath, coreModules);
    }
    else
    {
        boost::unordered_map<std::string, std::string> byteCodeModules = LuaVM::getBytecodeCoreModules();
        for (boost::unordered_map<std::string, std::string>::iterator iter = byteCodeModules.begin(); iter != byteCodeModules.end(); ++iter)
        {
            coreModules[Aya::rot13((*iter).first)] = ProtectedString::fromBytecode((*iter).second);
        }
    }

    for (boost::unordered_map<std::string, ProtectedString>::iterator iter = coreModules.begin(); iter != coreModules.end(); ++iter)
    {
        shared_ptr<ModuleScript> moduleScript = Aya::Creatable<Instance>::create<ModuleScript>();

        std::string name = (*iter).first;
        shared_ptr<Aya::Folder> lastFolder = moduleScriptFolder;

        boost::filesystem::path namePath(name);
        if (namePath.has_parent_path())
        {
            const std::string delimiter = "\\";

            while (name.find("/") != std::string::npos)
            {
                name.replace(name.find("/"), 1, delimiter);
            }

            size_t pos = 0;
            int level = 0;
            std::string token;
            while ((pos = name.find(delimiter)) != std::string::npos)
            {
                token = name.substr(0, pos);
                name.erase(0, pos + delimiter.length());

                if (Instance* findFolder = moduleScriptFolder->findFirstChildByNameRecursive(token))
                {
                    if (Aya::Folder* folderInstance = Instance::fastDynamicCast<Aya::Folder>(findFolder))
                    {
                        lastFolder = shared_from(folderInstance);
                    }
                }
                else
                {
                    shared_ptr<Aya::Folder> newFolder = Aya::Creatable<Instance>::create<Aya::Folder>();
                    newFolder->setName(token);
                    newFolder->setRobloxLocked(true);
                    if (level == 0)
                    {
                        newFolder->setParent(moduleScriptFolder.get());
                    }
                    else
                    {
                        newFolder->setParent(lastFolder.get());
                    }
                    lastFolder = newFolder;
                }
                level++;
            }
        }

        moduleScript->setName(name);
        moduleScript->setSource((*iter).second);
        moduleScript->setRobloxLocked(true);

        moduleScript->setParent(lastFolder.get());
    }
}

void StudioConfigurer::configure(Aya::Security::Identities identity, DataModel* dm, const std::string& args, int launchMode)
{
    dataModel = dm;
    parseArgs(args);

    std::string baseUrl = getParamString("BaseUrl");

    dataModel->create<InsertService>();

    setupUrls();

    if (Network::Players::frontendProcessing(dataModel))
    {
        loadCoreModules();

        // Really ugly hack since getting the datamodel is a pain in the fucking ass
        Aya::GameBasicSettings::singleton().virtualVersionChangedSignal.connect(
            [&](const GameBasicSettings::VirtualVersion version)
            {
                if (version == GameBasicSettings::VERSION_2016 && !moduleScriptFolder->getChildren2())
                {
                    boost::unordered_map<std::string, ProtectedString> coreModules;

                    if (LuaVM::canCompileScripts())
                    {
                        const std::string path = Aya::BaseScript::hasCoreScriptReplacements() ? Aya::BaseScript::adminScriptsPath + "/Modules"
                                                                                              : ContentProvider::assetFolder() + "scripts/Modules";

                        boost::filesystem::path filePath(path);
                        if (!boost::filesystem::exists(filePath))
                        {
                            return;
                        }

                        findModulesAndLoad(filePath.string(), filePath, coreModules);
                    }
                    else
                    {
                        boost::unordered_map<std::string, std::string> byteCodeModules = LuaVM::getBytecodeCoreModules();
                        for (boost::unordered_map<std::string, std::string>::iterator iter = byteCodeModules.begin(); iter != byteCodeModules.end();
                            ++iter)
                        {
                            coreModules[Aya::rot13((*iter).first)] = ProtectedString::fromBytecode((*iter).second);
                        }
                    }

                    for (boost::unordered_map<std::string, ProtectedString>::iterator iter = coreModules.begin(); iter != coreModules.end(); ++iter)
                    {
                        shared_ptr<ModuleScript> moduleScript = Aya::Creatable<Instance>::create<ModuleScript>();

                        std::string name = (*iter).first;
                        shared_ptr<Aya::Folder> lastFolder = moduleScriptFolder;

                        boost::filesystem::path namePath(name);
                        if (namePath.has_parent_path())
                        {
                            const std::string delimiter = "\\";

                            while (name.find("/") != std::string::npos)
                            {
                                name.replace(name.find("/"), 1, delimiter);
                            }

                            size_t pos = 0;
                            int level = 0;
                            std::string token;
                            while ((pos = name.find(delimiter)) != std::string::npos)
                            {
                                token = name.substr(0, pos);
                                name.erase(0, pos + delimiter.length());

                                if (Instance* findFolder = moduleScriptFolder->findFirstChildByNameRecursive(token))
                                {
                                    if (Aya::Folder* folderInstance = Instance::fastDynamicCast<Aya::Folder>(findFolder))
                                    {
                                        lastFolder = shared_from(folderInstance);
                                    }
                                }
                                else
                                {
                                    shared_ptr<Aya::Folder> newFolder = Aya::Creatable<Instance>::create<Aya::Folder>();
                                    newFolder->setName(token);
                                    newFolder->setRobloxLocked(true);
                                    if (level == 0)
                                    {
                                        newFolder->setParent(moduleScriptFolder.get());
                                    }
                                    else
                                    {
                                        newFolder->setParent(lastFolder.get());
                                    }
                                    lastFolder = newFolder;
                                }
                                level++;
                            }
                        }

                        moduleScript->setName(name);
                        moduleScript->setSource((*iter).second);
                        moduleScript->setRobloxLocked(true);
                        Aya::StandardOut::singleton()->printf(MESSAGE_INFO, "loading %s", name.c_str());
                        moduleScript->setParent(lastFolder.get());
                    }
                }
            });
    }

#if defined(AYA_PLATFORM_DURANGO)
    if (ScriptContext* scriptContext = dataModel->create<ScriptContext>())
    {
        if (starterScript.empty())
            starterScript = "StarterScript";
        scriptContext->addCoreScriptLocal(starterScript, shared_ptr<Instance>());
        return;
    }
#endif

#if defined(AYA_STUDIO) && ENABLE_XBOX_STUDIO_BUILD
    if (ScriptContext* scriptContext = dataModel->create<ScriptContext>())
    {
        starterScript = "XStarterScript";
        scriptContext->addCoreScriptLocal(starterScript, shared_ptr<Instance>());
        return;
    }
#endif


    // this will be called in case of old play solo
    if (ScriptContext* scriptContext = dataModel->create<ScriptContext>())
    {
        AyaService* ayaService = ServiceProvider::create<AyaService>(dataModel);

        {
            starterScript = "StarterScript";

            if (Network::Players::backendProcessing(dataModel))
                starterScript = "ServerCoreScripts/" + starterScript;

            scriptContext->addCoreScriptLocal(starterScript, shared_ptr<Instance>());
        }
    }
}
