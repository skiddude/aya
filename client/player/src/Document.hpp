#pragma once
#include "DataModel/GameBasicSettings.hpp"
#include "intrusive_ptr_target.hpp"

#include "signal.hpp"

#include "Utility/HttpAsync.hpp"

#include "DataModel/DataModel.hpp"

#include "Window.hpp"

namespace Aya
{

enum LaunchMode
{
    Play,
    Play_Protocol,
    Build,
    Edit
};

// Forward declarations
class FunctionMarshaller;
class Game;
class View;
class PlayerConfigurer;

// Class responsible for the game state
class Document
{
public:
    Aya::signal<void(bool)> startedSignal;

    Document();
    ~Document();

    void Initialize(OgreWidget* hWnd, bool useChat, bool shouldShowLoadingScreen = true, bool shouldShowCorescripts = true,
        GameBasicSettings::VirtualVersion vv = GameBasicSettings::VERSION_2016);
    void StartResult(HttpFuture& scriptResult, const LaunchMode launchMode, bool isTelport);
    void Start(std::string script, const LaunchMode launchMode, bool isTelport); // TODO: be able to parse lua scripts
    void executeScript(std::string& data) const;

    void Shutdown();
    void SetUiMessage(const std::string& message);
    void PrepareShutdown(); // call before destroying the view
    void sendInputObject(shared_ptr<InputObject> obj);
    FunctionMarshaller* GetMarshaller() const;
    boost::shared_ptr<Game> getGame()
    {
        return game;
    }

private:
    FunctionMarshaller* marshaller;

    // The game to run.
    boost::shared_ptr<Game> game;

    // Executes the 'script' as part of the game initialization.
    void executeScript(HttpFuture& scriptResult, const LaunchMode launchMode) const;

    void configureDataModelServices(bool useChat, Aya::DataModel* dataModel);

    void dataModelDidRestart();
    void dataModelWillShutdown();
    void gameIsLoaded();
};

} // namespace Aya
