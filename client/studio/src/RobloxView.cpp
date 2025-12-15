


#include "RobloxView.hpp"

// Qt Headers
#include <QApplication>
#include <QCursor>
#include <QPixmapCache>
#include <QStringList>
#include <QMouseEvent>
#include <QEvent>
#include <QWindow>

// Roblox Headers
#include "Coordinator.hpp"

#include "CEvent.hpp"

#include "SystemUtil.hpp"

#include "Players.hpp"

#include "API.hpp"

#include "Script/script.hpp"
#include "DataModel/BaseRenderJob.hpp"

#include "DataModel/Workspace.hpp"

#include "DataModel/Game.hpp"

#include "DataModel/UserController.hpp"

#include "DataModel/ContentProvider.hpp"

#include "DataModel/ToolMouseCommand.hpp"

#include "DataModel/UserInputService.hpp"

#include "DataModel/Decal.hpp"

#include "DataModel/GameBasicSettings.hpp"

#include "Utility/FileSystem.hpp"

#include "Utility/StandardOut.hpp"

#include "Utility/Profiling.hpp"

#include "Utility/ContentId.hpp"

#include "Xml/XmlSerializer.hpp"
#include "Xml/Serializer.hpp"
#include "Base/ViewBase.hpp"
#include "Base/RenderSettings.hpp"
#include "Base/FrameRateManager.hpp"
#include "DataModel/RenderSettingsItem.hpp"
#include "Tool/ICancelableTool.hpp"

#include "Draw.hpp"

// Roblox Studio Headers
#include "RobloxMainWindow.hpp"
#include "UserInput.hpp"
#include "UserInputUtil.hpp"
#include "ogrewidget.hpp"
#include "FunctionMarshaller.hpp"
#include "RobloxCustomWidgets.hpp"
#include "RenderStatsItem.hpp"
#include "StudioUtilities.hpp"
#include "RobloxSettings.hpp"
#include "UpdateUIManager.hpp"
#include "AuthoringSettings.hpp"
#include "QtUtilities.hpp"
#include "RobloxMouseConfig.hpp"
#include "RobloxTreeWidget.hpp"
#include "CommonInsertWidget.hpp"

#include "RobloxDocManager.hpp"
#include "RobloxIDEDoc.hpp"

#include "Profiler.hpp"


LOGGROUP(HardwareMouse)
LOGGROUP(TaskSchedulerTiming)
LOGVARIABLE(RenderRequest, 0)

FASTFLAG(StudioSeparateActionByActivationMethod)
FASTFLAG(StudioRecordToolboxInsert)

FASTFLAGVARIABLE(DirectX11Enable, false)
FASTFLAG(UserBetterInertialScrolling)

FASTINTVARIABLE(StudioRightClickBuffer, 500)
FASTINTVARIABLE(StudioRightClickDelay, 150)
FASTINTVARIABLE(StudioRightClickMax, 700)

// should point to local file that is the default cursor for 3D window
static std::string defaultMouseCursorPath = "textures/advCursor-default.png";

class RobloxView::RenderJobCyclic : public Aya::BaseRenderJob
{
    shared_ptr<Aya::DataModel> m_pDataModel;
    RobloxView* m_pRbxView;
    QOgreWidget* m_pQOgreWidget;
    bool viewUpdateRequested;
    double m_timestamp;
    bool isBusySkipRender;

public:
    Aya::CEvent updateView;
    Aya::CEvent viewUpdated;

    RenderJobCyclic(RobloxView* pRbxView, shared_ptr<Aya::DataModel> pDataModel, QOgreWidget* pQOgreWidget)
        : Aya::BaseRenderJob(CRenderSettingsItem::singleton().getMinFrameRate(), CRenderSettingsItem::singleton().getMaxFrameRate(), pDataModel)
        , m_pRbxView(pRbxView)
        , m_pDataModel(pDataModel)
        , m_pQOgreWidget(pQOgreWidget)
        , viewUpdateRequested(false)
        , updateView(false)
        , viewUpdated(false)
        , isBusySkipRender(false)
    {
        cyclicExecutive = true;
        isAwake = false;
        m_timestamp = Aya::Time::nowFastSec();
    }

    ~RenderJobCyclic() {}

    double getTimestamp()
    {
        return m_timestamp;
    }

    void setIsBusySkipRender(bool value)
    {
        isBusySkipRender = value;
    }

    bool ogreWidgetHasFocus()
    {
        return m_pQOgreWidget->hasApplicationFocus();
    }

    bool tryJobAgain()
    {
        if (!isAwake)
        {
            if (!viewUpdateRequested)
            {
                m_timestamp = Aya::Time::nowFastSec();
                // do view update
                // Prevent Double Events
                QCoreApplication::removePostedEvents(m_pQOgreWidget, OGRE_VIEW_UPDATE);
                QApplication::postEvent(m_pQOgreWidget, new RobloxCustomEvent(OGRE_VIEW_UPDATE));

                viewUpdateRequested = true;
            }
            return true;
        }
        return false;
    }

    // Have to define to compile
    virtual Aya::Time::Interval sleepTime(const Stats&)
    {
        return Aya::Time::Interval(0);
    }

    virtual Aya::TaskScheduler::StepResult stepDataModelJob(const Stats&)
    {
        lastRenderTime = Aya::Time::now<Aya::Time::Fast>();

        shared_ptr<Aya::DataModel> pDataModel(m_pDataModel);
        if (!pDataModel)
            return Aya::TaskScheduler::Done;

        Aya::DataModel::scoped_write_request request(pDataModel.get());
        isAwake = false;

        // This logic has to be set by RobloxView::updateView
        // This is to prevent Deadlocks, and to not follow-through the
        // locking nature of RenderJob if the UIThread is busy.
        if (!isBusySkipRender)
        {
            // start view update
            updateView.Set();

            // wait till view is updated
            viewUpdated.Wait();
        }
        viewUpdateRequested = false;
        isBusySkipRender = false;
        return Aya::TaskScheduler::Stepped;
    }
};

// Job code has been taken from Client/Roblox/RobloxWnd.cpp
class RobloxView::RenderRequestJob : public Aya::DataModelJob
{
    RobloxView* m_pRbxView;
    boost::shared_ptr<Aya::Tasks::Coordinator> m_pSequence;
    shared_ptr<Aya::DataModel> m_pDataModel;
    QOgreWidget* m_pQOgreWidget;
    volatile bool m_bIsAwake;

    double m_timestamp;

public:
    RenderRequestJob(RobloxView* pRbxView, shared_ptr<Aya::DataModel> pDataModel, QOgreWidget* pQOgreWidget)
        : Aya::DataModelJob("RenderRequest", Aya::DataModelJob::Render, false, pDataModel, Aya::Time::Interval(0))
        , m_pRbxView(pRbxView)
        , m_pDataModel(pDataModel)
        , m_pQOgreWidget(pQOgreWidget)
        , m_bIsAwake(false)
    {
        cyclicExecutive = true;

        m_timestamp = Aya::Time::nowFastSec();

// Fixed Rally: DE2449 for Mouse Performance
// Created Rally: DE2557 for Rendering performance improvements
// KLUDGE - For Mac Mouse issue, especially with Trackpad
// Qt mouse events starts throttling when using trackpad
// tieing the render job with heartbeat somehow breaks the chain & gives qt chance to process mouse trackpad events
// The disadvantage of this approach is that the Render job is now maxed out at 30 fps.
// Need to find the root cause of why the problem gets resolved with tieing to heartbeat job
// Without this fix Mouse performance is 0.5 to 2 fps on CTF Model with Trackpad, unusable
// With this fix Mouse performance is 30 to 50 fps on CTF Model with Trackpad, amazing fast
#ifndef _WIN32
        m_pSequence.reset(new Aya::Tasks::Sequence());
        addCoordinator(m_pSequence);
        m_pDataModel->create<Aya::RunService>()->getHeartbeat()->addCoordinator(m_pSequence);
#endif

        // Force rendering and physics to happen in lock-step
        if (CRenderSettingsItem::singleton().isSynchronizedWithPhysics)
        {
#ifdef _WIN32
            m_pSequence.reset(new Aya::Tasks::Sequence());
            addCoordinator(m_pSequence);
#endif
            m_pDataModel->create<Aya::RunService>()->getPhysicsJob()->addCoordinator(m_pSequence);
        }
    }

    ~RenderRequestJob()
    {
        if (m_pSequence)
        {
            if (Aya::RunService* pRunService = m_pDataModel->find<Aya::RunService>())
            {
#ifndef _WIN32
                pRunService->getHeartbeat()->removeCoordinator(m_pSequence);
#endif
                if (CRenderSettingsItem::singleton().isSynchronizedWithPhysics)
                    pRunService->getPhysicsJob()->removeCoordinator(m_pSequence);
            }

            m_pSequence.reset();
        }
    }

    void wake()
    {
        m_bIsAwake = true;
        Aya::TaskScheduler::singleton().reschedule(shared_from_this());
    }

    virtual Aya::Time::Interval sleepTime(const Stats& stats)
    {
        if (m_bIsAwake)
        {
            float frameRate = Aya::DataModel::throttleAt30Fps ? CRenderSettingsItem::singleton().getMaxFrameRate() : 1000;

            // check for application focus
            if (UpdateUIManager::Instance().isBusy() || !m_pQOgreWidget->hasApplicationFocus())
                frameRate *= (100 - qBound(0, AuthoringSettings::singleton().renderThrottlePercentage, 100)) / 100.0f;

            return computeStandardSleepTime(stats, frameRate);
        }

        return Aya::Time::Interval::max();
    }

    virtual Job::Error error(const Stats& stats)
    {
        if (m_bIsAwake)
            return computeStandardError(stats, Aya::DataModel::throttleAt30Fps ? CRenderSettingsItem::singleton().getMinFrameRate() : 1000);

        return Job::Error();
    }

    double getTimestamp()
    {
        return m_timestamp;
    }

    virtual Aya::TaskScheduler::StepResult stepDataModelJob(const Stats&)
    {
        m_timestamp = Aya::Time::nowFastSec();

        m_bIsAwake = false;
        // do view update
        QApplication::postEvent(m_pQOgreWidget, new RobloxCustomEvent(OGRE_VIEW_UPDATE));

        return Aya::TaskScheduler::Stepped;
    }
};

class RobloxView::RenderJob : public Aya::DataModelJob
{
    boost::weak_ptr<Aya::DataModel> m_pDataModel;
    Aya::Time m_LastRenderTime;
    volatile bool m_bIsAwake;

public:
    RenderJob(RobloxView*, shared_ptr<Aya::DataModel> pDataModel)
        : Aya::DataModelJob("Render", Aya::DataModelJob::Render, false, pDataModel, Aya::Time::Interval(.02))
        , m_pDataModel(pDataModel)
        , m_bIsAwake(false)
        , updateView(false)
        , viewUpdated(false)
    {
        cyclicExecutive = true;
    }

    Aya::CEvent updateView;
    Aya::CEvent viewUpdated;

    void wake()
    {
        m_bIsAwake = true;
        Aya::TaskScheduler::singleton().reschedule(shared_from_this());
    }

    Aya::Time::Interval timeSinceLastRender() const
    {
        return Aya::Time::now<Aya::Time::Fast>() - m_LastRenderTime;
    }

    virtual Aya::Time::Interval sleepTime(const Stats&)
    {
        return m_bIsAwake ? Aya::Time::Interval(0) : Aya::Time::Interval::max();
    }

    virtual Job::Error error(const Stats& stats)
    {
        if (!m_bIsAwake)
            return Job::Error();

        Job::Error result;
        if (Aya::TaskScheduler::singleton().isCyclicExecutive() && cyclicExecutive)
        {
            result = computeStandardErrorCyclicExecutiveSleeping(stats, CRenderSettingsItem::singleton().getMaxFrameRate());
        }
        else
        {
            result = computeStandardError(stats, CRenderSettingsItem::singleton().getMinFrameRate());
        }
        return result;
    }

    virtual Aya::TaskScheduler::StepResult stepDataModelJob(const Stats&)
    {
        m_LastRenderTime = Aya::Time::now<Aya::Time::Fast>();

        shared_ptr<Aya::DataModel> pDataModel(m_pDataModel.lock());
        if (!pDataModel)
            return Aya::TaskScheduler::Done;

        Aya::DataModel::scoped_write_request request(pDataModel.get());
        m_bIsAwake = false;

        // start view update
        updateView.Set();

        // wait till view is updated
        viewUpdated.Wait();

        return Aya::TaskScheduler::Stepped;
    }
};

class RobloxView::WrapMouseJob : public Aya::DataModelJob
{
    RobloxView* m_pRbxView;
    Aya::DataModel* m_pDataModel;

public:
    WrapMouseJob(RobloxView* pRbxView, shared_ptr<Aya::DataModel> pDataModel)
        : Aya::DataModelJob("WrapMouse", Aya::DataModelJob::Write, true, pDataModel, Aya::Time::Interval(0))
        , m_pRbxView(pRbxView)
        , m_pDataModel(pDataModel.get())
    {
        cyclicExecutive = true;
    }

    virtual Aya::Time::Interval sleepTime(const Stats& stats)
    {
        return computeStandardSleepTime(stats, CRenderSettingsItem::singleton().getMaxFrameRate());
    }

    virtual Job::Error error(const Stats& stats)
    {
        Job::Error result = computeStandardError(stats, CRenderSettingsItem::singleton().getMaxFrameRate());
        return result;
    }

    virtual Aya::TaskScheduler::StepResult stepDataModelJob(const Stats&)
    {
        Aya::DataModel::scoped_write_request request(m_pDataModel);

        if (m_pRbxView)
            m_pRbxView->processWrapMouse();

        return Aya::TaskScheduler::Stepped;
    }
};

static Aya::ViewBase* createGameWindow(QOgreWidget* pQtWrapperWindow);

boost::shared_ptr<Aya::ViewBase> RobloxView::createGameView(QOgreWidget* pQtWrapperWindow)
{
    return boost::shared_ptr<Aya::ViewBase>(createGameWindow(pQtWrapperWindow));
}

RobloxView::RobloxView(QOgreWidget* pQtWrapperWidget, boost::shared_ptr<Aya::Game> game, boost::shared_ptr<Aya::ViewBase> view, bool vr)
    : m_pViewBase(view)
    , m_pDataModel(game->getDataModel())
    , m_pQtWrapperWidget(pQtWrapperWidget)
    , m_bIgnoreCursorChange(false)
    , m_hoverOverAccum(0)
    , m_hoverOverLastTime(Aya::Time::nowFast())
    , m_rightClickContextAllowed(true)
    , m_rightClickMenuPending(false)
    , m_DataModelHashNeeded(true)
    , m_mouseCursorHidden(false)
    , m_updateViewStepId(0)
    , m_previousCursorPos(0, 0)
    , m_previousCursorPosFraction(0, 0)
    , m_mousePressedPos(0, 0)
{
    // Aya::Log::current()->writeEntry(Aya::Log::Information,"\t\tRobloxView::RobloxView - start");

    // Aya::Log::current()->writeEntry(Aya::Log::Information,"\t\t\tUserInput");
    m_pUserInput.reset(new UserInput(this, game->getDataModel()));

    // Aya::Log::current()->writeEntry(Aya::Log::Information,"\t\t\tMarshaller");
    m_pMarshaller = Aya::FunctionMarshaller::GetWindow();

    // Aya::Log::current()->writeEntry(Aya::Log::Information,"\t\t\tProfilingSection");
    m_profilingSection.reset(new Aya::Profiling::CodeProfiler("Render"));

    // set bounds appropriately
    setBounds(m_pQtWrapperWidget->width(), m_pQtWrapperWidget->height());

    // set this to Qt widget. Internally this may depend on the roblox view being
    // mostly set up. In particular, this may depend on the window bounds being set
    // before this call is made.
    m_pQtWrapperWidget->setRobloxView(this);

    // CRenderSettingsItem::singleton().runProfiler(false);

    // Bind view to the Workspace
    // Aya::Log::current()->writeEntry(Aya::Log::Information,"\t\t\tBindToWorkspace");
    bindToWorkspace();

    // Enable or disable VR
    // m_pViewBase->enableVR(vr);

    if (RbxViewCyclicFlagsEnabled())
    {
        // Aya::Log::current()->writeEntry(Aya::Log::Information, "\t\t\tRenderJobCyclic");
        m_pRenderJobCyclic = shared_ptr<RenderJobCyclic>(new RenderJobCyclic(this, m_pDataModel, m_pQtWrapperWidget));
    }
    else
    {
        // Create the rendering jobs
        //  Aya::Log::current()->writeEntry(Aya::Log::Information,"\t\t\tRenderRequestJob");
        m_pRenderRequestJob = shared_ptr<RenderRequestJob>(new RenderRequestJob(this, m_pDataModel, m_pQtWrapperWidget));

        // Aya::Log::current()->writeEntry(Aya::Log::Information,"\t\t\tRenderJob");
        m_pRenderJob = shared_ptr<RenderJob>(new RenderJob(this, m_pDataModel));
    }

    // Aya::Log::current()->writeEntry(Aya::Log::Information,"\t\t\tWrapMouseJob");
    m_pWrapMouseJob = shared_ptr<WrapMouseJob>(new WrapMouseJob(this, m_pDataModel));

    // Aya::Log::current()->writeEntry(Aya::Log::Information,"\t\t\tConfigureStats");
    configureStats();

    if (RbxViewCyclicFlagsEnabled())
    {
        Aya::TaskScheduler::singleton().add(m_pRenderJobCyclic);
        Aya::TaskScheduler::singleton().add(m_pWrapMouseJob);
    }
    else
    {
        Aya::TaskScheduler::singleton().add(m_pRenderJob);
        Aya::TaskScheduler::singleton().add(m_pRenderRequestJob);
        Aya::TaskScheduler::singleton().add(m_pWrapMouseJob);
    }

    mouseCursorChangeConnection = m_pUserInput->cursorIdChangedSignal.connect(boost::bind(&RobloxView::handleMouseCursorChange, this));

    if (m_pDataModel && m_pDataModel.get())
    {
        if (Aya::UserInputService* userInputService = Aya::ServiceProvider::create<Aya::UserInputService>(m_pDataModel.get()))
        {
            handleMouseIconEnabledEvent(userInputService->getMouseIconEnabled());

            mouseCursorEnabledConnection =
                userInputService->mouseIconEnabledEvent.connect(boost::bind(&RobloxView::handleMouseIconEnabledEvent, this, _1));
            m_textBoxGainFocus = userInputService->textBoxGainFocus.connect(boost::bind(&RobloxView::handleTextBoxGainFocus, this));
            m_textBoxReleaseFocus = userInputService->textBoxReleaseFocus.connect(boost::bind(&RobloxView::handleTextBoxReleaseFocus, this));
        }
    }

    m_lastRightClickBlockingEventDate = QDateTime::currentDateTime();

    // Aya::Log::current()->writeEntry(Aya::Log::Information,"\t\tRobloxView::RobloxView - end");
}

RobloxView::~RobloxView(void)
{
    {
        Aya::DataModel::LegacyLock lock(m_pDataModel, Aya::DataModelJob::Write);

        if (m_renderStatsItem)
        {
            m_renderStatsItem->setParent(nullptr);
            m_renderStatsItem.reset();
        }
    }

    if (m_pRenderJob && !RbxViewCyclicFlagsEnabled())
    {
        m_pRenderJob->viewUpdated.Set();

        // viewUpdateJob can deadlock if we don't process the marshaller queue
        boost::function<void()> callback = boost::bind(&Aya::FunctionMarshaller::ProcessMessages, m_pMarshaller);
        Aya::TaskScheduler::singleton().removeBlocking(m_pRenderJob, callback);
        m_pRenderJob.reset();
    }

    if (m_pRenderRequestJob && !RbxViewCyclicFlagsEnabled())
    {
        boost::function<void()> callback = boost::bind(&Aya::FunctionMarshaller::ProcessMessages, m_pMarshaller);
        Aya::TaskScheduler::singleton().removeBlocking(m_pRenderRequestJob, callback);
        m_pRenderRequestJob.reset();
    }

    if (m_pRenderJobCyclic && RbxViewCyclicFlagsEnabled())
    {
        m_pRenderJobCyclic->viewUpdated.Set();
        boost::function<void()> callback = boost::bind(&Aya::FunctionMarshaller::ProcessMessages, m_pMarshaller);
        Aya::TaskScheduler::singleton().removeBlocking(m_pRenderJobCyclic, callback);
        m_pRenderJobCyclic.reset();
    }

    if (m_pWrapMouseJob)
    {
        boost::function<void()> callback = boost::bind(&Aya::FunctionMarshaller::ProcessMessages, m_pMarshaller);
        Aya::TaskScheduler::singleton().removeBlocking(m_pWrapMouseJob, callback);
        m_pWrapMouseJob.reset();
    }

    // RenderJob is sure to be completed at this point, since removeBlocking returned - but it might have marshalled
    // renderPerform asynchronously before exiting, which means that we might still have a callback that uses this view
    // in the marshaller queue.
    // This makes sure that all pending marshalled events are processed to avoid a use after free.
    m_pMarshaller->ProcessMessages();

    {
        Aya::DataModel::LegacyLock lock(m_pDataModel, Aya::DataModelJob::Write);

        Aya::ControllerService* service = Aya::ServiceProvider::create<Aya::ControllerService>(m_pDataModel.get());
        service->setHardwareDevice(nullptr);

        m_pViewBase->bindWorkspace(boost::shared_ptr<Aya::DataModel>());

        mouseCursorChangeConnection.disconnect();
        mouseCursorEnabledConnection.disconnect();

        m_pUserInput.reset();
    }

    Aya::FunctionMarshaller::ReleaseWindow(m_pMarshaller);

    // First destroy the view before closing the DataModel
    m_pViewBase.reset();

    // TODO - ROBLOXSTUDIO
    // Roblox::relinquishGame(game);
}

void RobloxView::onEvent_playerAdded(shared_ptr<Aya::Instance> playerAdded)
{
    if (Aya::Network::Player* localPlayer = Aya::Network::Players::findLocalPlayer(m_pDataModel.get()))
        if (Aya::Network::Player* thePlayerAdded = Aya::Instance::fastDynamicCast<Aya::Network::Player>(playerAdded.get()))
            if (localPlayer == thePlayerAdded)
                buildGui(true);
}

void RobloxView::bindToWorkspace()
{
    // we need write locks on all datamodels at this point (about to edit them outside of normal datamodel tasks)

    AYAASSERT(m_pDataModel->currentThreadHasWriteLock());

    // Note that this code needs to be thread-sensitive
    CRenderSettingsItem::singleton().runProfiler(false);

    m_pViewBase->bindWorkspace(m_pDataModel);

    // we check when a player is added to players, in case we need to load in the gui (someone might add a player via cmd line)
    if (!StudioUtilities::isAvatarMode())
        if (Aya::Network::Players* players = Aya::ServiceProvider::create<Aya::Network::Players>(m_pDataModel.get()))
            m_itemAddedToPlayersConnection =
                players->onDemandWrite()->childAddedSignal.connect(boost::bind(&RobloxView::onEvent_playerAdded, this, _1));

    Aya::ControllerService* service = Aya::ServiceProvider::create<Aya::ControllerService>(m_pDataModel.get());
    service->setHardwareDevice(m_pUserInput.get());
}

void RobloxView::buildGui(bool buildInGameGui)
{
    if (m_pViewBase)
        m_pViewBase->buildGui(buildInGameGui);
}

void RobloxView::configureStats()
{
    // For diagnostic view
    Aya::Stats::StatsService* pStats = Aya::ServiceProvider::create<Aya::Stats::StatsService>(m_pDataModel.get());
    m_renderStatsItem = RenderStatsItem::create(m_pViewBase->getRenderStats());
    m_renderStatsItem->setName("Render");
    m_renderStatsItem->setParent(pStats);
    m_renderStatsItem->createBoundChildItem(*m_profilingSection);
}

void RobloxView::setBounds(unsigned int width, unsigned int height)
{
    m_Width = width;
    m_Height = height;
    if (m_pViewBase)
        m_pViewBase->onResize(width, height);
}

void RobloxView::handleMouse(Aya::InputObject::UserInputType event, Aya::InputObject::UserInputState state, int x, int y, Aya::ModCode modifiers)
{
    if (!m_pUserInput)
        return;

    switch (event)
    {
    case Aya::InputObject::TYPE_MOUSEMOVEMENT:
    {
        G3D::Vector2 newPos(x, y);

        // determine delta
        int deltaX = x - m_previousCursorPos.x;
        int deltaY = y - m_previousCursorPos.y;

        // if we didn't move, no need to go any further
        if (deltaX == 0 && deltaY == 0)
        {
            return;
        }

        if (StudioUtilities::isAvatarMode())
        {
            float scale = Aya::GameBasicSettings::singleton().getMouseSensitivity();
            m_previousCursorPosFraction.x += deltaX * scale;
            m_previousCursorPosFraction.y += deltaY * scale;
            newPos = m_previousCursorPos + m_previousCursorPosFraction;
            deltaX = (int)(newPos.x) - (int)(m_previousCursorPos.x);
            deltaY = (int)(newPos.y) - (int)(m_previousCursorPos.y);
            m_previousCursorPosFraction.x -= (float)deltaX;
            m_previousCursorPosFraction.y -= (float)deltaY;
            setCursorPos(&newPos, m_pUserInput->isLeftMouseDown(), true, false);
            if (hasApplicationFocus() && !m_mouseCursorHidden)
                handleMouseIconEnabledEvent(false);
            else if (!hasApplicationFocus() && m_mouseCursorHidden)
                handleMouseIconEnabledEvent(true);
        }

        // remember new location
        m_previousCursorPos = newPos;
        m_rightClickContextAllowed = false;

        // Use the calculated deltas, not the position
        x = deltaX;
        y = deltaY;
        break;
    }

    case Aya::InputObject::TYPE_MOUSEBUTTON2:
    {
        if (state == Aya::InputObject::INPUT_STATE_BEGIN)
        {
            getCursorPos(&m_mousePressedPos);
            m_rightClickContextAllowed = true;

            if (m_rightClickMenuPending)
            {
                m_rightClickContextAllowed = false;
                m_lastRightClickBlockingEventDate = QDateTime::currentDateTime();
            }
            else
            {
                m_rightClickMouseDownDate = QDateTime::currentDateTime();
            }
        }
        else if (state == Aya::InputObject::INPUT_STATE_END)
        {
#ifdef _WIN32
            // we want to show the cursor inside of the 3d view when right mouse rotating camera stops
            m_bIgnoreCursorChange = false;
            setCursorImage();
#endif

            if (RobloxMouseConfig::singleton().canOpenContextMenu(event, state) && m_rightClickContextAllowed &&
                !m_pUserInput->areEditModeMovementKeysDown() && !Aya::Profiler::isCapturingMouseInput() && !StudioUtilities::isAvatarMode())
            {
                m_rightClickPosition = QPoint(x, y);
                handleContextMenu(x, y);
                return;
            }
        }
        break;
    }
    case Aya::InputObject::TYPE_MOUSEBUTTON1:
    {
        if ((state == Aya::InputObject::INPUT_STATE_BEGIN) && ((modifiers == Aya::AYA_KMOD_LCTRL) || (modifiers == Aya::AYA_KMOD_LSHIFT)))
        {
            if (Aya::UserInputService* userInputService = Aya::ServiceProvider::find<Aya::UserInputService>(m_pDataModel.get()))
            {
                // check is Ctrl or Shift is not pressed already
                Aya::KeyCode rbxKey = Aya::AYA_SDLK_UNKNOWN;
                if (!userInputService->isCtrlDown() && (modifiers == Aya::AYA_KMOD_LCTRL))
                    rbxKey = Aya::AYA_SDLK_LCTRL;
                else if (!userInputService->isShiftDown() && (modifiers == Aya::AYA_KMOD_LSHIFT))
                    rbxKey = Aya::AYA_SDLK_LSHIFT;

                // DE7421 - if Ctrl or Shift is not pressed already then make sure we post key press event first so multiple selections can be done
                if (rbxKey != Aya::AYA_SDLK_UNKNOWN)
                    m_pUserInput->PostUserInputMessage(Aya::InputObject::TYPE_KEYBOARD, Aya::InputObject::INPUT_STATE_BEGIN, rbxKey, modifiers, NULL);
            }
        }

        break;
    }
    default:
        break;
    }

    m_pUserInput->PostUserInputMessage(event, state, modifiers, MAKEXYLPARAM((uint)x, (uint)y));
}

void RobloxView::handleKey(Aya::InputObject::UserInputType event, Aya::InputObject::UserInputState state, Aya::KeyCode keyCode,
    Aya::ModCode modifiers, const char keyWithModifier, bool processed)
{
    m_pUserInput->PostUserInputMessage(event, state, keyCode, modifiers, keyWithModifier, processed);
}

void RobloxView::handleFocus(bool focus)
{
    if (m_pUserInput)
        m_pUserInput->PostUserInputMessage(
            Aya::InputObject::TYPE_FOCUS, focus ? Aya::InputObject::INPUT_STATE_BEGIN : Aya::InputObject::INPUT_STATE_END, 0, 0);
}

void RobloxView::resetKeyState()
{
    if (m_pUserInput)
        m_pUserInput->resetKeyState();
}

void RobloxView::handleMouseInside(bool inside)
{
    if (!m_pUserInput)
        return;

    if (inside)
    {
        m_pUserInput->onMouseInside();
        getCursorPos(&m_previousCursorPos);

        // need to immediately set cursor to what the game has
        m_bIgnoreCursorChange = false;
        setCursorImage();
    }
    else
    {
        // should unload any cursors we added (Qt keeps a stack of cursor changes otherwise, also some platforms need this to properly set mouse
        // icons)
        m_pQtWrapperWidget->unsetCursor();
        m_bIgnoreCursorChange = false;

        m_pUserInput->onMouseLeave();
    }
}

void RobloxView::handleTextBoxGainFocus()
{
    m_pQtWrapperWidget->setLuaTextBoxHasFocus(true);
}

void RobloxView::handleTextBoxReleaseFocus()
{
    m_pQtWrapperWidget->setLuaTextBoxHasFocus(false);
}

void RobloxView::handleMouseIconEnabledEvent(bool iconEnabled)
{
    m_mouseCursorHidden = !iconEnabled;

    if (m_mouseCursorHidden)
    {
        m_pQtWrapperWidget->unsetCursor();
        m_pQtWrapperWidget->setCursor(Qt::BlankCursor);
    }
    else
    {
        handleMouseCursorChange();
    }
}

void RobloxView::handleMouseCursorChange()
{
    if (!m_bIgnoreCursorChange && m_pUserInput)
        setCursorImage();
}

void RobloxView::handleScrollWheel(float delta, int x, int y)
{
    if (m_pUserInput)
    {
        // normalize the wheel input
        if (!FFlag::UserBetterInertialScrolling)
        {
            delta = G3D::clamp(delta, -1.0f, 1.0f);
        }
        m_pUserInput->PostUserInputMessage(
            Aya::InputObject::TYPE_MOUSEWHEEL, Aya::InputObject::INPUT_STATE_CHANGE, delta, MAKEXYLPARAM((uint)x, (uint)y));
    }
}

shared_ptr<const Aya::Instances> RobloxView::handleDropOperation(const QString& url, int x, int y, bool& mouseCommandInvoked)
{
    if (!m_pDataModel)
        throw std::runtime_error("Can't insert at this time");

    shared_ptr<Aya::Instances> instances(new Aya::Instances());

    try
    {
        Aya::ContentProvider* cp = nullptr;
        {
            Aya::DataModel::LegacyLock lock(m_pDataModel, Aya::DataModelJob::Write);
            cp = m_pDataModel.get()->create<Aya::ContentProvider>();
        }

        AYAASSERT(cp);

        // load instances in a different thread
        bool hasError = false;

        Aya::ContentId contentId(url.toStdString());

        UpdateUIManager::Instance().waitForLongProcess(
            "Inserting...", boost::bind(&RobloxView::loadContent, this, shared_from(cp), contentId, boost::ref(*instances), boost::ref(hasError)));

        if (hasError || instances->size() < 1)
            throw std::runtime_error("Dragged object cannot be inserted.");

        Aya::DataModel::LegacyLock lock(m_pDataModel, Aya::DataModelJob::Write);
        Aya::LuaSourceContainer::blockingLoadLinkedScriptsForInstances(cp, *instances);

        Aya::InsertMode insertMode = Aya::INSERT_TO_3D_VIEW;
        if ((instances->size() == 1) && Aya::Instance::fastSharedDynamicCast<Aya::Decal>(instances->at(0)))
        {
            Aya::Selection* sel = Aya::ServiceProvider::create<Aya::Selection>(m_pDataModel->getWorkspace());
            if ((sel->size() == 1) && Aya::Instance::fastSharedDynamicCast<Aya::PartInstance>(sel->front()))
                insertMode = Aya::INSERT_TO_TREE;
        }

        insertInstances(x, y, *instances, mouseCommandInvoked, insertMode);
    }
    // Catch any invalid asset url being dragged into Roblox Ogre Widget, this can come from different application
    catch (std::exception const& e)
    {
        Aya::StandardOut::singleton()->print(Aya::MESSAGE_ERROR, e.what());
    }

    return instances;
}

void RobloxView::handleDropOperation(boost::shared_ptr<Aya::Instance> pInstanceToInsert, int x, int y, bool& mouseCommandInvoked)
{
    if (!m_pDataModel || !pInstanceToInsert)
        return;

    try
    {
        shared_ptr<Aya::Instance> pClonedObjectToInsert = pInstanceToInsert->clone(Aya::EngineCreator);
        if (!pClonedObjectToInsert)
            return;

        Aya::Instances instances;
        instances.push_back(pClonedObjectToInsert);

        Aya::InsertMode insertMode = Aya::INSERT_TO_3D_VIEW;
        if (Aya::Instance::fastDynamicCast<Aya::Decal>(pClonedObjectToInsert.get()) != nullptr)
            insertMode = Aya::INSERT_TO_TREE;
        insertInstances(x, y, instances, mouseCommandInvoked, insertMode);
    }

    catch (std::exception const& e)
    {
        Aya::StandardOut::singleton()->print(Aya::MESSAGE_ERROR, e.what());
    }
}

void RobloxView::cancelDropOperation(bool resetMouseCommand)
{
    Aya::DataModel::LegacyLock lock(m_pDataModel, Aya::DataModelJob::Write);

    std::for_each(m_DraggedItems.begin(), m_DraggedItems.end(), boost::bind(&Aya::Instance::setParent, _1, (Aya::Instance*)nullptr));
    m_DraggedItems.clear();

    if (resetMouseCommand)
    {
        Aya::MouseCommand* pCurrentMouseCommand = m_pDataModel->getWorkspace()->getCurrentMouseCommand();
        Aya::ICancelableTool* pCancelableTool = nullptr;

        if ((pCurrentMouseCommand != nullptr) && (pCancelableTool = dynamic_cast<Aya::ICancelableTool*>(pCurrentMouseCommand)))
            m_pDataModel->getWorkspace()->setMouseCommand(pCancelableTool->onCancelOperation());
    }
}

void RobloxView::insertInstances(int x, int y, const Aya::Instances& instances, bool& mouseCommandInvoked, Aya::InsertMode insertMode)
{
    Aya::DataModel::LegacyLock lock(m_pDataModel, Aya::DataModelJob::Write);

    try
    {
        m_DraggedItems.clear();
        mouseCommandInvoked = false;

        std::for_each(instances.begin(), instances.end(), boost::bind(&InsertObjectWidget::SetObjectDefaultValues, _1));

        Aya::Instances remaining;
        {
            m_pDataModel->getWorkspace()->insertInstances(
                instances, m_pDataModel->getWorkspace(), insertMode, Aya::SUPPRESS_PROMPTS, nullptr, &remaining);
            if (!remaining.empty())
                mouseCommandInvoked = m_pDataModel->getWorkspace()->startPartDropDrag(remaining, true);

            if ((instances.size() == 1) && Aya::Instance::fastDynamicCast<Aya::Decal>(instances[0].get()))
            {
                mouseCommandInvoked = m_pDataModel->getWorkspace()->startDecalDrag(
                    Aya::Instance::fastDynamicCast<Aya::Decal>(instances[0].get()), Aya::INSERT_TO_3D_VIEW);
            }

            Aya::Selection* sel = Aya::ServiceProvider::create<Aya::Selection>(m_pDataModel->getWorkspace());
            boost::shared_ptr<Aya::RunService> runService = shared_from(m_pDataModel->create<Aya::RunService>());

            // If we're not playing a game then select the content
            if (runService->getRunState() == Aya::RS_STOPPED)
            {
                sel->setSelection(instances.begin(), instances.end());
            }
        }

        for (size_t ii = 0; ii < instances.size(); ++ii)
            m_DraggedItems.push_back(instances[ii]);
    }

    catch (std::exception&)
    {
        std::for_each(instances.begin(), instances.end(), boost::bind(&Aya::Instance::setParent, _1, (Aya::Instance*)nullptr));
        throw;
    }
}

void RobloxView::loadContent(boost::shared_ptr<Aya::ContentProvider> cp, Aya::ContentId contentId, Aya::Instances& instances, bool& hasError)
{
    try
    {
        Aya::DataModel::LegacyLock lock(m_pDataModel, Aya::DataModelJob::Write);
        std::auto_ptr<std::istream> stream(cp->getContent(contentId));
        Serializer().loadInstances(*stream, instances);
        hasError = false;
    }
    catch (...)
    {
        hasError = true;
    }
}

void RobloxView::processWrapMouse()
{
    if (m_pDataModel && m_pDataModel.get())
        if (Aya::UserInputService* userInputService = Aya::ServiceProvider::create<Aya::UserInputService>(m_pDataModel.get()))
            if (userInputService->getMouseWrapMode() == Aya::UserInputService::WRAP_HYBRID)
                m_pUserInput->ProcessUserInputMessage(Aya::InputObject::TYPE_NONE, Aya::InputObject::INPUT_STATE_NONE, 0, 0);
}

void RobloxView::getCursorPos(G3D::Vector2* pPos)
{
    if (!m_pQtWrapperWidget || !pPos)
        return;

    // map global position to local widget (this gives us logical pixels)
    QPoint viewPos = QCursor::pos();
    QPoint localPos = m_pQtWrapperWidget->mapFromGlobal(viewPos);

    // Convert from logical to physical pixels to match the coordinate space used throughout
    qreal dpr = m_pQtWrapperWidget->windowHandle() ? m_pQtWrapperWidget->windowHandle()->devicePixelRatio() : 1.0;
    pPos->x = localPos.x() * dpr;
    pPos->y = localPos.y() * dpr;
}

void RobloxView::setDefaultCursorImage()
{
    if (!Aya::MouseCommand::isAdvArrowToolEnabled())
    {
        m_pQtWrapperWidget->setCursor(Qt::BlankCursor);
        return;
    }

    m_pQtWrapperWidget->setCursor(Qt::ArrowCursor);
}

bool RobloxView::setCursorImageInternal(const Aya::ContentId& contentId, shared_ptr<const std::string> imageContent)
{
    if (!Aya::MouseCommand::isAdvArrowToolEnabled())
    {
        m_pQtWrapperWidget->setCursor(Qt::BlankCursor);
        return true;
    }

    QString qs = contentId.toString().c_str();
    QPixmap pix;
    if (!QPixmapCache::find(qs, &pix))
    {
        QImage image;
        image.loadFromData((uchar*)imageContent->c_str(), imageContent->size());

        // For advanced tool cursors (pixelated bitmaps), use nearest neighbor scaling
        Aya::StandardOut::singleton()->printf(Aya::MESSAGE_SYSTEM, "RobloxView::setCursorImageInternal loading cursor %s", contentId.c_str());
        if (contentId.toString().find("textures/adv") != std::string::npos)
        {
            QImage scaledImage = image.scaled(image.size(), Qt::KeepAspectRatio, Qt::FastTransformation);
            pix = QPixmap::fromImage(scaledImage);
        }
        else
        {
            pix = QPixmap::fromImage(image);
        }

        QPixmapCache::insert(qs, pix);
    }

    if (!pix.isNull()) // we have a valid image file, lets set the cursor
        m_pQtWrapperWidget->setCursor(QCursor(pix));

    return !pix.isNull();
}

void RobloxView::setCursorImage(bool forceShowCursor)
{
    if (!m_pQtWrapperWidget)
        return;

    if (m_mouseCursorHidden && !forceShowCursor)
    {
        m_pQtWrapperWidget->setCursor(Qt::BlankCursor);
        return;
    }

    Aya::ContentId cursorContent = m_pUserInput->getCurrentCursorId();

    if (cursorContent.toString().empty())
    {
        if (Aya::Network::Players::findLocalPlayer(m_pDataModel.get()) != nullptr)
        {
            Aya::ToolMouseCommand* pToolMouseCommand = dynamic_cast<Aya::ToolMouseCommand*>(m_pDataModel->getWorkspace()->getCurrentMouseCommand());
            if (pToolMouseCommand != nullptr)
                cursorContent = Aya::ContentId::fromAssets("textures/ArrowCursor.png");
        }
        else
        {
            setDefaultCursorImage();
            return;
        }
    }

    bool cursorIsSet = false;

    // first, unload our current cursor (we are guaranteed to set this later)
    m_pQtWrapperWidget->unsetCursor();

    // If we want the default cursor just set with Qt arrow cursor...
    if (!cursorContent.toString().compare("ayaasset://" + defaultMouseCursorPath))
    {
        setDefaultCursorImage();
        cursorIsSet = true;
    }
    else
    {
        if (m_pDataModel)
        {
            if (Aya::DataModel* dm = m_pDataModel.get())
            {
                shared_ptr<const std::string> imageContent;
                try
                {
                    imageContent = Aya::ServiceProvider::create<Aya::ContentProvider>(dm)->requestContentString(
                        cursorContent, Aya::ContentProvider::PRIORITY_MFC);
                }
                catch (const std::exception& e)
                {
                    FASTLOG2(FLog::HardwareMouse, "RobloxView::setCursorImage failed to load cursorContent %s because %s", cursorContent.c_str(),
                        e.what());

                    // cursor load failed (either from web or local asset)
                    // make sure we still set cursor to something
                    setDefaultCursorImage();
                    return;
                }

                if (!imageContent || imageContent->empty() || !setCursorImageInternal(cursorContent, imageContent))
                {
                    setDefaultCursorImage();
                }

                cursorIsSet = true;
            }
        }
    }

    if (!cursorIsSet)
        setDefaultCursorImage();
}

void RobloxView::setCursorPos(G3D::Vector2* pPos, bool isLMB, bool force, bool updatePos)
{
    if (!m_pQtWrapperWidget || !pPos || !isValidCursorPos(pPos, isLMB, force))
        return;

    // pPos is in physical pixels, but Qt cursor API expects logical pixels
    // Convert from physical to logical by dividing by DPI scale
    qreal dpr = m_pQtWrapperWidget->windowHandle() ? m_pQtWrapperWidget->windowHandle()->devicePixelRatio() : 1.0;
    QPoint localPos(static_cast<int>(pPos->x / dpr + 0.5), static_cast<int>(pPos->y / dpr + 0.5));
    QPoint globalPos = m_pQtWrapperWidget->mapToGlobal(localPos);

    // now set the position
    QCursor::setPos(globalPos);

    // Update previous position to match what Qt will actually report back
    // This prevents delta calculation errors due to DPI rounding
    if (updatePos)
    {
        // Calculate what physical position Qt will actually report back after rounding
        m_previousCursorPos.x = localPos.x() * dpr;
        m_previousCursorPos.y = localPos.y() * dpr;
    }
}

bool RobloxView::hasApplicationFocus()
{
    if (m_pQtWrapperWidget)
        return m_pQtWrapperWidget->hasApplicationFocus();
    return false;
}

bool RobloxView::isValidCursorPos(G3D::Vector2* pPos, bool isLMB, bool force)
{
    if (force)
        return true;

    if (isLMB)
    {
        // cursorPos is in logical pixels, but pPos is in physical pixels
        // We need to scale cursorPos to physical pixels for comparison
        QPoint cursorPos = m_pQtWrapperWidget->mapFromGlobal(QCursor::pos());
        qreal dpr = m_pQtWrapperWidget->windowHandle() ? m_pQtWrapperWidget->windowHandle()->devicePixelRatio() : 1.0;
        int physicalX = static_cast<int>(cursorPos.x() * dpr);
        int physicalY = static_cast<int>(cursorPos.y() * dpr);

        // Check if physical pixel position is outside the widget bounds (also in physical pixels)
        int widgetWidth = static_cast<int>(m_pQtWrapperWidget->width() * dpr);
        int widgetHeight = static_cast<int>(m_pQtWrapperWidget->height() * dpr);
        return (physicalX < 0 || physicalX >= widgetWidth || physicalY < 0 || physicalY >= widgetHeight);
    }

    return (m_mousePressedPos == *pPos);
}

void RobloxView::requestUpdateView()
{
    if (m_pRenderRequestJob && !(RbxViewCyclicFlagsEnabled()))
    {
        m_pRenderRequestJob->wake();
    }
}

void RobloxView::doRender(const double timeStamp)
{
    m_pViewBase->renderPrepare(this);
}

void RobloxView::updateView()
{
    if ((!m_pRenderJob || !m_pRenderRequestJob) && (!(m_pRenderJobCyclic && RbxViewCyclicFlagsEnabled())))
        return;


    if (!RbxViewCyclicFlagsEnabled() &&
        (UpdateUIManager::Instance().getMainWindow().isMinimized() || UpdateUIManager::Instance().isBusy() || !m_pQtWrapperWidget->isVisible()))
    {
        return;
    }

    Aya::Profiling::Mark mark(*m_profilingSection, true, true);

    FASTLOG(FLog::RenderRequest, "Got render request, waking render job");

    bool cyclicExecutiveSkipsPrep = false;

    if (RbxViewCyclicFlagsEnabled())
    {
        // Keeps track of when to Skip frames for
        // Unfocused UI
        m_updateViewStepId++;

        cyclicExecutiveSkipsPrep =
            (UpdateUIManager::Instance().getMainWindow().isMinimized() || UpdateUIManager::Instance().isBusy() || !m_pQtWrapperWidget->isVisible());

        if (!m_pRenderJobCyclic->ogreWidgetHasFocus())
        {
            if (!(m_updateViewStepId / 4))
            {
                cyclicExecutiveSkipsPrep = true;
            }
        }

        if (m_updateViewStepId >= 4)
        {
            m_updateViewStepId = 0;
        }

        // Set the RenderJob to skip all the fancy shmancy Locking and Waiting
        if (cyclicExecutiveSkipsPrep)
        {
            m_pRenderJobCyclic->setIsBusySkipRender(true);
        }
        // Wake the Rendering Job
        m_pRenderJobCyclic->wake();

        // We ignore the locking nature of the RenderJob if Busy
        if (!cyclicExecutiveSkipsPrep)
        {
            m_pRenderJobCyclic->updateView.Wait();
        }
    }
    else
    {
        m_pRenderJob->wake();
        // block the job, as we are in between of an update
        m_pRenderJob->updateView.Wait();
    }

    if (!m_pViewBase.get() || !m_pDataModel)
    {
        if (RbxViewCyclicFlagsEnabled())
        {
            if (!cyclicExecutiveSkipsPrep)
            {
                m_pRenderJobCyclic->viewUpdated.Set();
            }
        }
        else
        {
            m_pRenderJob->viewUpdated.Set();
        }
        return;
    }

    FASTLOG(FLog::RenderRequest, "Job has started, proceeding");

    // update hover over animation, hover color, and selection color
    updateHoverOver();

    Aya::Profiling::CodeProfiler* renderCodeProfiler = m_pViewBase->getRenderStats().cpuRenderTotal.get();
    {
        Aya::Profiling::Mark mark(*renderCodeProfiler, true, true);
        {
            bool isViewUpdated = false;

            if (!cyclicExecutiveSkipsPrep)
            {
                try
                {
                    Aya::FrameRateManager* pFRMgr = m_pViewBase->getFrameRateManager();
                    if (pFRMgr)
                    {
                        pFRMgr->configureFrameRateManager(CRenderSettingsItem::singleton().getFrameRateManagerMode(), true); // hasCharacter);
                    }

                    // RenderUpdate phase
                    {
                        Aya::DataModel::scoped_write_transfer request(m_pDataModel.get());

                        // m_pViewBase->updateVR();

                        float secondsElapsed = m_pViewBase->getFrameRateManager()->GetFrameTimeStats().getLatest() / 1000.f;
                        m_pDataModel->renderStep(secondsElapsed);
                    }

                    double timeStamp;

                    if (RbxViewCyclicFlagsEnabled())
                    {
                        timeStamp = m_pRenderJobCyclic->getTimestamp();
                    }
                    else
                    {
                        timeStamp = m_pRenderRequestJob->getTimestamp();
                    }

                    doRender(timeStamp);

                    // done with the update, release job!
                    if (RbxViewCyclicFlagsEnabled())
                    {
                        m_pRenderJobCyclic->viewUpdated.Set();
                    }
                    else
                    {
                        m_pRenderJob->viewUpdated.Set();
                    }

                    isViewUpdated = true;

                    m_pViewBase->renderPerform(timeStamp);
                }

                catch (std::exception& e)
                {
                    Aya::StandardOut::singleton()->print(Aya::MESSAGE_ERROR, e.what());
                }
                catch (...)
                {
                    Aya::StandardOut::singleton()->print(Aya::MESSAGE_ERROR, "Unknown exception occurred during view update");
                }
            }

            if (!isViewUpdated)
            {
                if (RbxViewCyclicFlagsEnabled())
                {
                    if (!cyclicExecutiveSkipsPrep)
                    {
                        m_pRenderJobCyclic->viewUpdated.Set();
                    }
                }
                else
                {
                    m_pRenderJob->viewUpdated.Set();
                }
            }
            // TODO: add support for "isPaintMessageAware"
            requestUpdateView();
        }
    }

    // init dataModelHash
    if (m_DataModelHashNeeded)
    {
        RobloxDocManager::Instance().getPlayDoc()->initializeDataModeHash();
        m_DataModelHashNeeded = false;
    }

    FASTLOG(FLog::RenderRequest, "Render request finished");
}

/**
 * Updates the hover over color and updates the selection color.
 *  If hover over animation is enabled, the hover over color will be changed.
 */
void RobloxView::updateHoverOver()
{
    // adjust the hover over and select colors
    G3D::Color4 hoverOverColor = AuthoringSettings::singleton().hoverOverColor;
    G3D::Color4 selectColor = AuthoringSettings::singleton().selectColor;

    // animation time
    const Aya::Time currentTime = Aya::Time::nowFast();
    const double deltaTime = (currentTime - m_hoverOverLastTime).msec();
    m_hoverOverAccum += deltaTime;

    // determine length of cycle
    int animateTime;
    switch (AuthoringSettings::singleton().hoverAnimateSpeed)
    {
    case AuthoringSettings::VerySlow:
        animateTime = 2000; // 2 seconds
        break;
    case AuthoringSettings::Slow:
        animateTime = 1000; // 1 seconds
        break;
    default:
    case AuthoringSettings::Medium:
        animateTime = 500; // 1/2 second
        break;
    case AuthoringSettings::Fast:
        animateTime = 250; // 1/4 second
        break;
    case AuthoringSettings::VeryFast:
        animateTime = 100; // 1/10 second
        break;
    }

    // adjust time accumulator to a reasonable value
    while (m_hoverOverAccum > animateTime * 2)
        m_hoverOverAccum -= animateTime * 2;
    m_hoverOverLastTime = currentTime;

    // adjust colors based on animation time
    if (AuthoringSettings::singleton().animateHoverOver)
    {
        if (m_hoverOverAccum < animateTime)
        {
            hoverOverColor = selectColor.lerp(hoverOverColor, m_hoverOverAccum / animateTime);
        }
        else
        {
            hoverOverColor = hoverOverColor.lerp(selectColor, (m_hoverOverAccum - animateTime) / animateTime);
        }
    }

    // set new colors
    Aya::Draw::setSelectColor(selectColor);
    Aya::Draw::setHoverOverColor(hoverOverColor);
}

// IMetric
double RobloxView::getMetricValue(const std::string& metric) const
{
    Aya::FrameRateManager* pFrameRateManager = m_pViewBase ? m_pViewBase->getFrameRateManager() : nullptr;

    if (metric == "Video Memory")
        return Aya::SystemUtil::getVideoMemory();

    if (RbxViewCyclicFlagsEnabled())
    {
        if (metric == "Render FPS")
            return m_pRenderJobCyclic.get() ? m_pRenderJobCyclic->averageStepsPerSecond() : 0.0;

        if (metric == "Render Duty")
            return m_pRenderJobCyclic.get() ? m_pRenderJobCyclic->averageDutyCycle() : 0.0;
    }
    else
    {
        if (metric == "Render FPS")
            return m_pRenderJob.get() ? m_pRenderJob->averageStepsPerSecond() : 0.0;

        if (metric == "Render Duty")
            return m_pRenderJob.get() ? m_pRenderJob->averageDutyCycle() : 0.0;
    }

    if (metric == "Render Nominal FPS")
        return pFrameRateManager ? 1000.0 / pFrameRateManager->GetRenderTimeAverage() : 0.0;

    if (!m_pViewBase)
        return 0.0;

    if (metric == "Delta Between Renders")
        return m_pViewBase->getMetricValue(metric);

    if (metric == "Total Render")
        return m_pViewBase->getMetricValue(metric);

    if (metric == "Present Time")
        return m_pViewBase->getMetricValue(metric);

    if (metric == "GPU Delay")
        return m_pViewBase->getMetricValue(metric);

    return 0.0;
}

std::string RobloxView::getMetric(const std::string& metric) const
{
    if (!m_pViewBase)
        return "No View";

    if (metric == "Graphics Mode")
        return Aya::Reflection::EnumDesc<Aya::CRenderSettings::GraphicsMode>::singleton().convertToString(
            CRenderSettingsItem::singleton().getLatchedGraphicsMode());

    if (metric == "VirtualVersion")
        return Aya::Reflection::EnumDesc<Aya::GameBasicSettings::VirtualVersion>::singleton().convertToString(
            Aya::GameBasicSettings::singleton().getVirtualVersion());

    Aya::FrameRateManager* pFrameRateManager = m_pViewBase ? m_pViewBase->getFrameRateManager() : 0;

    if (metric == "FRM")
        return (pFrameRateManager && pFrameRateManager->IsBlockCullingEnabled()) ? "On" : "Off";

    if (metric == "Anti-Aliasing")
        return (pFrameRateManager && pFrameRateManager->GetQualityLevel() >= 17) ? "On" : "Off";

    AYAASSERT(0);
    return "";
}

void RobloxView::handleContextMenu()
{
    m_rightClickMenuPending = false;
    if (!m_rightClickContextAllowed)
        return;

    int x = m_rightClickPosition.x();
    int y = m_rightClickPosition.y();

    handleContextMenu(x, y);
}

/**
 * Displays a context menu at a given screen location.
 *  If nothing is selected, a left click will be simulated in order to attempt to select
 *  an object under the cursor.
 */
void RobloxView::handleContextMenu(int x, int y)
{
    m_pQtWrapperWidget->setFocus(Qt::MouseFocusReason);

    // send the right click to the engine first
    m_pUserInput->ProcessUserInputMessage(Aya::InputObject::TYPE_MOUSEBUTTON2, Aya::InputObject::INPUT_STATE_END, 0, MAKEXYLPARAM((uint)x, (uint)y));

    Aya::Instance* parent;

    {
        Aya::DataModel::LegacyLock lock(m_pDataModel, Aya::DataModelJob::Write);

        // fire a left click to make sure whatever is under the cursor is selected
        m_pUserInput->ProcessUserInputMessage(
            Aya::InputObject::TYPE_MOUSEBUTTON1, Aya::InputObject::INPUT_STATE_BEGIN, 0, MAKEXYLPARAM((uint)x, (uint)y));
        m_pUserInput->ProcessUserInputMessage(
            Aya::InputObject::TYPE_MOUSEBUTTON1, Aya::InputObject::INPUT_STATE_END, 0, MAKEXYLPARAM((uint)x, (uint)y));

        // get parent object now that we might have selected something
        Aya::Selection* selection = Aya::ServiceProvider::create<Aya::Selection>(m_pDataModel.get());
        parent = selection->front().get();
    }

    if (!parent)
        parent = m_pDataModel->getWorkspace();

    RobloxMainWindow& mainWindow = UpdateUIManager::Instance().getMainWindow();
    QMenu menu(m_pQtWrapperWidget);

    menu.addAction(mainWindow.cutAction);
    menu.addAction(mainWindow.copyAction);
    menu.addAction(mainWindow.pasteAction);
    menu.addAction(mainWindow.pasteIntoAction);
    menu.addAction(mainWindow.duplicateSelectionAction);
    menu.addAction(mainWindow.deleteSelectedAction);
    menu.addSeparator();

    menu.addAction(mainWindow.groupSelectionAction);
    menu.addAction(mainWindow.ungroupSelectionAction);

    menu.addAction(mainWindow.unionSelectionAction);
    menu.addAction(mainWindow.negateSelectionAction);
    menu.addAction(mainWindow.separateSelectionAction);

    menu.addAction(mainWindow.selectChildrenAction);
    menu.addAction(mainWindow.zoomExtentsAction);
    menu.addSeparator();

    menu.addAction(QIcon(QtUtilities::getPixmap(QString::fromStdString(GetAssetFolder()) + "/textures/ClassImages.PNG", 1)), // part icon
        "Insert Part", this, SLOT(onInsertPart()));

    QMenu* insertObjectMenu = InsertObjectWidget::createMenu(parent, this, SLOT(onInsertObject()));
    menu.addMenu(insertObjectMenu);

    menu.connect(&menu, SIGNAL(aboutToShow()), &UpdateUIManager::Instance(), SLOT(onMenuShow()));
    menu.connect(&menu, SIGNAL(aboutToHide()), &UpdateUIManager::Instance(), SLOT(onMenuHide()));

    QPoint p = m_pQtWrapperWidget->mapToGlobal(QPoint(x, y));

    menu.exec(p);
}

bool RobloxView::rightClickContextMenuAllowed()
{
    if (m_rightClickContextAllowed && !m_pUserInput->areEditModeMovementKeysDown() && !Aya::Profiler::isCapturingMouseInput() &&
        !StudioUtilities::isAvatarMode() &&
        // if there is not a right click pending
        !m_rightClickMenuPending &&
        // checking if maximum hold time has been hit
        m_rightClickMouseDownDate.time().msecsTo(QDateTime::currentDateTime().time()) < FInt::StudioRightClickMax)
    {
        // return if buffer time is less than time between last right click blocking event and now
        return (FInt::StudioRightClickBuffer < m_lastRightClickBlockingEventDate.time().msecsTo(QDateTime::currentDateTime().time()));
    }
    return false;
}

/**
 * Callback for user clicking on insert part in context menu menu.
 */
void RobloxView::onInsertPart()
{
    InsertObjectWidget::InsertObject("Part", m_pDataModel, InsertObjectWidget::InsertMode_ContextMenu, &m_rightClickPosition);
}

/**
 * Callback for user clicking on insert basic object in context menu sub menu.
 */
void RobloxView::onInsertObject()
{
    QAction* action = static_cast<QAction*>(sender());
    QString className = action->text();
    InsertObjectWidget::InsertObject(className, m_pDataModel, InsertObjectWidget::InsertMode_ContextMenu, &m_rightClickPosition);
}

bool RobloxView::eventFilter(QObject* watched, QEvent* evt)
{
    if (evt->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(evt);
        if (mouseEvent->button() == Qt::RightButton)
        {
            QMenu* menu = static_cast<QMenu*>(watched);
            m_pQtWrapperWidget->setFocus(Qt::MouseFocusReason);
            menu->resize(0, 0);
            menu->move(0, 0);
            m_pUserInput->PostUserInputMessage(Aya::InputObject::TYPE_MOUSEBUTTON1, Aya::InputObject::INPUT_STATE_BEGIN, mouseEvent->modifiers(),
                MAKEXYLPARAM((uint)mouseEvent->x(), (uint)mouseEvent->y()));
            return true;
        }
    }
    else if (evt->type() == QEvent::MouseButtonRelease)
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(evt);
        if (mouseEvent->button() == Qt::RightButton)
        {
            QMenu* menu = static_cast<QMenu*>(watched);
            menu->hide();
            m_pUserInput->PostUserInputMessage(Aya::InputObject::TYPE_MOUSEBUTTON1, Aya::InputObject::INPUT_STATE_BEGIN, mouseEvent->modifiers(),
                MAKEXYLPARAM((uint)mouseEvent->x(), (uint)mouseEvent->y()));
            return true;
        }
    }
    return watched->event(evt);
}

bool RobloxView::RbxViewCyclicFlagsEnabled()
{
    return true;
    // return (Aya::TaskScheduler::singleton().isCyclicExecutive());
}

static Aya::ViewBase* createGameWindow(QOgreWidget* pQtWrapperWindow)
{
    if (!pQtWrapperWindow)
        return nullptr;

    // Ogre Plugin Initialization
    static boost::once_flag flag = BOOST_ONCE_INIT;
    boost::call_once(&Aya::ViewBase::InitPluginModules, flag);

    Aya::ViewBase* pRbxView = nullptr;

    std::vector<Aya::CRenderSettings::GraphicsMode> modes;

    const char* graphicsModeName;
    Aya::CRenderSettings::GraphicsMode graphicsMode = CRenderSettingsItem::singleton().getLatchedGraphicsMode();
    switch (graphicsMode)
    {
    case Aya::CRenderSettings::NoGraphics:
        break;
    case Aya::CRenderSettings::OpenGL:
        graphicsModeName = "gfx_gl";
        modes.push_back(graphicsMode);
        break;
    default:
        graphicsModeName = "gfx_all";
        modes.push_back(Aya::CRenderSettings::OpenGL);
        break;
    }

    // View creation parameters
    Aya::OSContext context;
    context.hWnd = reinterpret_cast<void*>(pQtWrapperWindow->winId());
    context.width = 800;
    context.height = 600;

    bool success = false;
    size_t modei = 0;
    while (!success && modei < modes.size())
    {
        graphicsMode = modes[modei];
        try
        {
            Aya::CRenderSettings& renderSettings = CRenderSettingsItem::singleton();

            // Create Ogre view
            pRbxView = Aya::ViewBase::CreateView(graphicsMode, &context, &renderSettings);
            success = true;
        }
        catch (std::exception& e)
        {
            Aya::StandardOut::singleton()->printf(Aya::MESSAGE_WARNING, "Mode %d failed: \"%s\"", graphicsMode, e.what());

            modei++;
            if (modei < modes.size())
            {
                Aya::StandardOut::singleton()->printf(Aya::MESSAGE_WARNING, "Trying mode %d...", modes[modei]);
            }
        }
    }


    if (modes.size() != 0 && !success)
    {
        RobloxSettings settings;
        settings.setValue("lastGFXMode", -1);
        throw std::runtime_error("Your graphics drivers seem to be too old for Aya to use.");
    }

    if (success && pRbxView)
    {
        RobloxSettings settings;
        settings.setValue("lastGFXMode", graphicsMode);

        // Initialize resources
        pRbxView->initResources();
    }

    // Aya::StandardOut::singleton()->printf(Aya::MESSAGE_WARNING, "Mode %d used", graphicsMode);

    return pRbxView;
}
