#include "DataModel/GameBasicSettings.hpp"


#include "Document.hpp"
#include "FunctionMarshaller.hpp"
#include "GameVerbs.hpp"
#include "Utility/AyaService.hpp"
#include "Script/ScriptContext.hpp"
#include "Utility/Statistics.hpp"


#include "DataModel/ContentProvider.hpp"

#include "DataModel/DataModel.hpp"

#include "DataModel/DebugSettings.hpp"

#include "DataModel/Game.hpp"

#include "DataModel/GuiService.hpp"

#include "DataModel/HackDefines.hpp"

#include "DataModel/UserInputService.hpp"

#include "DataModel/UserController.hpp"

#include "API.hpp"

#include "InitializationError.hpp"
#include "View.hpp"
#include <string>

LOGGROUP(PlayerShutdownLuaTimeoutSeconds)

namespace Aya
{

Document::Document()
    : marshaller(NULL)
{
}

Document::~Document() {}

void Document::StartResult(HttpFuture& scriptResult, const LaunchMode launchMode, bool isTeleport)
{
    printf("Document::startResult\n");

    startedSignal(isTeleport);
    executeScript(scriptResult, launchMode);
}

void Document::Start(std::string script, const LaunchMode launchMode, bool isTeleport)
{
    printf("Document::start\n");

    startedSignal(isTeleport);

    {
        Security::Impersonator impersonate(Security::COM);
        if (script[0] == '{')
            game->configurePlayer(Security::COM, script, launchMode);
        else
        {
            if (shared_ptr<DataModel> dm = game->getDataModel())
            {
                ScriptContext* context = dm->create<ScriptContext>();
                ProtectedString ps = ProtectedString::fromTrustedSource(script);
                context->executeInNewThread(Security::COM, ps, "Start Game");
            }
        }
    }
}

static void setUiMessageImpl(shared_ptr<DataModel> dm, const std::string& message)
{
    StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "setUiMessageImpl %s", message.c_str());

    if (message.length() > 0)
    {
        dm->setUiMessage(message);
    }
    else
    {
        StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "cleared setUiMessageImpl %s", message.c_str());
        dm->clearUiMessage();
    }

    if (GuiService* gs = dm->create<GuiService>())
        gs->setUiMessage(GuiService::UIMESSAGE_INFO, message);
}

void Document::SetUiMessage(const std::string& message)
{
    if (shared_ptr<DataModel> dm = game->getDataModel())
    {
        dm->submitTask(boost::bind(setUiMessageImpl, dm, message), DataModelJob::Write);
    }
}

void Document::PrepareShutdown()
{
    // give scripts a deadline to finish
    if (game && game->getDataModel())
        if (FLog::PlayerShutdownLuaTimeoutSeconds > 0)
            if (ScriptContext* scriptContext = game->getDataModel()->find<ScriptContext>())
                scriptContext->setTimeout(FLog::PlayerShutdownLuaTimeoutSeconds);
}


void Document::Shutdown()
{
    if (marshaller)
        FunctionMarshaller::ReleaseWindow(marshaller);

    if (game)
    {
        game->shutdown();
        game.reset();
    }
}



void Document::configureDataModelServices(bool useChat, Aya::DataModel* dataModel)
{
    if (!dataModel)
        return;

    DataModel::LegacyLock lock(dataModel, DataModelJob::Write);


    // Inform the UserInputService what kind of input we are providing (this may have to change if we use this with windows 8 touch devices)
    if (UserInputService* userInputService = dataModel->find<UserInputService>())
    {
        userInputService->setKeyboardEnabled(true);
        userInputService->setMouseEnabled(true);
    }
}

void Document::Initialize(
    OgreWidget* hWnd, bool useChat, bool shouldShowLoadingScreen, bool shouldShowCorescripts, GameBasicSettings::VirtualVersion vv)
{
    marshaller = FunctionMarshaller::GetWindow();
    game.reset(new Aya::SecurePlayerGame(NULL, GetBaseURL().c_str(), shouldShowLoadingScreen, true, vv));

    AyaService* ayaService = ServiceProvider::create<AyaService>(game->getDataModel().get());
    ayaService->shouldShowCorescripts = shouldShowCorescripts;

    configureDataModelServices(useChat, game->getDataModel().get());

    DataModel::LegacyLock lock(game->getDataModel().get(), DataModelJob::Write);

    if (FLog::PlayerShutdownLuaTimeoutSeconds > 0)
        game->getDataModel()->create<ScriptContext>();

    game->getDataModel().get()->gameLoadedSignal.connect(boost::bind(&Document::gameIsLoaded, this));
}

void Document::sendInputObject(shared_ptr<InputObject> object)
{
    if (Aya::UserInputService* userInputService = Aya::ServiceProvider::find<Aya::UserInputService>(game->getDataModel().get()))
        userInputService->fireInputEvent(object, NULL);
}
void Document::gameIsLoaded()
{
    // MainLogManager::getMainLogManager()->setGameLoaded();
}

// Executes the 'script' as part of the game initialization.
void Document::executeScript(HttpFuture& scriptResult, const LaunchMode launchMode) const
{
    shared_ptr<Aya::DataModel> dataModel = game->getDataModel();

    Security::Impersonator impersonate(Security::COM);
    std::string data;

    try
    {
        data = scriptResult.get();
        printf("%s\n", data.c_str());
    }
    catch (const std::exception& e)
    {
        std::string err = Aya::format("Exception occured in Document::executeScript: %s", e.what());
        // LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, err.c_str());

        if (GuiService* gs = dataModel->create<GuiService>())
            gs->setUiMessage(GuiService::UIMESSAGE_INFO, "Unable to join game. Please try again later.");

        return;
    }

    ProtectedString verifiedSource;
    try
    {
        verifiedSource = ProtectedString::fromTrustedSource(data);
        ContentProvider::verifyScriptSignature(verifiedSource, true);
    }
    catch (std::bad_alloc& e)
    {
        std::string err = Aya::format("Exception occured in Document::executeScript: %s", e.what());
        // LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, err.c_str());
        throw;
    }
    catch (std::exception& e)
    {
        std::string err = Aya::format("Exception occured in Document::executeScript: %s", e.what());
        // LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, err.c_str());
        // SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, AYA_PROJECT_NAME " error", err.c_str(), NULL);
    }

    if (dataModel->isClosed())
        return;

    int firstNewLineIndex = data.find("\r\n");
    if (data[firstNewLineIndex + 2] == '{')
    {
        game->configurePlayer(Security::COM, data.substr(firstNewLineIndex + 2), launchMode);
    }
    else
    {
        ScriptContext* context = dataModel->create<ScriptContext>();
        context->executeInNewThread(Security::COM, verifiedSource, "Start Game");
    }
}

void Document::executeScript(std::string& data) const
{
    shared_ptr<Aya::DataModel> dataModel = game->getDataModel();
    Security::Impersonator impersonate(Security::COM);

    if (dataModel->isClosed())
        return;

    ProtectedString verifiedSource;
    verifiedSource = ProtectedString::fromTrustedSource("\r\n" + data);
    ContentProvider::verifyScriptSignature(verifiedSource, true);

    ScriptContext* context = dataModel->create<ScriptContext>();
    context->executeInNewThread(Security::COM, verifiedSource, "Start Avatar View");
}

FunctionMarshaller* Document::GetMarshaller() const
{
    return marshaller;
}

} // namespace Aya
