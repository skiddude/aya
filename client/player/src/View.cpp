
#include "DataModel/DebugSettings.hpp"

#include "DataModel/GameBasicSettings.hpp"

#include "Script/ScriptContext.hpp"
#include "FunctionMarshaller.hpp"
#include <qguiapplication_platform.h>
#include <qpa/qplatformnativeinterface.h>
#include "Coordinator.hpp"

#include "RenderJob.hpp"
#include "DataModel/RenderSettingsItem.hpp"
#include "Utility/ScopedAssign.hpp"

#include "DataModel/Game.hpp"

#include "DataModel/UserController.hpp"

#include "InitializationError.hpp"
#include "View.hpp"
#include "format_string.hpp"


#include "Log.hpp"
#include "SystemUtil.hpp"
#include "Render/VisualEngine.hpp"

LOGGROUP(PlayerShutdownLuaTimeoutSeconds)
LOGGROUP(RobloxWndInit)
FASTFLAGVARIABLE(GraphicsReportingInitErrorsToGAEnabled, true)
FASTFLAGVARIABLE(UseNewAppBridgeInputWindows, false)
FASTFLAGVARIABLE(GraphicsEnableBGFX, false)
DYNAMIC_FASTFLAGVARIABLE(FullscreenRefocusingFix, false)

namespace
{
OgreWidget* SetFocusWrapper(OgreWidget* hwnd)
{
    return hwnd;
}
} // namespace

namespace Aya
{

static const char* kSavedScreenSizeRegistryKey = "HKEY_CURRENT_USER\\Software\\Kiseki\\Kiseki\\Settings\\RobloxPlayerV4WindowSizeAndPosition";

View::View(OgreWidget* h)
    : marshaller(NULL)
{
#ifdef __linux__
    QPlatformNativeInterface* native = QGuiApplication::platformNativeInterface();
    context.hWnd = native->nativeResourceForWindow("surface", h);
    context.display = native->nativeResourceForIntegration("display");
#else
    context.hWnd = (HWND)h->winId();
#endif
    context.width = h->width();
    context.height = h->height();
    marshaller = FunctionMarshaller::GetWindow();
    // window = dynamic_cast<OgreWindow*>(h->parentWidget());

    initializeView();
}

View::~View()
{
    AYAASSERT(!this->game && "Call Stop() before shutting down!");
    view.reset();

    if (marshaller)
        FunctionMarshaller::ReleaseWindow(marshaller);
}


void View::AboutToShutdown() {}

void View::initializeView()
{
    ViewBase::InitPluginModules();

    char* rgLogSuffix[5];
    memset(rgLogSuffix, 0, sizeof(rgLogSuffix));
    rgLogSuffix[(size_t)CRenderSettings::BGFX] = "gfx_bgfx";
    rgLogSuffix[(size_t)CRenderSettings::OpenGL] = "gfx_gl";

    std::vector<CRenderSettings::GraphicsMode> modes;

    Aya::CRenderSettings::GraphicsMode graphicsMode = CRenderSettingsItem::singleton().getLatchedGraphicsMode();

    // this is very experimental right now
    if (FFlag::GraphicsEnableBGFX)
        modes.push_back(CRenderSettings::BGFX);

    modes.push_back(CRenderSettings::OpenGL);

    std::string lastMessage;
    bool success = false;
    size_t modei = 0;
    printf("%i modes\n", modes.size());
    while (!success && modei < modes.size())
    {
        graphicsMode = modes[modei];
        try
        {
            view.reset(ViewBase::CreateView(graphicsMode, &context, &CRenderSettingsItem::singleton()));
            view->initResources();

            success = true;
        }
        catch (std::exception& e)
        {
            // d9mz - gutted dx, so we don't need to report this anymore
            printf("Mode %d failed: \"%s\"\n", graphicsMode, e.what());
            lastMessage += e.what();
            lastMessage += " | ";

            modei++;
            if (modei < modes.size())
            {
                printf("Trying mode %d...\n", modes[modei]);
            }
        }
    }

    if (!success)
    {
        throw initialization_error("Failed to initialize view");
    }

    AYAASSERT(view);
    initializeSizes();
}

void View::resetScheduler()
{
    TaskScheduler& taskScheduler = TaskScheduler::singleton();
    taskScheduler.add(renderJob);
}

void View::initializeJobs()
{
    shared_ptr<DataModel> dataModel = game->getDataModel();
    renderJob.reset(new RenderJob(this, marshaller, dataModel));
}

void View::initializeInput()
{
    /*userInput.reset(new UserInput(GetHWnd(), game, this));

    if (userInput)
    {
        DataModel::LegacyLock lock(game->getDataModel(), DataModelJob::Write);

        ControllerService* service = ServiceProvider::create<ControllerService>(game->getDataModel().get());
        service->setHardwareDevice(userInput.get());
    }*/
}

void View::RemoveJobs()
{
    if (renderJob)
    {
        boost::function<void()> callback = boost::bind(&FunctionMarshaller::ProcessMessages, marshaller);
        TaskScheduler::singleton().removeBlocking(renderJob, callback);
    }

    // RenderJob is sure to be completed at this point, since removeBlocking returned - but it might have marshalled
    // renderPerform asynchronously before exiting, which means that we might still have a callback that uses this view
    // in the marshaller queue.
    // This makes sure that all pending marshalled events are processed to avoid a use after free.
    marshaller->ProcessMessages();

    // All render processing is complete; it's safe to reset job pointers now
    renderJob.reset();
}

shared_ptr<DataModel> View::getDataModel()
{
    return game ? game->getDataModel() : shared_ptr<DataModel>();
}

CRenderSettings::GraphicsMode View::GetLatchedGraphicsMode()
{
    return CRenderSettingsItem::singleton().getLatchedGraphicsMode();
}

void View::initializeSizes()
{
    if (GetHWnd() == NULL)
    {
        // LogManager::ReportEvent(EVENTLOG_WARNING_TYPE, "Attempt to initialize monitor sizes without valid HWND");
        return;
    }
    G3D::Vector2int16 currentDisplaySize = G3D::Vector2int16(800, 600);

    G3D::Vector2int16 fullscreenSize, windowSize;

    Aya::CRenderSettings::ResolutionPreset preference = CRenderSettingsItem::singleton().getResolutionPreference();
    if (preference == Aya::CRenderSettings::ResolutionAuto)
    {
        fullscreenSize = currentDisplaySize;
    }
    else
    {
        const Aya::CRenderSettings::RESOLUTIONENTRY& res = CRenderSettingsItem::singleton().getResolutionPreset(preference);
        fullscreenSize.x = res.width;
        fullscreenSize.y = res.height;
    }

    // validate mode
    windowSize = fullscreenSize;
    windowSize.x = std::min((int)windowSize.x, (int)currentDisplaySize.x);
    windowSize.y = std::min((int)windowSize.y, (int)currentDisplaySize.y);

    CRenderSettingsItem::singleton().setWindowSize(windowSize);
    CRenderSettingsItem::singleton().setFullscreenSize(fullscreenSize);
}

void View::onResize(int x, int y)
{
    printf("%i, %i\n", x, y);
    view->onResize(x, y);
}

void View::unbindWorkspace()
{
    shared_ptr<DataModel> dm = getDataModel();
    DataModel::LegacyLock lock(dm, DataModelJob::Write);
    view->bindWorkspace(boost::shared_ptr<DataModel>());
}

void View::bindWorkspace()
{
    shared_ptr<DataModel> dm = getDataModel();
    DataModel::LegacyLock lock(dm, DataModelJob::Write);
    view->bindWorkspace(game->getDataModel());
    view->buildGui();
}

void View::Start(const shared_ptr<Game>& game)
{
    AYAASSERT(!this->game);
    this->game = game;

    bindWorkspace();
    initializeJobs();
    initializeInput(); // NOTE: have to do this here, Input requires datamodel access
    resetScheduler();

    // ensure keyboard is in focus (DE6272)
    // if (userInput)
    //     userInput->setKeyboardDesired(true);
}

void View::Stop()
{
    AYAASSERT(this->game);
    this->RemoveJobs();

    if (game && game->getDataModel())
        if (Aya::ControllerService* service = Aya::ServiceProvider::create<Aya::ControllerService>(game->getDataModel().get()))
            service->setHardwareDevice(NULL);

    // if (userInput)
    // {
    //     userInput->removeJobs();
    //     userInput.reset();
    // }

    unbindWorkspace();

    game.reset();
}

void View::CloseWindow()
{
    window->closeGame();
}


} // namespace Aya
