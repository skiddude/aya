#pragma once

#include "Base/ViewBase.hpp"
#include "Window.hpp"

namespace Aya
{

// Forward declarations
class FunctionMarshaller;
class Game;
struct OSContext;
class RenderJob;
class ViewBase;

namespace Tasks
{
class Sequence;
}

// Class responsible for the game view
class View
{
    OgreWindow* window;
public:
    View(OgreWidget* h);
    ~View();

    void setOgreWindow(OgreWindow* wnd) { window = wnd; }

    void AboutToShutdown();

    void Start(const boost::shared_ptr<Game>& game);
    void Stop();

    void ShowWindow();

    void CloseWindow();

    OgreWidget* GetHWnd() const
    {
        return static_cast<OgreWidget*>(context.hWnd);
    }

    // TODO: refactor verbs so this isn't needed
    ViewBase* GetGfxView() const
    {
        return view.get();
    }

    CRenderSettings::GraphicsMode GetLatchedGraphicsMode();

    boost::shared_ptr<DataModel> getDataModel();

    void onResize(int x, int y);

private:
    // View references, but doesn't own the game
    boost::shared_ptr<Game> game;

    // The OS context used by the view.
    OSContext context;

    // The view into the game world.
    boost::scoped_ptr<Aya::ViewBase> view;
    
    boost::shared_ptr<Tasks::Sequence> sequence;
    boost::shared_ptr<RenderJob> renderJob;
    FunctionMarshaller* marshaller;

    void initializeSizes();

    void bindWorkspace();
    void unbindWorkspace();

    void initializeView();
    void initializeInput();
    void resetScheduler();

    void initializeJobs();
    void RemoveJobs();
};

} // namespace Aya
