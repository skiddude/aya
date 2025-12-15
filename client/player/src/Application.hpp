#pragma once
#include "intrusive_ptr_target.hpp"

#include "DataModel/DataModel.hpp"
#include "atomic.hpp"
#include "boost/scoped_ptr.hpp"
#include "View.hpp"
#include "Document.hpp"

#include "GameVerbs.hpp"

#include "boost/program_options.hpp"

#include "Window.hpp"
#include "AppSettings.hpp"
namespace Aya
{

// more forward declarations
class Game;
class UserInput;
class RenderJob;
class LeaveGameVerb;
class ToggleFullscreenVerb;
class FunctionMarshaller;
class Document;
struct StandardOutMessage;
class View;

namespace Tasks
{
class Sequence;
}

class Application
{
    boost::scoped_ptr<ToggleFullscreenVerb> toggleFullscreenVerb;
    boost::scoped_ptr<LeaveGameVerb> leaveGameVerb;
    boost::scoped_ptr<Document> currentDocument;
    OgreWidget* mainWindow;
    FunctionMarshaller* marshaller;
    boost::scoped_ptr<View> mainView;
    LaunchMode launchMode;
    boost::program_options::variables_map vm;
    std::string joinScript;
    bool gameReady;

    void HandleSDLMessage();

public:
    enum RequestPlaceInfoResult
    {
        SUCCESS,
        FAILED,
        RETRY,
        GAME_FULL,
        USER_LEFT,
    };


    Application();

    void parseCommandLine(int argc, char** argv);

    bool isGameReady()
    {
        return gameReady;
    };

    RequestPlaceInfoResult requestPlaceInfo(const std::string url, std::string& authenticationUrl, std::string& ticket, std::string& scriptUrl) const;
    bool requestPlaceInfo(int placeId, std::string& authenticationUrl, std::string& ticket, std::string& scriptUrl) const;

    void initVerbs();
    void shutdownVerbs();
    void loadAppSettings();
    void sendInputEvent(shared_ptr<InputObject> obj);
    shared_ptr<DataModel> getDM();
    void Initialize(OgreWidget* window);
    void InitializeNewGame(HttpFuture& scriptResult, GameBasicSettings::VirtualVersion vv);
    void InitializeNewGame(GameBasicSettings::VirtualVersion vv);
    void InitializeNewEmptyGame();
    void SendScript(std::string script, GameBasicSettings::VirtualVersion virtualVersion);
    void onResize(int x, int y);
    void onDocumentStarted(bool isTeleport);

    void prepareToShutdown();
    void shutdown();

    boost::program_options::variables_map getVm()
    {
        return vm;
    }

    AppSettings* settings;
};
} // namespace Aya
