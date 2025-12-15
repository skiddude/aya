


#include "UpdateUIManager.hpp"

// Qt Headers
#include <QAction>
#include <QBitArray>
#include <QDateTime>
#include <QDockWidget>
#include <QScrollBar>
#include <QStatusBar>
#include <QLabel>
#include <QProgressBar>
#include <QWidgetAction>
#include <QRadioButton>
#include <QProxyStyle>

// Roblox Headers
#include "Log.hpp"

#include "TaskScheduler.hpp"

#include "DataModel/DataModel.hpp"

#include "DataModel/DebugSettings.hpp"

#include "DataModel/MouseCommand.hpp"

#include "DataModel/GameBasicSettings.hpp"


// Roblox Studio Headers
#include "AuthoringSettings.hpp"
#include "Roblox.hpp"
#include "RobloxCustomWidgets.hpp"
#include "RobloxDiagnosticsView.hpp"
#include "RobloxDocManager.hpp"
#include "RobloxFindWidget.hpp"
#include "RobloxGameExplorer.hpp"
#include "RobloxMainWindow.hpp"
#include "RobloxPropertyWidget.hpp"
#include "RobloxScriptReview.hpp"
#include "RobloxSettings.hpp"
#include "RobloxTaskScheduler.hpp"
#include "RobloxTextOutputWidget.hpp"
#include "RobloxToolBox.hpp"
#include "RobloxTreeWidget.hpp"
#include "CommonInsertWidget.hpp"
#include "RobloxIDEDoc.hpp"
#include "QtUtilities.hpp"
#include "AuthoringSettings.hpp"
#include "ScriptAnalysisWidget.hpp"

QString UpdateUIManager::defaultCommands = "fileOpenAction fileNewAction fileExitAction aboutRobloxAction actionStartPage actionFullScreen "
                                           "objectBrowserAction instanceDumpAction settingsAction openPluginsFolderAction "
                                           "fastLogDumpAction filePublishedProjectsAction shortcutHelpAction managePluginsAction switchWindowsMenu "
                                           "resetViewAction __qtn_Quick_Access_Button "
                                           "customizeQuickAccessAction viewCommandBarAction fileOpenRecentSavesAction ";
static QString kActionsThatBypassOnMenuShowStatusUpdates = "publishGameAction";
// Added following actions if we want to support theme modification at runtime
// Office2007Blue Office2007Black Office2007Silver Office2007Aqua Windows7Scenic Office2010Blue Office2010Silver Office2010Black Office2013White
// Office2013Gray
bool UpdateUIManager::m_longProcessInProgress = false;

FASTFLAG(StudioSeparateActionByActivationMethod)

UpdateUIManager& UpdateUIManager::Instance()
{
    static UpdateUIManager instance;
    return instance;
}

UpdateUIManager::UpdateUIManager()
    : m_pRobloxMainWindow(NULL)
    , m_PauseStatusBar(0)
    , m_isRunning(false)
    , m_dockRestoreStates(eDW_MAX)
    , m_BusyState(0)
{
}

UpdateUIManager::~UpdateUIManager() {}

void UpdateUIManager::saveDocksGeometry()
{
    RobloxSettings settings;
    for (int i = 0; i < eDW_MAX; i++)
    {
        EDockWindowID id = (EDockWindowID)i;
        QDockWidget* dock = m_DockWidgets[id];
        if (dock->widget())
            settings.setValue(dock->objectName() + "/Geometry", dock->widget()->saveGeometry());
    }
}

void UpdateUIManager::loadDocksGeometry()
{
    RobloxSettings settings;
    for (int i = 0; i < eDW_MAX; i++)
    {
        EDockWindowID id = (EDockWindowID)i;
        QDockWidget* dock = m_DockWidgets[id];
        QByteArray state = settings.value(dock->objectName() + "/Geometry").toByteArray();
        if (!state.isEmpty() && dock->widget())
            dock->widget()->restoreGeometry(state);

        // force the layout to be updated to fix issues initializing detached on a second monitor
        dock->layout()->update();
    }
}

QWidget* UpdateUIManager::getExplorerWidget()
{
    return m_ViewWidgets[eDW_OBJECT_EXPLORER];
}

class DockWidgetStyle : public QProxyStyle
{
    int pixelMetric(PixelMetric metric, const QStyleOption* option = 0, const QWidget* widget = 0) const
    {
        if (metric == PM_DockWidgetTitleMargin || metric == PM_DockWidgetTitleBarButtonMargin)
            return 2;
        return QProxyStyle::pixelMetric(metric, option, widget);
    }
};

void UpdateUIManager::initDocks()
{
    // TODO : convert this initialization stuff to a factory

    m_DockData[eDW_DIAGNOSTICS] = SDockData(UpdateUIManager::getAction("viewDiagnosticsAction"), false);

    m_DockData[eDW_TASK_SCHEDULER] = SDockData(UpdateUIManager::getAction("viewTaskSchedulerAction"), false);

    m_DockData[eDW_TOOLBOX] = SDockData(UpdateUIManager::getAction("viewToolboxAction"), true);


    m_DockData[eDW_BASIC_OBJECTS] = SDockData(UpdateUIManager::getAction("viewBasicObjectsAction"), false);

    m_DockData[eDW_SCRIPT_REVIEW] = SDockData(UpdateUIManager::getAction("viewScriptPerformanceAction"), false);

    m_DockData[eDW_OBJECT_EXPLORER] = SDockData(UpdateUIManager::getAction("viewObjectExplorerAction"), true, QSize(200, 300));

    m_DockData[eDW_PROPERTIES] = SDockData(UpdateUIManager::getAction("viewPropertiesAction"), false, QSize(200, 300));

    m_DockData[eDW_OUTPUT] = SDockData(UpdateUIManager::getAction("viewOutputWindowAction"), false);

    m_DockData[eDW_FIND] = SDockData(UpdateUIManager::getAction("viewFindResultsWindowAction"), false);

    m_DockData[eDW_GAME_EXPLORER] = SDockData(UpdateUIManager::getAction("gameExplorerAction"), false);

    m_DockData[eDW_SCRIPT_ANALYSIS] = SDockData(UpdateUIManager::getAction("viewScriptAnalysisAction"), false);

    m_DockWidgets[eDW_DIAGNOSTICS] = m_pRobloxMainWindow->diagnosticsDockWidget;
    m_DockWidgets[eDW_TASK_SCHEDULER] = m_pRobloxMainWindow->taskSchedulerDockWidget;
    m_DockWidgets[eDW_TOOLBOX] = m_pRobloxMainWindow->toolBoxDockWidget;
    m_DockWidgets[eDW_BASIC_OBJECTS] = m_pRobloxMainWindow->basicObjectsDockWidget;
    m_DockWidgets[eDW_SCRIPT_REVIEW] = m_pRobloxMainWindow->scriptReviewDockWidget;
    m_DockWidgets[eDW_OBJECT_EXPLORER] = m_pRobloxMainWindow->objectExplorer;
    m_DockWidgets[eDW_PROPERTIES] = m_pRobloxMainWindow->propertyBrowser;
    m_DockWidgets[eDW_OUTPUT] = m_pRobloxMainWindow->outputWindow;
    m_DockWidgets[eDW_FIND] = m_pRobloxMainWindow->findResultsWindow;
    m_DockWidgets[eDW_GAME_EXPLORER] = m_pRobloxMainWindow->gameExplorerDockWidget;
    m_DockWidgets[eDW_SCRIPT_ANALYSIS] = m_pRobloxMainWindow->scriptAnalysisDockWidget;

    m_ViewWidgets[eDW_DIAGNOSTICS] = new RobloxDiagnosticsView();
    m_ViewWidgets[eDW_TASK_SCHEDULER] = new RobloxTaskScheduler();
    m_ViewWidgets[eDW_TOOLBOX] = new RobloxToolBox();
    m_ViewWidgets[eDW_BASIC_OBJECTS] = new InsertObjectWidget(m_pRobloxMainWindow->basicObjectsDockWidget);
    m_ViewWidgets[eDW_SCRIPT_REVIEW] = new RobloxScriptReview();

    m_ViewWidgets[eDW_OBJECT_EXPLORER] = new RobloxExplorerWidget(m_pRobloxMainWindow->objectExplorer);

    m_ViewWidgets[eDW_PROPERTIES] = new RobloxPropertyWidget(m_pRobloxMainWindow->propertyBrowser);
    m_ViewWidgets[eDW_OUTPUT] = m_pRobloxMainWindow->getOutputWidget();
    m_ViewWidgets[eDW_FIND] = &RobloxFindWidget::singleton();
    m_ViewWidgets[eDW_GAME_EXPLORER] = new RobloxGameExplorer(m_pRobloxMainWindow->gameExplorerDockWidget);
    m_ViewWidgets[eDW_SCRIPT_ANALYSIS] = new ScriptAnalysisWidget(m_pRobloxMainWindow->scriptAnalysisDockWidget);

    for (int i = 0; i < eDW_MAX; ++i)
    {
        EDockWindowID id = (EDockWindowID)i;

        // Unfortunately we need to remove all the dockwidgets and re-add.  If we don't we end up with a bug (DE5372)
        m_pRobloxMainWindow->removeDockWidget(m_DockWidgets[id]);
        if (id == eDW_BASIC_OBJECTS || id == eDW_DIAGNOSTICS || id == eDW_SCRIPT_REVIEW || id == eDW_TASK_SCHEDULER || id == eDW_TOOLBOX ||
            id == eDW_GAME_EXPLORER)
        {
            m_pRobloxMainWindow->addDockWidget(Qt::LeftDockWidgetArea, m_DockWidgets[id]);
        }
        else if (id == eDW_PROPERTIES || id == eDW_OBJECT_EXPLORER)
        {
            m_pRobloxMainWindow->addDockWidget(Qt::RightDockWidgetArea, m_DockWidgets[id]);
        }
        else if (id == eDW_OUTPUT || id == eDW_FIND || id == eDW_SCRIPT_ANALYSIS)
        {
            m_pRobloxMainWindow->addDockWidget(Qt::BottomDockWidgetArea, m_DockWidgets[id]);
        }

        // make sure all the proper data and widgets were created
        AYAASSERT(m_DockData.contains(id));
        AYAASSERT(m_DockWidgets.contains(id));
        AYAASSERT(!m_DockWidgets[id]->objectName().isEmpty());
        AYAASSERT(m_ViewWidgets.contains(id));
        AYAASSERT(!m_DockData[id].UpdateAction->objectName().isEmpty());

        // hook up views to docks
        if (m_ViewWidgets.contains(id))
            m_DockWidgets[id]->setWidget(m_ViewWidgets[id]);

        m_actionDockMap.insert(m_DockData[id].UpdateAction->objectName(), id);

        // clear any old connections on this action to avoid duplicates
        QObject::disconnect(m_DockWidgets[id]->toggleViewAction(), nullptr, nullptr, nullptr);

        QDockWidget* dockWidget = m_DockWidgets[id];
        QAction* dockAct = dockWidget->toggleViewAction();
        QAction* uiAct = m_DockData[id].UpdateAction;

        // keep the UI action’s check state in sync with the dock’s visibility
        QObject::connect(dockWidget, &QDockWidget::visibilityChanged, uiAct, &QAction::setChecked, Qt::UniqueConnection);

        // clicking the UI action shows or hides the dock
        QObject::connect(uiAct, &QAction::toggled, dockWidget, &QDockWidget::setVisible, Qt::UniqueConnection);


        // Used to handle close event, etc.
        m_DockWidgets[id]->installEventFilter(this);

        // set our style
        m_DockWidgets[id]->setStyle(new DockWidgetStyle());
    }
}

void UpdateUIManager::init(RobloxMainWindow* pMainWindow)
{
    if (!pMainWindow)
        return;

    m_pRobloxMainWindow = pMainWindow;

#ifdef _WIN32
    if (!pMainWindow->isRibbonStyle())                                 // This fails for ribbon for some reason
        qApp->setStyleSheet("QScrollBar { scrollbar-contextmenu: 0}"); // remove scrollbar contextmenu from our application (this doesn't work for
                                                                       // QWebEngineView related scrollbars)
#endif

    initDocks();
    setupStatusBar();
    setupSlots();

    // Init our axisWidgets and 3d grid
    pMainWindow->toggleAxisWidgetAction->setChecked(get3DAxisEnabled());
    pMainWindow->toggle3DGridAction->setChecked(get3DGridEnabled());

    m_isRunning = true;

    /*
    // d9mz - what the hell is this
    #if WINVER >= 0x601
        // initialize Vista/Win7 task bar interface

        m_isTaskBarInitialized 		= false;
        m_isIndeterminateProgress 	= false;
        m_taskbar 					= NULL;

        HRESULT hr = ::CoCreateInstance(
            CLSID_TaskbarList,
            0,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&m_taskbar) );

        if ( hr == S_OK )
        {
            hr = m_taskbar->HrInit();

            if ( hr == S_OK )
            {
                m_isTaskBarInitialized = true;
            }
            else
            {
                m_taskbar->Release();
                m_taskbar = NULL;
            }
        }
    #endif*/

    // update status bar
    QTimer::singleShot(0, this, SLOT(updateStatusBar()));
}

void UpdateUIManager::setupStatusBar()
{
    // don't create border for the permanent widgets
    QStatusBar* pStatusBar = m_pRobloxMainWindow->statusBar();
    if (!pStatusBar)
        return;

    pStatusBar->setStyleSheet("QStatusBar::item { border: none; } ");

    // create labels to show different messages
    m_pTaskSchedulerSBLabel = new QLabel;
    m_pTimeSBLabel = new QLabel;

    m_pFPSSBLabel = new QLabel;

    m_pTaskSchedulerSBLabel->setMargin(5);
    m_pTimeSBLabel->setMargin(5);
    m_pFPSSBLabel->setMargin(5);

    // add labels to statusbar
    pStatusBar->addPermanentWidget(m_pTaskSchedulerSBLabel);
    pStatusBar->addPermanentWidget(m_pTimeSBLabel);
    pStatusBar->addPermanentWidget(m_pFPSSBLabel);
}

void UpdateUIManager::setMenubarEnabled(bool state)
{
    QList<QMenu*> childMenus = m_pRobloxMainWindow->findChildren<QMenu*>();
    for (int jj = 0; jj < childMenus.size(); ++jj)
    {
        if (state)
            connect(childMenus.at(jj), SIGNAL(aboutToShow()), this, SLOT(onMenuShow()));
        else
            disconnect(childMenus.at(jj), SIGNAL(aboutToShow()), this, SLOT(onMenuShow()));
    }
}

void UpdateUIManager::setupSlots()
{
    QList<QMenu*> childMenus = m_pRobloxMainWindow->findChildren<QMenu*>();
    for (int j = 0; j < childMenus.size(); ++j)
        connect(childMenus.at(j), SIGNAL(aboutToShow()), this, SLOT(onMenuShow()));

    connect(&m_pRobloxMainWindow->fillColorToolButton(), SIGNAL(frameShown()), this, SLOT(onMenuShow()));
    connect(&m_pRobloxMainWindow->materialToolButton(), SIGNAL(frameShown()), this, SLOT(onMenuShow()));
}

QString UpdateUIManager::getDockGeometryKeyName(EDockWindowID dockId) const
{
    return m_DockWidgets[dockId]->objectName() + "_geometry";
}

void UpdateUIManager::setDockVisibility(EDockWindowID dockId, bool visible)
{
    if (m_ViewWidgets.contains(dockId))
        m_ViewWidgets[dockId]->setVisible(visible);

    m_DockWidgets[dockId]->setVisible(visible);
}

/**
 * Sets up the view menu with the dock widget actions.
 *
 * @param insertBefore  location to add the actions in the menu
 */
void UpdateUIManager::setupDockWidgetsViewMenu(QAction& insertBefore)
{
    // Connect all dock widgets to their appropriate actions and add to the view menu
    for (int i = 0; i < eDW_MAX; ++i)
    {
        EDockWindowID id = (EDockWindowID)i;
        QAction& action = *m_DockData[id].UpdateAction;

        // TODO : show view icons in menu
        action.setIconVisibleInMenu(false);

        m_pRobloxMainWindow->viewToolsToolBar->addAction(&action);
        m_pRobloxMainWindow->menuView->insertAction(&insertBefore, &action);
    }
}

void UpdateUIManager::shutDown()
{
    m_isRunning = false;

    /*
    #if WINVER >= 0x601
        // delete Vista/Win7 task bar interface
        if ( m_isTaskBarInitialized )
            m_taskbar->Release();
    #endif*/
}

void UpdateUIManager::onMenuShow()
{
    IRobloxDoc* pDoc = RobloxDocManager::Instance().getCurrentDoc();
    if (!pDoc)
        return;

    const QObject* pSender = sender();
    if (!pSender)
        return;

    bool isTreeWidgetConextMenu = (pSender->parent() == m_pRobloxMainWindow->treeWidgetStack().currentWidget());
    IRobloxDoc* playDoc = RobloxDocManager::Instance().getPlayDoc();

    if (pSender->inherits("QMenu"))
    {
        QList<QAction*> menuActions = ((const QMenu*)pSender)->actions();
        QAction* pAction = NULL;
        bool checkedState = false, enabledState = false;
        for (int i = 0; i < menuActions.size(); ++i)
        {
            pAction = menuActions.at(i);

            if (!pAction)
                continue;

            QString objectName = pAction->objectName();
            if (objectName.isEmpty())
                continue;

            if (kActionsThatBypassOnMenuShowStatusUpdates.contains(objectName))
                continue;

            if (defaultCommands.contains(objectName))
            {
                pAction->setEnabled(true);
            }
            else if (isTreeWidgetConextMenu && playDoc && playDoc->actionState(objectName, enabledState, checkedState))
            {
                pAction->setEnabled(enabledState);
                pAction->setChecked(checkedState);
            }
            else if (pDoc->actionState(objectName, enabledState, checkedState))
            {
                pAction->setEnabled(enabledState);
                pAction->setChecked(checkedState);
            }
        }
    }

    if (isTreeWidgetConextMenu)
        pDoc->handleAction("actionCustomContextMenuShow");
}

void UpdateUIManager::onMenuHide()
{
    IRobloxDoc* pDoc = RobloxDocManager::Instance().getCurrentDoc();
    if (!pDoc)
        return;

    const QObject* pSender = sender();
    if (!pSender)
        return;

    if (!pSender->inherits("QMenu"))
        return;

    if (pSender->parent() == m_pRobloxMainWindow->treeWidgetStack().currentWidget())
        pDoc->handleAction("actionCustomContextMenuHide");
}

void UpdateUIManager::setDefaultApplicationState()
{
    if (m_pRobloxMainWindow == nullptr)
    {
        return;
    }

    QList<QAction*> allActions = m_pRobloxMainWindow->findChildren<QAction*>();
    QAction* pAction = NULL;

    for (int i = 0; i < allActions.size(); ++i)
    {
        pAction = allActions.at(i);
        if (!pAction || pAction->objectName().isEmpty() || defaultCommands.contains(pAction->objectName()) ||
            pAction->objectName() == "executeScriptAction" || getDockActionNames().contains(pAction->objectName())) // docks are always enabled
            continue;

        pAction->setEnabled(false);
        pAction->setChecked(false);
    }
}

void UpdateUIManager::updateToolBars()
{
    onToolBarsUpdate();
}

void UpdateUIManager::onToolBarsUpdate()
{
    updateToolbarTask();
}

void UpdateUIManager::updateToolbarTask()
{
    // update actions with shortcut, these need to be updated in both ribbon and menu
    updateAction(*m_pRobloxMainWindow->deleteSelectedAction);
    updateAction(*m_pRobloxMainWindow->selectAllAction);
    updateAction(*m_pRobloxMainWindow->pasteAction);
    updateAction(*m_pRobloxMainWindow->pasteIntoAction);

    updateAction(*m_pRobloxMainWindow->playSoloAction);
    updateAction(*m_pRobloxMainWindow->startServerAction);
    updateAction(*m_pRobloxMainWindow->startPlayerAction);

    updateAction(*m_pRobloxMainWindow->findAction);
    updateAction(*m_pRobloxMainWindow->replaceAction);
    updateAction(*m_pRobloxMainWindow->findNextAction);
    updateAction(*m_pRobloxMainWindow->findPreviousAction);
    updateAction(*m_pRobloxMainWindow->goToLineAction);

    updateAction(*m_pRobloxMainWindow->simulationPlayAction);
    updateAction(*m_pRobloxMainWindow->simulationRunAction);
    updateAction(*m_pRobloxMainWindow->simulationResetAction);
    updateAction(*m_pRobloxMainWindow->simulationStopAction);

    updateAction(*m_pRobloxMainWindow->rotateSelectionAction);
    updateAction(*m_pRobloxMainWindow->zoomExtentsAction);

    if (!m_pRobloxMainWindow->isRibbonStyle())
    {
        updateActions(m_pRobloxMainWindow->editCameraToolBar->actions());
        updateActions(m_pRobloxMainWindow->runToolBar->actions());
        updateActions(m_pRobloxMainWindow->advToolsToolBar->actions());
        updateActions(m_pRobloxMainWindow->commandToolBar->actions());
        updateActions(m_pRobloxMainWindow->menuTools->actions());
        updateActions(m_pRobloxMainWindow->scriptMenu->actions());
        updateActions(m_pRobloxMainWindow->standardToolBar->actions());
    }
    else
    {
        // Loop through every ribbon page, and group, and update all actions
        const RibbonMainWindow* pMainWindow = static_cast<RibbonMainWindow*>(m_pRobloxMainWindow);
        const int pagesCount = pMainWindow->ribbonBar()->getPageCount();
        for (int i = 0; i < pagesCount; i++)
        {
            const QList<RibbonGroup*> ribbonGroups = pMainWindow->ribbonBar()->getPage(i)->findChildren<RibbonGroup*>();
            for (int k = 0; k < ribbonGroups.count(); k++)
            {
                RibbonGroup* group = ribbonGroups[k];

                const QList<RibbonGallery*> galleries = group->findChildren<RibbonGallery*>();
                for (int j = 0; j < galleries.count(); j++)
                {
                    const bool galleryEnabled = RobloxDocManager::Instance().getPlayDoc() != NULL;
                    galleries[j]->setEnabled(galleryEnabled);
                }

                updateActions(group->actions());
                if (group->optionButtonAction() && group->optionButtonAction()->isVisible())
                    updateAction(*group->optionButtonAction());
            }
        }

        // Update quickaccess bar's actions
        if (pMainWindow->ribbonBar()->getQuickAccessBar())
            updateActions(pMainWindow->ribbonBar()->getQuickAccessBar()->actions());

        // Update all button groups, since they are not associated with an action
        QList<QButtonGroup*> buttonGroups = pMainWindow->findChildren<QButtonGroup*>();
        for (int ii = 0; ii < buttonGroups.count(); ++ii)
            modifyButtonGroupState(*buttonGroups.at(ii));

        // Update all menu actions under ToolButtonProxyMenu
        QList<ToolButtonProxyMenu*> toolButtonProxies = pMainWindow->findChildren<ToolButtonProxyMenu*>();
        for (int ii = 0; ii < toolButtonProxies.count(); ++ii)
            updateActions(toolButtonProxies.at(ii)->actions());

        updateAction(*m_pRobloxMainWindow->anchorAction); // Updated based on selection
    }
}

void UpdateUIManager::updateActions(const QList<QAction*>& actions)
{
    for (int i = 0; i < actions.size(); ++i)
    {
        QAction* action = actions.at(i);
        updateAction(*action);

        if (action->inherits("QWidgetAction"))
        {
            QWidgetAction* widgetAction = static_cast<QWidgetAction*>(action);
            QWidget* pDefaultWidget = widgetAction->defaultWidget();
            if (!pDefaultWidget)
                continue;

            if (pDefaultWidget->inherits("QToolButton"))
            {
                QToolButton* button = static_cast<QToolButton*>(pDefaultWidget);
                updateAction(*button->defaultAction());
            }
            else if (pDefaultWidget->inherits("QRadioButton"))
            {
                QRadioButton* button = static_cast<QRadioButton*>(pDefaultWidget);
                // Block all signals: Temporary Hack to fix Ribbon Bar slow down caused by the Radio buttons.
                bool oldState = pDefaultWidget->blockSignals(true);
                const QString buttonName = button->objectName();
                QAction* action = m_pRobloxMainWindow->findChild<QAction*>(buttonName);

                if (action)
                {
                    updateAction(*action);
                    if (FFlag::StudioSeparateActionByActivationMethod)
                    {
                        button->setChecked(action->isChecked());
                        button->setEnabled(action->isEnabled());
                    }
                }

                // Unblock all signals: Temporary hack to fix Ribbon Bar slow down caused by Radio buttons.
                pDefaultWidget->blockSignals(oldState);
            }
        }
    }
}

void UpdateUIManager::updateAction(QAction& action)
{
    if (action.objectName().isEmpty())
        return;

    bool checkedState = action.isChecked();
    bool enabledState = action.isEnabled();

    // These commands should always be enabled
    if (UpdateUIManager::defaultCommands.contains(action.objectName()))
    {
        modifyActionState(action, true, checkedState);
    }
    else
    {
        IRobloxDoc* pDoc = RobloxDocManager::Instance().getCurrentDoc();
        if (!pDoc)
        {
            // No current doc, disabled, but keep current checkedstate
            modifyActionState(action, false, checkedState);
        }
        else if (pDoc->actionState(action.objectName(), enabledState, checkedState))
        {
            // pass on to current doc
            modifyActionState(action, enabledState, checkedState);
        }
    }
}

void UpdateUIManager::modifyActionState(QAction& action, bool enabledState, bool checkedState)
{
    // force disabled if we're in a busy state
    if (UpdateUIManager::Instance().isBusy())
        enabledState = false;

    m_pRobloxMainWindow->updateInternalWidgetsState(&action, enabledState, checkedState);

    // don't spam if we're already configured properly
    if (action.isEnabled() == enabledState && (!action.isCheckable() || action.isChecked() == checkedState))
    {
        return;
    }

    if (QThread::currentThread() == thread())
    {
        action.setEnabled(enabledState);
        action.setChecked(checkedState);
    }
    else
    {
        QMetaObject::invokeMethod(&action, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, enabledState));
        QMetaObject::invokeMethod(&action, "setChecked", Qt::QueuedConnection, Q_ARG(bool, checkedState));
    }
}

void UpdateUIManager::modifyButtonGroupState(QButtonGroup& pButtonGroup)
{
    bool checkedState = false;
    bool enabledState = false;

    IRobloxDoc* pDoc = RobloxDocManager::Instance().getCurrentDoc();
    if (pDoc)
        pDoc->actionState(pButtonGroup.objectName(), enabledState, checkedState);

    QList<QAbstractButton*> buttons = pButtonGroup.buttons();

    for (int ii = 0; ii < buttons.count(); ++ii)
    {
        buttons.at(ii)->setEnabled(enabledState);
        if (buttons.at(ii)->isCheckable())
        {
            if (QThread::currentThread() == thread())
            {
                buttons.at(ii)->setChecked(checkedState);
            }
            else
            {
                QMetaObject::invokeMethod(buttons.at(ii), "setChecked", Qt::QueuedConnection, Q_ARG(bool, checkedState));
            }
        }
    }
}

bool UpdateUIManager::toggleDockView(const QString& actionName)
{
    QMap<QString, EDockWindowID>::iterator iter = m_actionDockMap.find(actionName);
    if (iter == m_actionDockMap.end())
        return false;

    EDockWindowID id = iter.value();

    if (m_DockWidgets[id]->widget() != m_ViewWidgets[id])
    {
        AYAASSERT(0);
        m_DockWidgets[id]->setWidget(m_ViewWidgets[id]);
    }
    bool visibility1 = m_DockWidgets[id]->isVisible();
    bool wasSetChecked = m_DockData[id].UpdateAction->isChecked();
    setDockVisibility(id, wasSetChecked);
    bool visibility2 = m_DockWidgets[id]->isVisible();


    if (wasSetChecked)
        m_DockWidgets[id]->raise();

    if (visibility1 == visibility2)
        return false;

    return true;
}

/**
 * Stops status bar updates.
 *  Increments pause state.  Paused if pause state > 0.
 */
void UpdateUIManager::pauseStatusBarUpdate()
{
    ++m_PauseStatusBar;
}

/*
#if WINVER >= 0x601
/**
 * Synchronizes the task bar progress with the progress bar progress.
void UpdateUIManager::configureTaskBar(QProgressBar* progressBar)
{
    if ( !m_isTaskBarInitialized )
        return;

    m_taskProgressBar = progressBar;

    if ( !progressBar )
    {
        m_taskbar->SetProgressState(m_pRobloxMainWindow->winId(),TBPF_NOPROGRESS);
        return;
    }

    int min = progressBar->minimum();
    int max = progressBar->maximum();

    if ( min == max )   // indeterminate
    {
        m_isIndeterminateProgress = true;
        m_taskbar->SetProgressState(m_pRobloxMainWindow->winId(),TBPF_INDETERMINATE);
    }
    else
    {
        m_isIndeterminateProgress = false;
        m_taskbar->SetProgressState(m_pRobloxMainWindow->winId(),TBPF_NORMAL);

        m_totalProgress = max - min;    // convert to 0 to max
        m_taskbar->SetProgressValue(m_pRobloxMainWindow->winId(),0,m_totalProgress);
    }
}
// #endif // WINVER
*/
// d9mz - Olaf I-Dont Give a fuck gif

/**
 * Change the cursor to the wait cursor and shut off some operations while busy.
 *  Only marked unbusy if busy state is 0.  Each setBusy(true) must be followed by a
 *  setBusy(false) so the busy state will reach 0.
 */
void UpdateUIManager::setBusy(bool busy, bool useWaitCursor)
{
    m_BusyState += busy ? 1 : -1;

    m_pRobloxMainWindow->setUpdatesEnabled(false);

    if (m_BusyState == 1 && busy)
    {
        // only go into busy state 1 for the first time.
        // if we go to 2+ then back to 1, we don't want to redo this code

        if (useWaitCursor)
            QApplication::setOverrideCursor(Qt::WaitCursor);

        pauseStatusBarUpdate();

        // save currently focused widget
        m_BusyFocusWidget = QApplication::focusWidget();

        // disable any menu updates
        UpdateUIManager::Instance().setMenubarEnabled(false);

        // disable widgets
        m_pRobloxMainWindow->enableScriptCommandInput(false);
        m_pRobloxMainWindow->centralWidget()->setEnabled(false);

        // disable all actions
        m_BusyEnabledActions.clear();
        QList<QAction*> actions = m_pRobloxMainWindow->findChildren<QAction*>();
        QList<QAction*>::iterator iter = actions.begin();
        while (iter != actions.end())
        {
            QAction* action = *iter;

            // only worry about actions with names
            if (!action->objectName().isEmpty())
            {
                m_BusyEnabledActions[action] = action->isEnabled();
                action->setEnabled(false);
            }

            ++iter;
        }

        // disable the dock widgets
        for (int i = 0; i < eDW_MAX; ++i)
        {
            const EDockWindowID id = (EDockWindowID)i;
            m_DockWidgets[id]->setEnabled(false);
        }

        /*
        d9mz - Olaf I Dont Give a Fuck Gif
        #if WINVER >= 0x601
                // change task bar overlay icon to a busy icon
                if ( m_isTaskBarInitialized )
                {
                    QIcon icon = m_pRobloxMainWindow->style()->standardIcon(QStyle::SP_MessageBoxWarning);

                    HICON overlay_icon = icon.isNull() ? NULL : icon.pixmap(48).toWinHICON();
                    m_taskbar->SetOverlayIcon(m_pRobloxMainWindow->winId(),overlay_icon,L"Busy");

                    if ( overlay_icon )
                        ::DestroyIcon(overlay_icon);
                }
        #endif
        */
    }
    else if (m_BusyState == 0)
    {
        if (useWaitCursor)
            QApplication::restoreOverrideCursor();

        resumeStatusBarUpdate();

        // enable menu updates
        UpdateUIManager::Instance().setMenubarEnabled(true);

        // enable widgets
        m_pRobloxMainWindow->enableScriptCommandInput(true);
        m_pRobloxMainWindow->centralWidget()->setEnabled(true);

        // restore focus if we can, otherwise set to main window
        //  this fixes some focus issues on mac
        m_pRobloxMainWindow->centralWidget()->setFocus();
        if (!m_BusyFocusWidget.isNull())
            m_BusyFocusWidget->setFocus();
        else if (QApplication::focusWidget())
            QApplication::focusWidget()->setFocus();
        else
            m_pRobloxMainWindow->setFocus();
        m_BusyFocusWidget = NULL;

        // enable all main actions
        QList<QAction*> actions = m_BusyEnabledActions.keys();
        QList<QAction*>::iterator iter = actions.begin();
        while (iter != actions.end())
        {
            QAction* action = *iter;
            action->setEnabled(m_BusyEnabledActions[action]);
            ++iter;
        }
        m_BusyEnabledActions.clear();

        // enable the dock widgets
        for (int i = 0; i < eDW_MAX; ++i)
        {
            const EDockWindowID id = (EDockWindowID)i;
            m_DockWidgets[id]->setEnabled(true);
        }

        /*
        // d9mz - Olaf I Dont give a fuck gif
        #if WINVER >= 0x601
                // restore task bar overlay icon
                if ( m_isTaskBarInitialized )
                    m_taskbar->SetOverlayIcon(m_pRobloxMainWindow->winId(),NULL,L"");
        #endif
        */
    }

    m_pRobloxMainWindow->setUpdatesEnabled(true);
}

/**
 * Resumes the status bar updates.
 *  The pause state is decremented.  Only at 0 will the status bar be updated.
 */
void UpdateUIManager::resumeStatusBarUpdate()
{
    --m_PauseStatusBar;
    if (m_isRunning && !m_PauseStatusBar)
        QTimer::singleShot(200, this, SLOT(updateStatusBar()));
}

/**
 * Creates a new progress bar and adds it to the status bar.
 *  If min == max then the progress bar is set to indeterminate.
 *  Synchronizes the task bar progress.
 *
 * @param   message     text message displayed in a label to the left
 * @param   min         minimum progress bar value (start)
 * @param   max         maximum progress bar value (end)
 */
QProgressBar* UpdateUIManager::showProgress(const QString& message, int min, int max)
{
    QStatusBar* statusBar = m_pRobloxMainWindow->statusBar();

    // configure progress bar widget
    QProgressBar* progressBar = new QProgressBar(statusBar);
    progressBar->setTextVisible(false); // cleaner
    progressBar->setMinimum(min);
    progressBar->setMaximum(max);
    progressBar->setValue(min);

    // configure static label
    QLabel* label = new QLabel(message, progressBar);

    statusBar->setUpdatesEnabled(false);
    statusBar->clearMessage();
    statusBar->addWidget(label, 0);
    statusBar->addWidget(progressBar, 1);
    statusBar->setUpdatesEnabled(true);

    // set up indeterminate if necessary
    if (min == max)
        progressBar->setValue(-1);

    m_ProgressBars.append(progressBar);
    m_ProgressLabels.append(label);

    // make sure they get rendered at least once
    progressBar->show();
    label->show();

    /*
    // d9mz - I give a fuck :D
    #if WINVER >= 0x601
        // sync task bar progress with progress bar
        configureTaskBar(progressBar);
    #endif
    */
    return progressBar;
}

/**
 * Deletes the progress bar.
 */
void UpdateUIManager::hideProgress(QProgressBar* progressBar)
{
    AYAASSERT(m_ProgressBars.contains(progressBar));
    int index = m_ProgressBars.indexOf(progressBar);
    if (index != -1)
    {
        delete m_ProgressBars[index];
        m_ProgressBars.remove(index);

        delete m_ProgressLabels[index];
        m_ProgressLabels.remove(index);
    }

    /*
    // I don't give a fuck
    #if WINVER >= 0x601
        if ( m_isTaskBarInitialized )
        {
            // reset the "main" progress bar used to synchronize the task bar
            if ( progressBar == m_taskProgressBar )
            {
                configureTaskBar(NULL);
                if ( !m_ProgressBars.empty() )
                    configureTaskBar(m_ProgressBars.back());
            }
        }
    #endif*/
}

/**
 * Updates the progress bar with a new value.
 *  If the value is -1 and the progress bar is indeterminate, update
 *  the indeterminate value.
 *  Synchronizes the task bar if this progress bar is the "main" one.
 */
void UpdateUIManager::updateProgress(QProgressBar* progressBar, int value)
{
    AYAASSERT(progressBar);

    // check for indeterminate and update
    if (value == -1)
        value = progressBar->value() + 1;

    progressBar->setValue(value);
    progressBar->update();
    QApplication::processEvents();

    /*
    // d9mz - I don't give a fuck
    #if WINVER >= 0x601
        if ( progressBar == m_taskProgressBar )
        {
            // synchronize the task bar progress
            if ( m_isTaskBarInitialized && !m_isIndeterminateProgress )
                m_taskbar->SetProgressValue(m_pRobloxMainWindow->winId(),value,m_totalProgress);
        }
    #endif*/
}

/**
 * "Blocks" the main thread while a child thread is processed.
 *  The main thread can still handles events and GUI updates.  This allows it to be
 *  responsive and update the progress bar.  The application can set the state
 *  as "busy" to prevent reentrancy.  Long running processes general lock resources and
 *  reentrancy will cause a deadlock.
 *
 * @param   message         message to display for the progress bar
 * @param   userWorkFunc    long running function to run in a child thread
 */
void UpdateUIManager::waitForLongProcess(const QString& message, boost::function<void()> userWorkFunc)
{
    QEventLoop eventLoop;
    QMutex eventLoopMutex;

    volatile bool done = false;

    m_longProcessInProgress = true;


    new boost::thread(boost::bind(&UpdateUIManager::exectueProgressFunction, this, userWorkFunc, &eventLoop, &eventLoopMutex, (bool*)&done));

    // wait for approx 300 ms, check if done and abort
    //  this avoids needing to show the progress and setting busy if the operation is fast
    for (int i = 0; i < 30 && !done; i++)
        QtUtilities::sleep(10);

    if (!done)
    {
        setBusy(true);
        QProgressBar* progressBar = showProgress(message);

        // if still not done, then start the event loop
        if (!done)
        {
            QMutexLocker locker(&eventLoopMutex);
            if (!done)
                eventLoop.exec();
        }

        hideProgress(progressBar);
        setBusy(false);
    }
    m_longProcessInProgress = false;
}

/**
 * Thread main for a progress function.
 *  Unblocks the main thread when done.
 */
void UpdateUIManager::exectueProgressFunction(boost::function<void()> userWorkFunc, QEventLoop* eventLoop, QMutex* eventLoopMutex, bool* done)
{
    AYAASSERT(eventLoop);
    AYAASSERT(eventLoopMutex);
    AYAASSERT(done);

    try
    {
        userWorkFunc();
    }
    catch (...)
    {
        AYAASSERT(false);
    }

    // check to see if the event loop is running
    if (eventLoopMutex->tryLock())
    {
        // abort starting the event loop completely
        *done = true;
        eventLoopMutex->unlock();
    }
    else
    {
        // we're already executing or starting to execute, can't set done now

        // wait until we really are executing
        while (!eventLoop->isRunning())
            QtUtilities::sleep(10);

        // now shut it down since we know its running
        eventLoop->exit();
    }
}

void UpdateUIManager::commonContextMenuActions(QList<QAction*>& commonActions, bool insertIntoPasteMode)
{
    commonActions.append(m_pRobloxMainWindow->cutAction);
    commonActions.append(m_pRobloxMainWindow->copyAction);
    commonActions.append(insertIntoPasteMode ? m_pRobloxMainWindow->pasteIntoAction : m_pRobloxMainWindow->pasteAction);
    commonActions.append(m_pRobloxMainWindow->duplicateSelectionAction);
    commonActions.append(m_pRobloxMainWindow->deleteSelectedAction);
    commonActions.append(m_pRobloxMainWindow->renameObjectAction);
    commonActions.append(NULL);
    commonActions.append(m_pRobloxMainWindow->groupSelectionAction);
    commonActions.append(m_pRobloxMainWindow->ungroupSelectionAction);
    commonActions.append(m_pRobloxMainWindow->selectChildrenAction);
    commonActions.append(m_pRobloxMainWindow->zoomExtentsAction);

    commonActions.append(NULL);
    commonActions.append(m_pRobloxMainWindow->insertIntoFileAction);

    commonActions.append(NULL);
    commonActions.append(m_pRobloxMainWindow->selectionSaveToFileAction);
    commonActions.append(m_pRobloxMainWindow->saveToRobloxAction);
    commonActions.append(m_pRobloxMainWindow->createNewLinkedSourceAction);

    commonActions.append(m_pRobloxMainWindow->publishAsPluginAction);
    commonActions.append(m_pRobloxMainWindow->exportSelectionAction);
}

void UpdateUIManager::updateStatusBar()
{
    const Aya::TaskScheduler& ts(Aya::TaskScheduler::singleton());
    QString str;
    str.asprintf("Sleep: %.1f Wait: %.1f Run: %.2f Affinity: %d%% Scheduler: %.0f/s %d%% SortFreq: %.0f/s ErrorCalc: %.0f/s", ts.numSleepingJobs(),
        ts.numWaitingJobs(), ts.numRunningJobs(), (int)(100.0 * ts.threadAffinity()), ts.schedulerRate(),
        (int)(100.0 * ts.getSchedulerDutyCyclePerThread()), ts.getSortFrequency(), ts.getErrorCalculationRate());

    m_pTaskSchedulerSBLabel->setText(str);

    IRobloxDoc* pDoc = RobloxDocManager::Instance().getCurrentDoc();
    if (pDoc)
    {
        pDoc->handleAction("actionUpdateStatusBar");
    }

    if (m_isRunning && !m_PauseStatusBar)
        QTimer::singleShot(200, this, SLOT(updateStatusBar()));
}

void UpdateUIManager::setStatusLabels(QString sTimeLabel, QString sFpsLabel)
{
    m_pTimeSBLabel->setText(sTimeLabel);
    m_pFPSSBLabel->setText(sFpsLabel);
}

void UpdateUIManager::onQuickInsertFocus()
{
    RobloxIDEDoc* playDoc = RobloxDocManager::Instance().getPlayDoc();
    if (!playDoc)
        return;

    getViewWidget<InsertObjectWidget>(eDW_BASIC_OBJECTS)
        .startingQuickInsert(QApplication::focusWidget(), !m_DockWidgets[eDW_BASIC_OBJECTS]->isVisible());

    setDockVisibility(eDW_BASIC_OBJECTS, true);

    m_DockWidgets[eDW_BASIC_OBJECTS]->raise();

    m_ViewWidgets[eDW_BASIC_OBJECTS]->activateWindow();
    m_ViewWidgets[eDW_BASIC_OBJECTS]->setFocus(Qt::ShortcutFocusReason);
}

void UpdateUIManager::filterExplorer()
{
    m_ViewWidgets[eDW_OBJECT_EXPLORER]->setFocus(Qt::ShortcutFocusReason);
}

void UpdateUIManager::filterProperties()
{
    m_ViewWidgets[eDW_PROPERTIES]->setFocus(Qt::ShortcutFocusReason);
}

bool UpdateUIManager::eventFilter(QObject* watched, QEvent* evt)
{
    // Capture dock widget closing and set checked to false
    if ((evt->type() == QEvent::Show || evt->type() == QEvent::Hide) && watched->inherits("QDockWidget"))
    {
        QString dockName = watched->objectName();
        for (int i = 0; i < eDW_MAX; i++)
        {
            EDockWindowID id = (EDockWindowID)i;
            if (m_DockWidgets[id]->objectName() == dockName)
                m_DockData[id].UpdateAction->setChecked(m_DockWidgets[id]->isVisible());
        }
    }

    return false;
}

QAction* UpdateUIManager::getAction(const QString& actionName)
{
    QList<QAction*> allActions = m_pRobloxMainWindow->findChildren<QAction*>(actionName);
    if (allActions.size())
        return allActions.at(0);
    return NULL;
}

QList<QString> UpdateUIManager::getDockActionNames() const
{
    return m_actionDockMap.keys();
}

QAction* UpdateUIManager::getDockAction(const EDockWindowID id) const
{
    return m_DockData[id].UpdateAction;
}

void UpdateUIManager::updateBuildMode()
{
    eBuildMode buildMode = m_pRobloxMainWindow->getBuildMode();

    m_pRobloxMainWindow->setUpdatesEnabled(false);

    RobloxIDEDoc* playDoc = RobloxDocManager::Instance().getPlayDoc();
    if (playDoc)
    {
        playDoc->enableUndo(buildMode != BM_BASIC);

        if (buildMode == BM_BASIC)
            RobloxDocManager::Instance().setPlayDocCentralWidget();
        else
            RobloxDocManager::Instance().restoreCentralWidget();
    }

    if (buildMode == BM_BASIC)
    {
        // hide the dock widgets
        for (int i = 0; i < eDW_MAX; ++i)
        {
            const EDockWindowID id = (EDockWindowID)i;
            setDockVisibility(id, false);
        }

        // hide the toolbars
        QObjectList objects = m_pRobloxMainWindow->children();
        QObjectList::iterator iter = objects.begin();
        for (; iter != objects.end(); ++iter)
        {
            QToolBar* toolbar = dynamic_cast<QToolBar*>(*iter);
            if (toolbar)
                toolbar->setVisible(false);
        }

        // hide menu and status
        m_pRobloxMainWindow->menuBar()->setVisible(false);
        m_pRobloxMainWindow->statusBar()->setVisible(false);
        m_pRobloxMainWindow->setWindowState(Qt::WindowFullScreen);
    }
    else if (buildMode == BM_ADVANCED)
    {
        // show menu and status
        if (!m_pRobloxMainWindow->isRibbonStyle())
        {
            m_pRobloxMainWindow->menuBar()->setVisible(true);
            m_pRobloxMainWindow->statusBar()->setVisible(true);
        }

        // reload layout - this will restore all docks and toolbars to previous state
        // m_pRobloxMainWindow->loadApplicationStates();
        // m_pRobloxMainWindow->show();

        // show the dock view widgets
        for (int i = 0; i < eDW_MAX; ++i)
        {
            const EDockWindowID id = (EDockWindowID)i;
            if (m_DockWidgets[id]->isVisible() && m_ViewWidgets[id])
                m_ViewWidgets[id]->setVisible(true);
        }
    }

    updateDockActionsCheckedStates();
    m_pRobloxMainWindow->updateWindowTitle();

    Aya::MouseCommand::enableAdvArrowTool(buildMode != BM_BASIC);

    Aya::GameBasicSettings::singleton().setStudioMode(buildMode != BM_BASIC);
    Aya::GameBasicSettings::singleton().setFullScreen(buildMode == BM_BASIC);

    m_pRobloxMainWindow->setUpdatesEnabled(true);
}

void UpdateUIManager::updateDockActionsCheckedStates()
{
    // Sets the actions to be checked if the dock widget is opened.
    for (int i = 0; i < eDW_MAX; ++i)
    {
        EDockWindowID id = (EDockWindowID)i;
        m_DockData[id].UpdateAction->setChecked(m_DockWidgets[id]->isVisible());
    }
}

bool UpdateUIManager::get3DAxisEnabled()
{
    RobloxSettings settings;
    return settings.value("View_AxisWidget", false).toBool();
}

void UpdateUIManager::set3DAxisEnabled(bool value)
{
    RobloxSettings settings;
    return settings.setValue("View_AxisWidget", value);
}

bool UpdateUIManager::get3DGridEnabled()
{
    RobloxSettings settings;
    return settings.value("View_ZeroPlaneGrid", false).toBool();
}

void UpdateUIManager::set3DGridEnabled(bool value)
{
    RobloxSettings settings;
    return settings.setValue("View_ZeroPlaneGrid", value);
}

QDockWidget* UpdateUIManager::getDockWidget(EDockWindowID dockId)
{
    return m_DockWidgets[dockId];
}

bool UpdateUIManager::isLongProcessInProgress()
{
    return m_longProcessInProgress;
}

void UpdateUIManager::showErrorMessage(QString title, QString message)
{
    QMessageBox::critical(&getMainWindow(), title, message);
}

void UpdateUIManager::configureDockWidget(QDockWidget* dockWidget)
{
    if (!m_pRobloxMainWindow->restoreDockWidget(dockWidget))
        m_pRobloxMainWindow->addDockWidget(Qt::LeftDockWidgetArea, dockWidget);

    if (!m_pRobloxMainWindow->isRibbonStyle())
    {
        m_pRobloxMainWindow->viewToolsToolBar->addAction(dockWidget->toggleViewAction());
    }
    else
    {
        if (RibbonPage* pPage = m_pRobloxMainWindow->ribbonBar()->findChild<RibbonPage*>("View"))
        {
            if (RibbonGroup* pGroup = pPage->findChild<RibbonGroup*>("Show"))
                pGroup->addAction(dockWidget->toggleViewAction(), Qt::ToolButtonTextBesideIcon);
        }
    }

    dockWidget->toggleViewAction()->setIconVisibleInMenu(false);

    // remove any existing connections on the dock's toggleViewAction
    QObject::disconnect(dockWidget->toggleViewAction(), nullptr, nullptr, nullptr);

    QAction* dockAct = dockWidget->toggleViewAction();

    // make its icon hidden in menus
    dockAct->setIconVisibleInMenu(false);

    // keep UI updated when visibility changes
    QObject::connect(dockWidget, &QDockWidget::visibilityChanged, this, &UpdateUIManager::updateActionWidgetVisibility, Qt::UniqueConnection);

    // optionally, if updateActionWidgetVisibility expects to run when the dock action toggles:
    QObject::connect(dockAct, &QAction::toggled, this, &UpdateUIManager::updateActionWidgetVisibility, Qt::UniqueConnection);
}

void UpdateUIManager::updateActionWidgetVisibility()
{
    QAction* pAction = qobject_cast<QAction*>(sender());
    if (!pAction || !pAction->parentWidget())
        return;

    pAction->parentWidget()->setVisible(pAction->isChecked());
    if (pAction->isChecked())
        pAction->parentWidget()->raise();
}