


#include "RobloxMainWindow.hpp"

// Qt Headers
#include <QNetworkProxyFactory>
#include <QFileOpenEvent>
#include <QDateTime>
#include <QScrollBar>
#include <QFileInfo>
#include <QDesktopServices>
#include <QDir>
#include <QComboBox>
#include <QToolTip>
#include <QFileDialog>
#include <QLineEdit>
#include <QLabel>
#include <QToolBar>
#include <QToolButton>
#include <QCompleter>
#include <QWidgetAction>
#include <QProcess>
#include <QSignalMapper>
#include <QSplashScreen>
#include <QSharedMemory>
#include <QShortcut>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPageSetupDialog>
#include <QPainter>
#include <QResource>
#include <QMimeData>
#include <QGuiApplication>
#include <QScreen>
#include <QNetworkProxy>
#include <QtConcurrentRun>

#include <functional>

// 3rd Party Headers
#include "boost/filesystem/path.hpp"
#include "boost/iostreams/stream.hpp"
#include "boost/iostreams/device/array.hpp"

// Roblox Headers
#include "Utility/StandardOut.hpp"

#include "Utility/ScopedAssign.hpp"

#include "Utility/RbxStringTable.hpp"

#include "Utility/SoundService.hpp"

#include "Script/ScriptContext.hpp"
#include "DataModel/DebugSettings.hpp"

#include "DataModel/PartInstance.hpp"

#include "DataModel/ModelInstance.hpp"

#include "DataModel/Explosion.hpp"

#include "DataModel/PluginManager.hpp"

#include "DataModel/GameBasicSettings.hpp"

#include "DataModel/FastLogSettings.hpp"

#include "DataModel/Stats.hpp"

#include "World/Contact.hpp"
#include "Kernel/Body.hpp"

#include "Kernel/ContactConnector.hpp"

#include "BaldPtr.hpp"

#include "CEvent.hpp"

#include "Players.hpp"

#include "FastLog.hpp"
#include "RobloxStudioVersion.hpp"
#include "RobloxQuickAccessConfig.hpp"
#include "DataModel/RenderSettingsItem.hpp"
#include "Utility/RobloxServicesTools.hpp"

#include "DataModel/CSGMesh.hpp"

#include "Utility/CSGKernel.hpp"


#ifdef _WIN32
#include "WinInet.h"
#else
#include <semaphore.h>
#endif

// Roblox Studio Headers
#include "FunctionMarshaller.hpp"
#include "RobloxDocManager.hpp"
#include "RobloxCustomWidgets.hpp"
#include "Roblox.hpp"
#include "RobloxSettings.hpp"
#include "RobloxSettingsDialog.hpp"
#include "RobloxDiagnosticsView.hpp"
#include "RobloxScriptReview.hpp"
#include "RobloxTaskScheduler.hpp"
#include "CommonInsertWidget.hpp"
#include "RobloxToolBox.hpp"
#include "StudioUtilities.hpp"
#include "RobloxTextOutputWidget.hpp"
#include "QtUtilities.hpp"
#include "AuthoringSettings.hpp"
#include "UpdateUIManager.hpp"
#include "RobloxPluginHost.hpp"
#include "WebDialog.hpp"
#include "RobloxApplicationManager.hpp"
#include "AuthenticationHelper.hpp"
#include "RobloxIDEDoc.hpp"
#include "RobloxInputConfigDialog.hpp"
#include "RobloxKeyboardConfig.hpp"
#include "RobloxMouseConfig.hpp"
#include "RobloxStudioVerbs.hpp"
#include "RobloxScriptDoc.hpp"
#include "ShortcutHelpDialog.hpp"
#include "RobloxUser.hpp"
#include "AutoSaveDialog.hpp"
#include "InsertServiceDialog.hpp"
#include "StudioIntellesense.hpp"
#include "RobloxWebDoc.hpp"
#include "RbxWorkspace.hpp"
#include "FindDialog.hpp"
#include "ScriptTextEditor.hpp"


#include "StudioDeviceEmulator.hpp"

DYNAMIC_FASTFLAGVARIABLE(ShowAutoSaveDialog, false)

FASTFLAGVARIABLE(StudioCheckForUpgradeEnabled, false)
FASTFLAGVARIABLE(StudioSeparateActionByActivationMethod, false)
FASTFLAGVARIABLE(StudioFixMacStartPage, false)
FASTFLAGVARIABLE(StudioSplashScreenUntilOpen, false)
FASTFLAGVARIABLE(StudioEarlyCookieConstraintCheckGlobal, false)
FASTSTRINGVARIABLE(StudioCookieConstraintUrlFragment, "fulfillconstraint")

FASTINTVARIABLE(StudioBootstrapperVersionNumber, 52886)
FASTINTVARIABLE(StudioInsertDeletionCheckTimeMS, 8000)

FASTFLAGVARIABLE(StudioCustomStatsEnabled, false)
FASTFLAGVARIABLE(StudioSettingsGAEnabled, true)
FASTFLAGVARIABLE(AssertOnConfigurationReportToGA, true)
FASTFLAGVARIABLE(ReportBuildVSEditMode, true)
FASTFLAGVARIABLE(StudioDoublingOnUploadFixEnabled, true)

FASTFLAG(StudioInSyncWebKitAuthentication)
FASTFLAG(StudioDataModelIsStudioFix)
FASTFLAG(StudioShowsByDefault)
FASTFLAG(DontSwallowInputForStudioShortcuts)
FASTFLAG(StudioSettingsGAEnabled)
FASTFLAG(UseBuildGenericGameUrl)

FASTINT(StudioWebDialogMinimumWidth)
FASTINT(StudioWebDialogMinimumHeight)

LOGGROUP(Network)

FASTFLAGVARIABLE(Dep, true)

static const int MaxLengthFilenameMRU = 64;                    //!< maximum length of a filename in the MRU list
static const char* sWindow_Title = AYA_PROJECT_NAME " Studio"; //!< default main window title when no place selected
static const char* sDialog_Title = AYA_PROJECT_NAME " Studio"; //!< default child dialog title
static const int SplashTotalTime = 3505;                       //!< minimum time to display splashscreen

bool RobloxMainWindow::sIsAppRunning = false;
const QString RobloxMainWindow::NEW_PLACE_FILENAME = "ROBLOX_NEW_PLACE";

QString RobloxMainWindow::sWindowStateKey = "window_state";
QString RobloxMainWindow::sGeometryKey = "window_geometry";

#ifdef __APPLE__
#include "LogProvider.hpp"
static LogProvider logProvider;

#include "StudioMacUtilities.hpp"
#endif

RobloxMainWindow* RobloxMainWindow::get(QObject* context)
{
    while (context != NULL && dynamic_cast<RobloxMainWindow*>(context) == NULL)
    {
        context = context->parent();
    }

    return static_cast<RobloxMainWindow*>(context);
}

RobloxMainWindow::RobloxMainWindow(const QMap<QString, QString> argMap)
    : RobloxRibbonMainWindow(this)
    , m_pSettingsDialog(NULL)
    , m_pMinutesPlayedTimer(NULL)
    , m_pInsertServiceDlg(NULL)
    , m_pShortcutHelpDialog(NULL)
    , m_pInputConfigDialog(NULL)
    , m_pPluginHost(new RobloxPluginHost(this))
    , m_splashScreen(NULL)
    , m_publishGameDialog(NULL)
    , m_AutoSaveAccum(0)
    , m_IsInitialized(false)
    , m_pPublishedProjectsWebDialog(NULL)
    , m_isRibbon(false)
    , m_fileOpenHandled(false)
    , m_BuildMode(BM_ADVANCED)
    , m_cookieConstraintChecker(NULL)
    , m_cookieConstraintCheckDone(NULL)
    , m_appSettings(new AppSettings(QCoreApplication::applicationDirPath().toStdString()))
{
    try
    {
        QNetworkProxyFactory::setUseSystemConfiguration(true);

        // grab all of our known args from the map (to be used later)
        parseCommandLineOptions(argMap);

        // load appsettings
        if (!m_appSettings->load())
        {
            // show error
            QMessageBox::warning(nullptr, "Error",
                "Failed to load application settings from AppSettings.ini. Make sure that the file exists and is free of any errors.");
            QApplication::exit(-1);
            return;
        }

        // make sure we call this first, this will initialize boost related stuff also
        // or else we can run into raise condition which doing HTTP init as being mentioned here
        // - https://svn.boost.org/trac/boost/ticket/6320
        boost::filesystem::path::codecvt();

        Aya::Game::globalInit(true);

        // get logging going quickly, so any output is recorded
        setupLogging();
        // Aya::Log::current()->writeEntry(Aya::Log::Information,"RobloxMainWindow::RobloxMainWindow - start");

        std::string clientSettings;
        try
        {
            FetchClientSettingsData(CLIENT_APP_SETTINGS_STRING, CLIENT_SETTINGS_API_KEY, &clientSettings);
            LoadClientSettingsFromString(CLIENT_APP_SETTINGS_STRING, clientSettings, &Aya::ClientAppSettings::singleton());
        }
        catch (std::exception&)
        {
            FASTLOG(FLog::Error, "Failed to load settings");
        }

        // uncomment asap
        // AuthenticationHelper::Instance().authenticateUserAsync(urlArg, ticketArg);

        // set up ui using the .ui file
        setupUi(this);
        // this->setStyleSheet("background-color: white;");

        // set up external analytics reporting variables
        QString country = QLocale::countryToString(QLocale::system().country());

        // initializing textOutputWidget early to catch any output in globalInit
        m_pTextOutput = new RobloxTextOutputWidget(dockWidgetContents_2);

        Aya::Http robloxRequest(AuthenticationHelper::getLoggedInUserUrl().toStdString());
        Aya::Http externalRequest("http://www.google.com");

        QSettings retentionData("Roblox", "Retention");

        static const char* const kRetentionInstallDateKey = "InstallDate";
        static const char* const kRetentionLastRunDateKey = "LastRunDate";
        QString dateString = QDate::currentDate().toString("yyyyMMdd");
        QString installDate = retentionData.value(kRetentionInstallDateKey, "").toString();

        if (installDate.isEmpty())
        {
            StudioUtilities::setIsFirstTimeOpeningStudio(true);
        }

        // Init our engine - can only use ClientAppSettings AFTER this!!!
        Roblox::globalInit(urlArg, ticketArg);

        // Disable FRM in studio by default
        CRenderSettingsItem::singleton().setEnableFRM(false);

        // Following are the different modes launched from website
        // (this is required since website conveys 'only' to be launched from Studio but doesn't specify the mode
        // 1) Build mode: BuildArgument == True (default mode set)
        // 2) Edit mode: BuildArgument == True && ScriptArgument has edit.ashx
        // 3) Start Page mode: BuildArgument == True && ScriptArgument is empty
        if (StudioUtilities::containsEditScript(argMap[ScriptArgument]) || ((argMap[BuildArgument] == "TRUE") && argMap[ScriptArgument].isEmpty()))
        {
            m_BuildMode = BM_ADVANCED;
            StudioUtilities::setAvatarMode(false);
        }

        Aya::Http::useDefaultTimeouts = false;

        if (AuthoringSettings::singleton().getUIStyle() == AuthoringSettings::Ribbon)
        {
            m_isRibbon = true;
        }

        // check for crash
        RobloxSettings settings;
        settings.setValue("appClosed", false);

#ifdef _WIN32
        if (FFlag::Dep)
        {
            typedef BOOL(WINAPI * SetProcessDEPPolicyPfn)(DWORD);
            SetProcessDEPPolicyPfn pfn =
                reinterpret_cast<SetProcessDEPPolicyPfn>(GetProcAddress(GetModuleHandleA("Kernel32"), "SetProcessDEPPolicy"));
            if (pfn)
            {
                static const DWORD kEnable = 1;
                pfn(kEnable);
            }
        }
#endif

        {
            RobloxSettings settings;
            if (settings.value("studioVersion").toString() != RobloxSettings::getVersionString())
            {
                settings.setValue("studioVersion", RobloxSettings::getVersionString());
            }
        }

        // Set the CSGMesh Factory to use CGAL.
        Aya::CSGMeshFactory::set(new Aya::CSGMeshFactoryCGAL());

        initializeUI();

        // reset the position of the toolbars. so they are identical to the old studio.
        setToolbarPosition();

        // set up command toolbar
        setupCommandToolBar();

        // create material and color tool button
        setupCustomToolButton();

        // set default commands
        UpdateUIManager::Instance().setDefaultApplicationState();

        sIsAppRunning = true;

        // Set Security context Identity
        Aya::Security::Impersonator impersonate(Aya::Security::LocalGUI_);

        // setup statusbar and docks
        UpdateUIManager::Instance().init(this);

        RobloxScriptDoc::init(*this);

        QPainter p(this);

        // Set the background to white
        p.fillRect(rect(), DARKMODE ? QColor(45, 45, 45) : Qt::white);

        if (isRibbonStyle())
        {
            setupRibbonBar();
        }
        else
        {
            setupViewMenu();
        }

        // set up slots
        setupSlots();

        // set up shortcut key sequences
        assignAccelerators();

        updateRecentFilesUI();

        // Idempotent function - and if there is no open tab, we need to update actions (disable ones that need to be)
        UpdateUIManager::Instance().updateToolBars();

        setBuildMode(m_BuildMode);

        // save current state as default state
        saveDefaultApplicationState();

        // load the application states from memory
        loadApplicationStates();

        m_PropertyChangedConnection =
            AuthoringSettings::singleton().propertyChangedSignal.connect(boost::bind(&RobloxMainWindow::onPropertyChanged, this, _1));
        onPropertyChanged(NULL);

        show();

        // take the args that were passed in and act upon them
        // has to be at the very end of the constructor, or else the menu bar will behave weird.

        handleCommandLineOptions();
        m_fileOpenHandled = false;

        // Mac will open file through finder event; need to handle open start page this way
        IRobloxDoc* currentPlayDoc = RobloxDocManager::Instance().getPlayDoc();
        if (!currentPlayDoc)
        {
            // if nothing else opened, open a start web page
            actionStartPage->setChecked(true);
        }

        // Unfortunately has to be called after show due to windowState not loading until then.
        UpdateUIManager::Instance().updateDockActionsCheckedStates();

        viewCommandBarAction->setChecked(commandToolBar->isVisible());

        // set window layout setting according to application states.
        // it has to be after loadApplicationStates. or else full screen button will not be initialized correctly.
        setWindowLayout();

        if (DFFlag::ShowAutoSaveDialog)
        {
            if (AutoSaveDialog::checkForAutoSaveFiles())
            {
                AutoSaveDialog dialog(this);
                while (dialog.exec() == QDialog::Rejected)
                {
                    /* nothing */
                }
            }
        }

        RobloxKeyboardConfig::singleton().storeDefaults(*this);
        RobloxKeyboardConfig::singleton().loadKeyboardConfig(*this);
        RobloxMouseConfig::singleton().loadMouseConfig();

        m_IsInitialized = true;
    }
    catch (std::runtime_error const& exp)
    {
        // must make sure the splash screen is hidden before popping up any errors
        try
        {
        }
        catch (...)
        {
            // ignore errors
        }

        QtUtilities::RBXMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(exp.what());
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
        throw;
    }
    catch (...)
    {
        // must make sure the splash screen is hidden before popping up any errors
        try
        {
        }
        catch (...)
        {
            // ignore errors
        }
        throw;
    }

    // Aya::Log::current()->writeEntry(Aya::Log::Information,"RobloxMainWindow::RobloxMainWindow - end");
}

RobloxMainWindow::~RobloxMainWindow()
{
    // Aya::Log::current()->writeEntry(Aya::Log::Information,"RobloxMainWindow::~RobloxMainWindow");

    m_PropertyChangedConnection.disconnect();

    // remove all documents
    RobloxDocManager::Instance().shutDown();

    // stop UI update
    UpdateUIManager::Instance().shutDown();

    // do a global shut down
    Roblox::globalShutdown();

    // Set appClosed true before shutdown
    if (RobloxApplicationManager::instance().getApplicationCount() == 1)
    {
        RobloxSettings settings;
        settings.setValue("appClosed", true);
    }
}

void RobloxMainWindow::setupLogging()
{
#if defined(__APPLE__)
    Aya::Log::setLogProvider(&logProvider);
#endif
}

void RobloxMainWindow::causeCrash()
{
    AYACRASH();
}

void RobloxMainWindow::parseCommandLineOptions(const QMap<QString, QString> argMap)
{
    fileLocationArg = argMap[FileLocationArgument];
    urlArg = argMap[AuthUrlArgument];
    ticketArg = argMap[AuthTicketArgument];
    startEventArg = argMap[StartEventArgument];
    readyEventArg = argMap[ReadyEventArgument];
    showEventArg = argMap[ShowEventArgument];
    scriptArg = argMap[ScriptArgument];
    QString sBrowserTrackerId = argMap[BrowserTrackerId];

    QString sWidth = argMap[StudioUtilities::StudioWidthArgument];
    QString sHeight = argMap[StudioUtilities::StudioHeightArgument];

    // convert loadfile('http://www.roblox.com/game/join.ashx')() to just the url
    if (StudioUtilities::containsJoinScript(scriptArg) && scriptArg.contains("loadfile("))
    {
        int urlBegin = scriptArg.indexOf("(") + 2; // skip over the qoute
        int urlEnd = scriptArg.indexOf(")") - 1;
        scriptArg = scriptArg.mid(urlBegin, urlEnd - urlBegin);
    }

    if (argMap[BuildArgument] == "TRUE")
        m_BuildMode = BM_BASIC;
    else if (argMap[IDEArgument] == "TRUE")
        m_BuildMode = BM_ADVANCED;

    if (argMap[TestModeArgument] == "TRUE")
        StudioUtilities::setTestMode(true);
    if (argMap[AvatarModeArgument] == "TRUE")
        StudioUtilities::setAvatarMode(true);

    if (argMap[StudioUtilities::EmulateTouchArgument] == "TRUE")
        StudioDeviceEmulator::Instance().setIsEmulatingTouch(true);

    if (!sWidth.isEmpty())
        StudioDeviceEmulator::Instance().setCurrentDeviceWidth(sWidth.toInt());

    if (!sHeight.isEmpty())
        StudioDeviceEmulator::Instance().setCurrentDeviceHeight(sHeight.toInt());

    if (!sBrowserTrackerId.isEmpty())
        Aya::Stats::setBrowserTrackerId(sBrowserTrackerId.toStdString());
}

void RobloxMainWindow::handleCommandLineOptions()
{
    // make sure sound is disabled if it should be before we create datamodel
    if (StudioUtilities::isTestMode() && !StudioUtilities::isAvatarMode() && StudioUtilities::containsGameServerScript(scriptArg))
    {
        if (AuthoringSettings::singleton().getTestServerAudioBehavior() == AuthoringSettings::OnlineGame)
        {
            Aya::Soundscape::SoundService::soundDisabled = true;
        }
    }

    // check if fileOpen has already been handled
    if (m_fileOpenHandled)
        return;

    bool openedPlace = handleFileOpen(fileLocationArg, IRobloxDoc::IDE, scriptArg);
    if (!openedPlace)
    {
        // if nothing else opened, open a start web page
        actionStartPage->setChecked(true);
    }

    if (openedPlace && isRibbonStyle() && StudioUtilities::isTestMode() && !StudioUtilities::isAvatarMode() &&
        StudioUtilities::containsGameServerScript(scriptArg))
    {
        // start local server, so we can cleanup processes started by 'server'
        RobloxApplicationManager::instance().startLocalServer();
        // this is required to update client server related actions
        UpdateUIManager::Instance().updateToolBars();

        RobloxIDEDoc* pIDEDoc = RobloxDocManager::Instance().getPlayDoc();

        int numPlayers = RobloxSettings().value("rbxRibbonNumPlayer").toInt();
        if (numPlayers > 0)
        {
            if (pIDEDoc && pIDEDoc->getDataModel())
            {
                StartServerAndPlayerVerb* pVerb =
                    dynamic_cast<StartServerAndPlayerVerb*>(pIDEDoc->getDataModel()->getVerb("StartServerAndPlayerVerb"));
                if (pVerb)
                    pVerb->launchPlayers(numPlayers);
            }
        }

        if (pIDEDoc && pIDEDoc->getDataModel())
        {
            pIDEDoc->initServerAudioBehavior();
        }
    }

    if (!openedPlace)
    {
        if (m_splashScreen)
            onDeleteSplashScreen();
    }
}

void RobloxMainWindow::moveEvent(QMoveEvent* event)
{
    RobloxRibbonMainWindow::moveEvent(event);
    updateEmbeddedFindPosition();
}

bool RobloxMainWindow::eventFilter(QObject* watched, QEvent* evt)
{
    if (FFlag::StudioSeparateActionByActivationMethod && evt->type() == QEvent::Shortcut)
    {
        if (QAction* action = dynamic_cast<QAction*>(watched))
        {
            if (commonSlotShortcut(action, !action->isChecked()))
            {
                evt->accept();
                return true;
            }
        }
    }

#ifdef Q_WS_MAC
    // handle special file open command on Mac coming from Finder
    if (evt->type() == QEvent::FileOpen && watched == qApp)
    {
        if (FFlag::StudioFixMacStartPage)
        {
            // Make sure the start page is the first thing opened.
            actionStartPage->setChecked(true);
        }

        m_fileOpenHandled = handleFileOpen(static_cast<QFileOpenEvent*>(evt)->file(), IRobloxDoc::IDE);
        return true;
    }
    else if (isRibbonStyle() && (evt->type() == QEvent::Polish) && watched->inherits("QMenu"))
    {
        static_cast<QMenu*>(watched)->setFont(QApplication::font());
    }
    else
#endif
        if (evt->type() == QEvent::KeyPress || evt->type() == QEvent::ShortcutOverride)
    {
        // ignore shortcut events during busy state
        if (UpdateUIManager::Instance().isBusy())
        {
            evt->accept();
            return true;
        }
    }

    if (evt->type() == QEvent::NonClientAreaMouseButtonPress)
        Studio::Intellesense::singleton().deactivate();

    if ((evt->type() == QEvent::FocusIn || evt->type() == QEvent::FocusOut) && AuthoringSettings::singleton().onlyPlayFocusWindowAudio)
    {
        if (IRobloxDoc* playDoc = RobloxDocManager::Instance().getPlayDoc())
        {
            if (RobloxIDEDoc* ide = dynamic_cast<RobloxIDEDoc*>(playDoc))
            {
                ide->setMuteAllSounds(!QApplication::activeWindow());
                if (QAction* audioToggleWidget = findChild<QAction*>("audioToggleAction"))
                {
                    audioToggleWidget->setChecked(!QApplication::activeWindow());
                }
            }
        }
    }

    if (watched == qApp)
        return qApp->eventFilter(watched, evt);
    else
        return QMainWindow::eventFilter(watched, evt);
}

void RobloxMainWindow::setWindowLayout()
{
    actionFullScreen->setChecked(isFullScreen());
}

void RobloxMainWindow::setupViewMenu()
{
    menuView->insertAction(menuView->actions().value(1), resetViewAction);
    menuView->insertSeparator(resetViewAction);

    // Add dock widgets to view menu
    //  Insert after Start Page.  Start Page is first (index 0).
    UpdateUIManager::Instance().setupDockWidgetsViewMenu(*menuView->actions().value(1));

    // Set up the toolbar
    menuToolBars->addAction(standardToolBar->toggleViewAction());
    menuToolBars->addAction(advToolsToolBar->toggleViewAction());
    menuToolBars->addAction(editCameraToolBar->toggleViewAction());
    menuToolBars->addAction(commandToolBar->toggleViewAction());
    menuToolBars->addAction(runToolBar->toggleViewAction());
    menuToolBars->addAction(viewToolsToolBar->toggleViewAction());
    if (!Aya::MouseCommand::isAdvArrowToolEnabled()) // Remove this, this flag is true
        menuToolBars->addAction(oldToolsToolBar->toggleViewAction());
}

void RobloxMainWindow::toggleFullScreen(bool state)
{
    if (isFullScreen() == state)
        return;

    return;

    // Toggle
    setWindowState(windowState() ^ Qt::WindowFullScreen);

    // Toggle fullscreen glitches with layout on Mac for Ribbon Bar (send event to update layout)
    // #ifdef Q_WS_MAC
    if (isRibbonStyle())
    {
        /*
Qtitan::RibbonPage* pPage = ribbonBar()->getPage(ribbonBar()->currentIndexPage());
if (pPage)
{
    QEvent evt(QEvent::LayoutRequest);
    QApplication::sendEvent(pPage, &evt);
}*/
        // I don't give a fuck
    }
    // #endif

    // if we have an IDE, we need to lock the datamodel
    IRobloxDoc* playDoc = RobloxDocManager::Instance().getPlayDoc();
    if (playDoc)
    {
        RobloxIDEDoc* ide = dynamic_cast<RobloxIDEDoc*>(playDoc);
        if (ide)
        {
            Aya::DataModel::LegacyLock lock(ide->getDataModel(), Aya::DataModelJob::Write);
            Aya::GameBasicSettings::singleton().setFullScreen(state);
            return;
        }
    }

    Aya::GameBasicSettings::singleton().setFullScreen(state);
}

bool RobloxMainWindow::commonSlotHelper(const QString& objectName, bool isChecked)
{
    bool handled = false;

    // action to be handled by the relevant document
    if (RobloxDocManager::Instance().getCurrentDoc())
        handled = RobloxDocManager::Instance().getCurrentDoc()->handleAction(objectName, isChecked);

    // Otherwise handle it ourselves
    if (!handled)
    {

        // Handle global dock actions here
        if (UpdateUIManager::Instance().getDockActionNames().contains(objectName))
        {
            handled = UpdateUIManager::Instance().toggleDockView(objectName);
        }
        else
        {
            // Check if IDE Doc can handle the action
            IRobloxDoc* pPlayDoc = RobloxDocManager::Instance().getPlayDoc();
            if (pPlayDoc)
                handled = pPlayDoc->handleAction(objectName, isChecked);
        }
    }

    // update toolbar status
    UpdateUIManager::Instance().updateToolBars();

    return handled;
}

void RobloxMainWindow::commonSlot(bool isChecked)
{
    const QObject* pSender = sender();
    if (!pSender || dynamic_cast<QuickAccessBarProxyAction*>(sender()))
        return;

    commonSlotHelper(pSender->objectName(), isChecked);
}

bool RobloxMainWindow::commonSlotShortcut(QAction* action, bool isChecked)
{
    return commonSlotHelper(action->objectName(), isChecked);
}

bool RobloxMainWindow::commonSlotQuickAccess(QAction* action)
{
    return commonSlotHelper(action->objectName(), !action->isChecked());
}

void RobloxMainWindow::processAppEvent(void* pClosure)
{
    // Handle the event
    if (sIsAppRunning)
        Aya::FunctionMarshaller::handleAppEvent(pClosure);
    else
        Aya::FunctionMarshaller::freeAppEvent(pClosure); // control flow comes here means application is closed
}

bool RobloxMainWindow::requestDocClose(IRobloxDoc& doc, bool closeIfLastDoc)
{
    if (!doc.getViewer())
    {
        RobloxDocManager::Instance().removeDoc(doc);
        return true;
    }

    if (doc.isModified())
    {
        IRobloxDoc::RBXCloseRequest closeMode = doc.requestClose();

        if (closeMode == IRobloxDoc::CLOSE_CANCELED)
        {
            if (&doc != RobloxDocManager::Instance().getCurrentDoc())
                RobloxDocManager::Instance().setCurrentDoc(&doc);
            return false;
        }
        else if (closeMode == IRobloxDoc::REQUEST_HANDLED)
        {
            // LUA will handle it from here on out
            return false;
        }
        else if (closeMode != IRobloxDoc::NO_SAVE_NEEDED)
        {
            // ask the user if they want to save before close

            QtUtilities::RBXMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Question);
            msgBox.setText("Save changes to " + doc.displayName() + "?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Yes);
            int ret = msgBox.exec();

            // if the user wants to save
            if (ret == QMessageBox::Yes && !fileSave(&doc))
            {
                // User hit the Yes to Save in the Yes No Cancel MsgBox,
                // but then did a Cancel on Save Dialog, change the state to Cancel
                ret = QMessageBox::Cancel;
            }

            // if the user want to cancel close
            if (ret == QMessageBox::Cancel)
            {
                if (&doc != RobloxDocManager::Instance().getCurrentDoc())
                    RobloxDocManager::Instance().setCurrentDoc(&doc);
                return false;
            }
        }
    }

    // special case Start Page since it can be toggled in the View menu
    if (doc.keyName() == "StartPage" && doc.docType() == IRobloxDoc::BROWSER)
        actionStartPage->setChecked(false);
    else if (doc.docType() == IRobloxDoc::OBJECTBROWSER)
        objectBrowserAction->setChecked(false);
#ifdef Q_WS_MAC
    // On Mac, if we do not make the IDE document current then it results in DE7073
    // Combination of QWidget and ViewBase creation coupled with deletion of IDE document
    // without making it current results in non responding keyboard (?)
    else if ((doc.docType() == IRobloxDoc::IDE) && (&doc != RobloxDocManager::Instance().getCurrentDoc()))
        RobloxDocManager::Instance().setCurrentDoc(&doc);
#endif

    // WARNING: Do not use doc to call any function after this (it will get deleted)
    RobloxDocManager::Instance().removeDoc(doc);

    updateWindowTitle();

    if (RobloxDocManager::Instance().getDocCount() == 0 && closeIfLastDoc &&
        (RobloxApplicationManager::instance().getApplicationCount() > 1 || getBuildMode() == BM_BASIC))
    {
        // If there are no tabs remaining and this is another instance of Studio open, close this one
        close();
    }

    return true;
}

void RobloxMainWindow::fileNew()
{
    handleFileOpen(NEW_PLACE_FILENAME, IRobloxDoc::IDE);
}

void RobloxMainWindow::openStartPage(bool checked, QString optionalQueryParams /* = "" */)
{
    if (checked)
    {
        QString fileToOpen;

        const char* startPageUrl = Aya::ClientAppSettings::singleton().GetValueStartPageUrl();
        if (startPageUrl && startPageUrl[0] != '\0')
        {
            // Generate start page url from our recent files -- these files are put into the query string
#ifdef AYA_TEST_BUILD
            QString url = QUrl("http://localhost:5173").toString();
#else
            QString url = QUrl::fromLocalFile(qAssetFolderPath + "/app/index.html").toString();
#endif
            fileToOpen = url.append("?mode=studio");
            RobloxSettings settings;
            QStringList recentFiles = settings.value("rbxRecentFiles").toStringList();
            for (int i = 0; i < qMin(recentFiles.length(), (int)MAX_RECENT_FILES); i++)
            {
                QFileInfo info(recentFiles.at(i));
                fileToOpen.append(i == 0 ? "" : "&")
                    .append("filepath=" + QUrl::toPercentEncoding(info.filePath()) + "&filename=" + QUrl::toPercentEncoding(info.fileName()));
            }
        }
        else
            fileToOpen = QString::fromStdString(GetBaseURL() + "/My/Places.aspx");

        if (!optionalQueryParams.isEmpty())
            fileToOpen.append("&").append(optionalQueryParams);

        // make sure we are authenticated (if there's any authentication happening) before we open the browser
        AuthenticationHelper::Instance().waitForQtWebkitAuthentication();

        handleFileOpen(fileToOpen, IRobloxDoc::BROWSER);
    }
    else
    {
        IRobloxDoc* pDoc = RobloxDocManager::Instance().getOrCreateDoc(IRobloxDoc::BROWSER);
        requestDocClose(*pDoc, false);
    }
}

void RobloxMainWindow::fileOpen()
{
    QString fileName;

    fileName = QFileDialog::getOpenFileName(this, tr("Open Roblox File"), m_LastDirectory, tr(RobloxIDEDoc::getOpenFileFilters()));

    if (fileName.isEmpty())
        return;

    setCurrentDirectory(fileName);
    handleFileOpen(fileName, IRobloxDoc::IDE);
}

void RobloxMainWindow::fileOpenRecentSaves()
{
    QString fileName;

    fileName = QFileDialog::getOpenFileName(
        this, tr("Open Roblox File"), AuthoringSettings::singleton().recentSavesDir.absolutePath(), tr(RobloxIDEDoc::getOpenFileFilters()));

    if (fileName.isEmpty())
        return;

    setCurrentDirectory(fileName);
    handleFileOpen(fileName, IRobloxDoc::IDE);
}

void RobloxMainWindow::fileClose()
{
    IRobloxDoc* doc = RobloxDocManager::Instance().getCurrentDoc();
    if (doc)
        requestDocClose(*doc);
}

bool RobloxMainWindow::verifyFilePermissions(const QString& fileName)
{
    bool retVal = false;

    if (!fileName.isEmpty())
    {
        QFileInfo fileInfo(fileName);
        if (fileInfo.exists() && !fileInfo.isWritable())
        {
            QtUtilities::RBXMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText(fileName + "\nThis file is set to read-only.\nTry again with a different name");
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.exec();
        }
        else
        {
            retVal = true;
        }
    }
    else /// In case of Empty File return it as True as the file is not present and new file should be created
    {
        retVal = true;
    }
    return retVal;
}

bool RobloxMainWindow::fileSave(IRobloxDoc* pDoc)
{
    bool retVal = false;

    // if no document is provided then get the play document.
    if (!pDoc)
        pDoc = RobloxDocManager::Instance().getPlayDoc();

    if (pDoc && pDoc->isModified())
    {
        if (verifyFilePermissions(pDoc->fileName()))
        {
            if (pDoc->fileName().isEmpty() || !pDoc->save())
                retVal = fileSaveAs(pDoc);
            else
                retVal = true;
        }
    }
    return retVal;
}

bool RobloxMainWindow::fileSaveAs(IRobloxDoc* pDoc)
{
    bool retVal = false;

    // if no document is provided then get the play document
    if (!pDoc)
        pDoc = RobloxDocManager::Instance().getPlayDoc();

    if (pDoc)
    {
        QString fileName(pDoc->fileName());
        if (fileName.isEmpty() && boost::filesystem::portable_file_name(pDoc->displayName().toStdString()))
            fileName = m_LastDirectory + "/" + pDoc->displayName();

        fileName = QFileDialog::getSaveFileName(this, tr("Save As"), fileName, pDoc->saveFileFilters());

        if (!fileName.isEmpty() && verifyFilePermissions(fileName) && pDoc->saveAs(fileName))
        {
            setCurrentDirectory(fileName);
            RobloxDocManager::Instance().setDocTitle(*pDoc, pDoc->displayName(), pDoc->titleTooltip(), pDoc->titleIcon());
            updateRecentFile(fileName);

            m_AutoSaveAccum = 0;
            retVal = true;
        }
    }

    return retVal;
}

bool RobloxMainWindow::filePublishedProjects()
{
    QString url = QString(Aya::ClientAppSettings::singleton().GetValuePublishedProjectsPageUrl());
    if (url.isEmpty())
        return false;

    if (!StudioUtilities::checkNetworkAndUserAuthentication())
        return false;

    WebDialog dlg(this, url.prepend(RobloxSettings::getBaseURL()), NULL, Aya::ClientAppSettings::singleton().GetValuePublishedProjectsPageWidth(),
        Aya::ClientAppSettings::singleton().GetValuePublishedProjectsPageHeight());
    dlg.setMinimumSize(FInt::StudioWebDialogMinimumWidth, FInt::StudioWebDialogMinimumHeight);
    dlg.exec();

    return true;
}

bool RobloxMainWindow::openRecentFile(const QString& fileName)
{
    if (!fileName.isEmpty())
    {
        QFileInfo fileInfo(fileName);
        if (!fileInfo.exists())
        {
            updateRecentFile(fileName);

            QtUtilities::RBXMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText(fileName + "\nThis file doesn't exist.");
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.exec();
            return false;
        }
    }

    return handleFileOpen(fileName, IRobloxDoc::IDE);
}

bool RobloxMainWindow::openRecentFile()
{
    QAction* action = qobject_cast<QAction*>(sender());
    QString fileName = action->data().toString();
    if (action)
    {
        return openRecentFile(fileName);
    }
    else
        return false;
}

void RobloxMainWindow::publishGame()
{
    if (!StudioUtilities::checkNetworkAndUserAuthentication())
        return;

    QString initialUrl = QString("%1/IDE/publishgameas").arg(RobloxSettings::getBaseURL());

    if (!m_publishGameDialog)
    {
        m_publishGameDialog = new WebDialog(this, initialUrl, NULL);
        m_publishGameDialog->setMinimumSize(FInt::StudioWebDialogMinimumWidth, FInt::StudioWebDialogMinimumHeight);
    }
    else
    {
        m_publishGameDialog->load(initialUrl);
    }

    m_publishGameDialog->show();
    m_publishGameDialog->raise();
    m_publishGameDialog->activateWindow();
}

void RobloxMainWindow::closePublishGameWindow()
{
    AYAASSERT(m_publishGameDialog);
    if (m_publishGameDialog)
    {
        m_publishGameDialog->close();
    }
}

void RobloxMainWindow::updateRecentFilesUI()
{
    if (isRibbonStyle())
    {
        RibbonPageSystemRecentFileList* pRecentFileList = findChild<RibbonPageSystemRecentFileList*>("recentFileList");
        if (pRecentFileList)
            pRecentFileList->updateRecentFileActions(getRecentFiles());
    }
    else
    {
        updateRecentFilesList(getRecentFiles());
    }
}

void RobloxMainWindow::updateRecentFilesList(const QStringList& files)
{
    int numRecentFiles = qMin(files.size(), (int)MAX_RECENT_FILES_SHOWN);

    for (int i = 0; i < numRecentFiles; ++i)
    {
        QFileInfo fileInfo(files[i]);
        QString fullFilename = fileInfo.absoluteFilePath();
        QString filename = fullFilename;

        if (filename.startsWith(m_LastDirectory))
            filename = filename.right(filename.length() - 1 - m_LastDirectory.length());

        bool isTextModified = false;
        if (filename.length() > MaxLengthFilenameMRU)
        {
            QFontMetrics fm = fontMetrics();
            filename = fm.elidedText(filename, Qt::ElideMiddle, MaxLengthFilenameMRU * fm.horizontalAdvance('0'));
            isTextModified = true;
        }

        recentOpenedFiles[i]->setText(QString("&%1 %2").arg(i + 1).arg(filename));

        if (isTextModified)
            recentOpenedFiles[i]->setToolTip(fullFilename);

        recentOpenedFiles[i]->setData(fullFilename);
        recentOpenedFiles[i]->setStatusTip(fullFilename);
        recentOpenedFiles[i]->setVisible(true);
    }

    for (int j = numRecentFiles; j < (int)MAX_RECENT_FILES_SHOWN; ++j)
        recentOpenedFiles[j]->setVisible(false);

    separator->setVisible(numRecentFiles > 0);
}

void RobloxMainWindow::updateInternalWidgetsState(QAction* pAction, bool enabledState, bool checkedState)
{
    if (isRibbonStyle())
        RobloxRibbonMainWindow::updateInternalWidgetsState(pAction, enabledState, checkedState);
}

#define COPYRIGHT_PREFIX 0xA9

void RobloxMainWindow::about()
{
    QString aboutMsg(tr("<div style=\"text-align: center\">"));
    aboutMsg.append(tr("<p style=\"font-weight: bold\">"));
    aboutMsg.append(tr(AYA_PROJECT_NAME " Studio "));
    aboutMsg.append(tr("Version %1</p>"));

    QMessageBox::about(this, tr("Welcome to " AYA_PROJECT_NAME " Studio!"), aboutMsg);
}


void RobloxMainWindow::shortcutHelp()
{
    if (!m_pInputConfigDialog)
        m_pInputConfigDialog = new RobloxInputConfigDialog(*this);

    if (m_pInputConfigDialog->isVisible())
        m_pInputConfigDialog->hide();
    else
    {
        m_pInputConfigDialog->show();
        m_pInputConfigDialog->raise();
    }
}

void RobloxMainWindow::fastLogDump()
{
    Aya::StandardOut::singleton()->printf(Aya::MESSAGE_INFO, "Log Dumps");
#ifdef _WIN32
    // mainLogManager.CreateFakeCrashDump();
#else
    // CrashReporter::CreateLogDump();
#endif
}

void RobloxMainWindow::instanceDump() {}

void RobloxMainWindow::openPluginsFolder()
{
    QString userPluginPath = RobloxPluginHost::userPluginPath();
    QDir pluginDir(userPluginPath);

    if (!pluginDir.exists())
        pluginDir.mkpath(userPluginPath);

    QDesktopServices::openUrl(QUrl::fromLocalFile(userPluginPath));
}

void RobloxMainWindow::managePlugins()
{
    if (!StudioUtilities::checkNetworkAndUserAuthentication())
        return;

    if (!m_managePluginsDoc)
    {
        m_managePluginsDoc.reset(new RobloxWebDoc(tr("Plugin Management"), "PluginManagement"));
        RobloxWebDoc* webDoc = dynamic_cast<RobloxWebDoc*>(RobloxDocManager::Instance().getOrCreateDoc(IRobloxDoc::BROWSER));
        connect(webDoc->getWorkspace().get(), SIGNAL(PluginInstallComplete(bool, int)), m_managePluginsDoc.get(), SLOT(refreshPage()));
    }
    m_managePluginsDoc->open(this, QString("%1/studio/plugins/manage").arg(RobloxSettings::getBaseURL()));
    QWidget* widgetToAddInTab = m_managePluginsDoc->getViewer();
    if (widgetToAddInTab)
    {
        RobloxDocManager::Instance().configureDocWidget(*m_managePluginsDoc);
    }
}

RobloxWebDoc* RobloxMainWindow::getConfigureWebDoc()
{
    if (!m_configureGameDoc)
    {
        m_configureGameDoc.reset(new RobloxWebDoc(tr("Configure"), "GameEntityConfigure"));
        RobloxDocManager::Instance().getOrCreateDoc(IRobloxDoc::BROWSER);
    }
    // The widget doesn't show up in the tabs unless open() has been called at least once.
    m_configureGameDoc->open(this, "");
    QWidget* widgetToAddInTab = m_configureGameDoc->getViewer();
    if (widgetToAddInTab)
    {
        RobloxDocManager::Instance().configureDocWidget(*m_configureGameDoc);
    }

    return m_configureGameDoc.get();
}

void RobloxMainWindow::closeConfigureWebDoc()
{
    if (m_configureGameDoc)
    {
        requestDocClose(m_configureGameDoc.get());
    }
}

void RobloxMainWindow::openObjectBrowser(bool checked)
{
    if (checked)
    {
        handleFileOpen(QString(""), IRobloxDoc::OBJECTBROWSER);
    }
    else
    {
        IRobloxDoc* pDoc = RobloxDocManager::Instance().getOrCreateDoc(IRobloxDoc::OBJECTBROWSER);
        requestDocClose(*pDoc);
    }
}

void RobloxMainWindow::openSettingsDialog()
{
    if (!m_pSettingsDialog)
        m_pSettingsDialog = new RobloxSettingsDialog(this);
    m_pSettingsDialog->exec();

    // perform any fixup if settings changed
    assignAccelerators();
}

void RobloxMainWindow::executeScriptFile()
{
    AYAASSERT(RobloxDocManager::Instance().getCurrentDoc());
    AYAASSERT(RobloxDocManager::Instance().getCurrentDoc() == RobloxDocManager::Instance().getPlayDoc());

    // check is there's no current document then create one
    if (!RobloxDocManager::Instance().getCurrentDoc())
        fileNew();

    // still no document? can't do anything!
    if (!RobloxDocManager::Instance().getCurrentDoc())
        return;

    // open the script file
    QString fileName;

    fileName = QFileDialog::getOpenFileName(this, tr("Open Script File"), AuthoringSettings::singleton().defaultScriptFileDir.absolutePath(),
        tr("Scripts (*.rbxs *.lua *.txt);;All Files (*.*)"));

    if (fileName.isEmpty())
        return;

    QFileInfo info(fileName);
    AuthoringSettings::singleton().defaultScriptFileDir = info.dir();

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    // read file
    QTextStream inStream(&file);
    QString toExecute = inStream.readAll();

    if (!toExecute.isEmpty())
    {
        if (RobloxIDEDoc* ideDoc = RobloxDocManager::Instance().getPlayDoc())
            ideDoc->handleScriptCommand(toExecute);
    }
}

void RobloxMainWindow::onCustomToolButton(const QString& selectedItem)
{
    const QObject* pSender = sender();
    if (!pSender)
        return;

    // action to be handled by the relevant document
    if (!RobloxDocManager::Instance().getCurrentDoc())
        return;

    QString modifiedActionName(pSender->objectName());
    modifiedActionName.append("_");
    modifiedActionName.append(selectedItem);

    RobloxDocManager::Instance().getCurrentDoc()->handleAction(modifiedActionName);

    // update toolbar status
    UpdateUIManager::Instance().updateToolBars();
}

void RobloxMainWindow::onMenuActionHovered(QAction* action)
{
    if (!action)
        return;

    for (int i = 0; i < MAX_RECENT_FILES_SHOWN; ++i)
    {
        if (action == recentOpenedFiles[i])
        {
            QString tip = action->toolTip();
            if (tip != action->text().remove(0, 1))
                QToolTip::showText(QCursor::pos(), tip);
            else
                QToolTip::hideText();
            break;
        }
    }
}

void RobloxMainWindow::closeEvent(QCloseEvent* evt)
{
    // Aya::Log::current()->writeEntry(Aya::Log::Information,"RobloxMainWindow::closeEvent");

    if (!m_IsInitialized || UpdateUIManager::Instance().isBusy())
    {
        evt->ignore();
        return;
    }

    Studio::Intellesense::singleton().deactivate();

    // first save and close out the play doc if we're in build mode
    if (m_BuildMode == BM_BASIC)
    {
        IRobloxDoc* playDoc = RobloxDocManager::Instance().getPlayDoc();
        if (playDoc && !requestDocClose(*playDoc))
        {
            evt->ignore();
            return;
        }
    }

    // save and close the rest of the docs
    if (!RobloxDocManager::Instance().requestCloseAllDocs())
    {
        evt->ignore();
        return;
    }

    // before close the application, save its states.
    saveApplicationStates();
    // remove event filter
    if (FFlag::StudioInSyncWebKitAuthentication)
        qApp->removeEventFilter(&AuthenticationHelper::Instance());

    hide();

    QMainWindow::closeEvent(evt);
}

/**
 * Close all documents and save locally.
 */
void RobloxMainWindow::saveAndClose()
{
    if (RobloxDocManager::Instance().requestCloseAndSaveAllDocs())
        close();
}

/**
 * Closes all documents but doesn't save.
 */
void RobloxMainWindow::forceClose()
{
    RobloxDocManager::Instance().closeAllDocs();
    close();
}

void RobloxMainWindow::contextMenuEvent(QContextMenuEvent* evt)
{
    // don't show the main window context menu if in basic mode
    if (m_BuildMode != BM_BASIC)
        QMainWindow::contextMenuEvent(evt);
}

void RobloxMainWindow::dragEnterEvent(QDragEnterEvent* evt)
{
    const QMimeData* pMimeData = evt->mimeData();
    if (!pMimeData || !pMimeData->hasUrls())
        return QMainWindow::dragEnterEvent(evt);

    bool isValidFileList = false;

    QList<QUrl> urlList = pMimeData->urls();
    for (int i = 0; i < urlList.size() && i < 6; ++i)
    {
        QString filePath = urlList.at(i).toLocalFile();
        isValidFileList =
            !filePath.isEmpty() && (filePath.endsWith(".rbxl", Qt::CaseInsensitive) || filePath.endsWith(".rbxlx", Qt::CaseInsensitive) ||
                                       filePath.endsWith(".rbxm", Qt::CaseInsensitive) || filePath.endsWith(".rbxmx", Qt::CaseInsensitive));

        if (!isValidFileList)
            break;
    }

    if (!isValidFileList)
        return QMainWindow::dragEnterEvent(evt);

    evt->acceptProposedAction();
}

void RobloxMainWindow::dragMoveEvent(QDragMoveEvent* evt)
{
    evt->acceptProposedAction();
}

void RobloxMainWindow::dropEvent(QDropEvent* evt)
{
    const QMimeData* pMimeData = evt->mimeData();

    if (!pMimeData || !pMimeData->hasUrls())
        return QMainWindow::dropEvent(evt);

    QList<QUrl> urlList = pMimeData->urls();
    for (int i = 0; i < urlList.size() && i < 6; ++i)
    {
        QString fileName = urlList.at(i).toLocalFile();
        if (!fileName.isEmpty())
        {
            if (fileName.endsWith(".rbxl", Qt::CaseInsensitive) || fileName.endsWith(".rbxlx", Qt::CaseInsensitive))
                handleFileOpen(fileName, IRobloxDoc::IDE);
            else
            {
                if (RobloxDocManager::Instance().getCurrentDoc())
                    RobloxDocManager::Instance().getCurrentDoc()->handleDrop(fileName);
            }
        }
    }

    evt->acceptProposedAction();
}

void RobloxMainWindow::dragLeaveEvent(QDragLeaveEvent* evt)
{
    evt->accept();
}

void RobloxMainWindow::initializeUI()
{

    // Apply the global Roblox Studio style sheet to the entire application.
    // Studio.css is where we can tweak and tune the look-and-feel
    // of the Studio UI.  All style overrides that are application-wide
    // should be put there.
    setStyleSheet(QtUtilities::getResourceFileText(DARKMODE ? ":/studio_dark.css" : ":/studio.css"));

    // force height so we don't get any resizing when adding and removing controls to the status bar
    statusBar()->setFixedHeight(32);
    // Apply the global Roblox Studio style sheet to the entire application.

    QString css;

    if (isRibbonStyle())
    {
        css = QtUtilities::getResourceFileText(":/ribbon.css");
    }
    else
    {
        css = QtUtilities::getResourceFileText(DARKMODE ? ":/studio_dark.css" : ":/studio.css");

        menuTools->insertSeparator(openPluginsFolderAction);

        advToolsToolBar->insertAction(advanceJointCreationManualAction, toggleCollisionCheckAction);
        advToolsToolBar->insertSeparator(advanceJointCreationManualAction);

        if (Aya::MouseCommand::isAdvArrowToolEnabled())
            removeToolBar(oldToolsToolBar);

        // Added so that we can dump Crash for Breakpad as needed
        // To enable crashing add following to AppSettings.xml
        //   <CrashMenu>1</CrashMenu>
        if (RobloxSettings::showCrashMenu())
        {
            QAction* crashAction = new QAction("&Crash", this);
            connect(crashAction, SIGNAL(triggered()), this, SLOT(causeCrash()));
            QMenu* debugMenu = menuBar()->addMenu("&Debug");
            debugMenu->addAction(crashAction);
        }

        advToolsToolBar->insertAction(glueSurfaceAction, smoothNoOutlinesAction);
    }

    setStyleSheet(css);
    Studio::Intellesense::singleton().setStyleSheet(css);

    // support drag drop
    setAcceptDrops(true);

    // fix focus issues in Mac
    setFocusPolicy(Qt::StrongFocus);

    // default is max screen
    setWindowState(Qt::WindowMaximized);

    QTimer* minute_timer = new QTimer(this);
    connect(minute_timer, SIGNAL(timeout()), this, SLOT(onMinuteTimer()));
    minute_timer->start(60 * 1000);

    qApp->installEventFilter(this);

    // Initialize the doc manager
    RobloxDocManager::Instance().initialize(*this);
}

void RobloxMainWindow::setupSlots()
{
    // file menu
    connect(menuFile, SIGNAL(hovered(QAction*)), this, SLOT(onMenuActionHovered(QAction*)));
    connect(fileNewAction, SIGNAL(triggered()), this, SLOT(fileNew()));
    connect(fileOpenAction, SIGNAL(triggered()), this, SLOT(fileOpen()));
    connect(fileCloseAction, SIGNAL(triggered()), this, SLOT(fileClose()));
    connect(fileSaveAction, SIGNAL(triggered()), this, SLOT(fileSave()));
    connect(fileSaveAsAction, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
    connect(fileExitAction, SIGNAL(triggered()), this, SLOT(close()));
    connect(publishGameAction, SIGNAL(triggered()), this, SLOT(publishGame()));
    publishGameAction->setEnabled(false);

    connect(fileOpenRecentSavesAction, SIGNAL(triggered()), this, SLOT(fileOpenRecentSaves()));

    // create global shortcuts that should always work even if actions are disabled

    QShortcut* shortcut = new QShortcut(fileExitAction->shortcut(), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(close()));

    shortcut = new QShortcut(toggleBuildModeAction->shortcut(), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(toggleBuildMode()));

    addAction(zoomExtentsAction);
    addAction(toggleLocalSpaceAction);
    addAction(quickInsertAction);
    addAction(explorerFilterAction);
    addAction(propertiesFilterAction);

    if (Aya::ClientAppSettings::singleton().GetValuePublishedProjectsPageUrl() &&
        Aya::ClientAppSettings::singleton().GetValuePublishedProjectsPageUrl()[0] != '\0')
    {
        connect(filePublishedProjectsAction, SIGNAL(triggered()), this, SLOT(filePublishedProjects()));
    }
    else
    {
        filePublishedProjectsAction->setEnabled(false);
        filePublishedProjectsAction->setVisible(false);
    }

    // set up recent open files slots
    for (int i = 0; i < MAX_RECENT_FILES_SHOWN; ++i)
    {
        if (!isRibbonStyle())
        {
            recentOpenedFiles[i] = new QAction(this);
            menuFile->insertAction(fileExitAction, recentOpenedFiles[i]);
            recentOpenedFiles[i]->setVisible(false);
            connect(recentOpenedFiles[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
        }
        else
        {
            recentOpenedFiles[i] = NULL;
        }
    }

    QMenu* pMenu = isRibbonStyle() ? findChild<QMenu*>("switchWindowsMenu") : menuWindow;

    // for window menu open tabs slots
    for (int i = 0; i < MAX_DOC_WINDOWS; ++i)
    {
        currentOpenedfiles[i] = new QAction(this);

        if (pMenu)
            pMenu->addAction(currentOpenedfiles[i]);

        currentOpenedfiles[i]->setVisible(false);
        connect(currentOpenedfiles[i], SIGNAL(triggered()), &RobloxDocManager::Instance(), SLOT(onActivateWindow()));
        currentOpenedfiles[i]->setCheckable(true);
    }

    separator = menuFile->insertSeparator(fileExitAction);

    // reset view
    connect(resetViewAction, SIGNAL(triggered()), this, SLOT(loadDefaultApplicationState()));

    // execute script
    connect(executeScriptAction, SIGNAL(triggered()), this, SLOT(executeScriptFile()));

    // Tools->Settings....
    connect(settingsAction, SIGNAL(triggered()), this, SLOT(openSettingsDialog()));

    // Tools Menu
    instanceDumpAction->setVisible(false);

    connect(openPluginsFolderAction, SIGNAL(triggered()), this, SLOT(openPluginsFolder()));
    connect(managePluginsAction, SIGNAL(triggered()), this, SLOT(managePlugins()));

    // Help Menu
    connect(aboutRobloxAction, SIGNAL(triggered()), this, SLOT(about()));
    connect(shortcutHelpAction, SIGNAL(triggered()), this, SLOT(shortcutHelp()));
    connect(objectBrowserAction, SIGNAL(toggled(bool)), this, SLOT(openObjectBrowser(bool)));
    connect(fastLogDumpAction, SIGNAL(triggered()), this, SLOT(fastLogDump()));

    connect(viewCommandBarAction, SIGNAL(toggled(bool)), commandToolBar, SLOT(setVisible(bool)));

    // Insert Object
    if (FFlag::StudioSeparateActionByActivationMethod)
        connect(quickInsertAction, SIGNAL(triggered()), &UpdateUIManager::Instance(), SLOT(commonSlot()));
    else
        connect(quickInsertAction, SIGNAL(triggered()), &UpdateUIManager::Instance(), SLOT(onQuickInsertFocus()));

    connect(explorerFilterAction, SIGNAL(triggered()), &UpdateUIManager::Instance(), SLOT(filterExplorer()));
    connect(propertiesFilterAction, SIGNAL(triggered()), &UpdateUIManager::Instance(), SLOT(filterProperties()));

    // Cleaup players and servers
    connect(cleanupServersAndPlayersAction, SIGNAL(triggered()), this, SLOT(cleanupPlayersAndServers()));

    // Insert Service
    if (!m_pInsertServiceDlg)
        m_pInsertServiceDlg = new InsertServiceDialog(this);
    connect(insertServiceAction, SIGNAL(triggered()), m_pInsertServiceDlg, SLOT(show()));

    connect(findInScriptsAction, SIGNAL(triggered()), this, SLOT(showFindAllDialog()));

    emulateDeviceAction->setEnabled(true);
    manageEmulationDeviceAction->setEnabled(true);

    viewCommandBarAction->setEnabled(true);
    testCustomStatsAction->setEnabled(FFlag::StudioCustomStatsEnabled);

    QAction* commonActions[] = {rotateSelectionAction, tiltSelectionAction, groupSelectionAction, ungroupSelectionAction, selectChildrenAction,
        moveUpBrickAction, moveDownBrickAction, deleteSelectedAction, selectAllAction, lockAction, glueSurfaceAction,

        // stats menu
        testStatsAction, testRenderStatsAction, testPhysicsStatsAction, testNetworkStatsAction, testSummaryStatsAction, testCustomStatsAction,
        testClearStatsAction,

        smoothSurfaceAction, weldSurfaceAction, studsAction, inletAction, universalsAction, hingeAction, anchorAction, motorRightAction,
        smoothNoOutlinesAction, dropperAction, simulationRunAction, simulationPlayAction, simulationStopAction, simulationResetAction, zoomInAction,
        zoomOutAction, tiltUpAction, tiltDownAction, zoomExtentsAction, panRightAction, panLeftAction, advanceJointCreationManualAction,
        gridToOneAction, gridToOneFifthAction, gridToOffAction, actionFillColor, actionMaterial, advTranslateAction, advRotateAction, resizeAction,
        cutAction, copyAction, pasteAction, duplicateSelectionAction, pasteIntoAction, undoAction, redoAction, playSoloAction, startServerAction,
        startPlayerAction, insertModelAction, insertIntoFileAction, selectionSaveToFileAction, publishToRobloxAction, publishToRobloxAsAction,
        publishGameAction, publishSelectionToRobloxAction, saveToRobloxAction, publishAsPluginAction, createNewLinkedSourceAction, advArrowToolAction,
        toggleAxisWidgetAction, toggle3DGridAction, toggleWireframeRenderingAction, viewDiagnosticsAction, viewTaskSchedulerAction, viewToolboxAction,
        viewBasicObjectsAction, viewScriptPerformanceAction, viewObjectExplorerAction, viewPropertiesAction, viewOutputWindowAction,
        viewFindResultsWindowAction, viewScriptAnalysisAction, gameExplorerAction, toggleCollisionCheckAction, renameObjectAction, unlockAllAction,
        openPluginsFolderAction, toggleLocalSpaceAction, quickInsertAction, explorerFilterAction, propertiesFilterAction, exportSelectionAction,
        exportPlaceAction, unionSelectionAction, negateSelectionAction, separateSelectionAction,

        // script actions
        goToScriptErrorAction, commentSelectionAction, uncommentSelectionAction, toggleCommentAction, expandAllFoldsAction, collapseAllFoldsAction,
        findAction, replaceAction, findNextAction, goToLineAction, findPreviousAction, resetScriptZoomAction, reloadScriptAction,
        neverBreakOnScriptErrorsAction, breakOnAllScriptErrorsAction, breakOnUnhandledScriptErrorsAction, manageEmulationDeviceAction,

        gridSizeToTwoAction, gridSizeToFourAction, gridSizeToSixteenAction};

    int numActions = sizeof(commonActions) / sizeof(QAction*);
    for (int i = 0; i < numActions; ++i)
    {
        Aya::BaldPtr<QAction> action = commonActions[i];
        if (!FFlag::StudioSeparateActionByActivationMethod)
            connect(action, SIGNAL(triggered(bool)), this, SLOT(commonSlot(bool)));

        QList<QKeySequence> shortcuts = action->shortcuts();
        if (!shortcuts.isEmpty())
        {
            // add the shortcuts to the tooltip
            QtUtilities::setActionShortcuts(*action, shortcuts);
        }
        else
        {
            // set status tip
            if (action->statusTip().isEmpty())
                action->setStatusTip(action->toolTip());
        }
    }

    connect(&Roblox::Instance(), SIGNAL(marshallAppEvent(void*, bool)), this, SLOT(processAppEvent(void*)));
    connect(actionStartPage, SIGNAL(toggled(bool)), this, SLOT(openStartPage(bool)));
    connect(actionFullScreen, SIGNAL(toggled(bool)), this, SLOT(toggleFullScreen(bool)));

    QPainter p(this);

    // Set the background to white
    p.fillRect(rect(), DARKMODE ? QColor(45, 45, 45) : Qt::white);

    updateShortcutSet();
}

void RobloxMainWindow::updateShortcutSet()
{
    if (FFlag::DontSwallowInputForStudioShortcuts)
    {
        shortcutSet.clear();
        QObjectList objects = UpdateUIManager::Instance().getMainWindow().children();
        for (QObjectList::iterator iter = objects.begin(); iter != objects.end(); ++iter)
        {
            Aya::BaldPtr<QAction> action = dynamic_cast<QAction*>(*iter);
            if (action && !action->text().isEmpty() && !action->text().startsWith("&"))
            {
                shortcutSet.insert(action->shortcut().toString());
            }
        }
    }
}

bool RobloxMainWindow::isShortcut(const QKeySequence& keySequence)
{
    return (shortcutSet.find(keySequence.toString()) != shortcutSet.end());
}

/**
 * Configure the shortcuts for the actions that are platform specific.
 *  Common actions have different shortcuts on different platforms.  This
 *  function will reconfigure the common action shortcuts for the platform.
 */
void RobloxMainWindow::assignAccelerators()
{
    // file menu
    QtUtilities::setActionShortcuts(*fileNewAction, QKeySequence::keyBindings(QKeySequence::New));
    QtUtilities::setActionShortcuts(*fileOpenAction, QKeySequence::keyBindings(QKeySequence::Open));
    QtUtilities::setActionShortcuts(*fileSaveAction, QKeySequence::keyBindings(QKeySequence::Save));
    QtUtilities::setActionShortcuts(*fileSaveAsAction, QKeySequence::keyBindings(QKeySequence::SaveAs));
    QtUtilities::setActionShortcuts(*fileCloseAction, QKeySequence::keyBindings(QKeySequence::Close));
    QtUtilities::setActionShortcuts(*fileExitAction, QKeySequence::keyBindings(QKeySequence::Quit));

    // edit menu
    QtUtilities::setActionShortcuts(*copyAction, QKeySequence::keyBindings(QKeySequence::Copy));
    QtUtilities::setActionShortcuts(*cutAction, QKeySequence::keyBindings(QKeySequence::Cut));
    QtUtilities::setActionShortcuts(*pasteAction, QKeySequence::keyBindings(QKeySequence::Paste));
    QtUtilities::setActionShortcuts(*redoAction, QKeySequence::keyBindings(QKeySequence::Redo));
    QtUtilities::setActionShortcuts(*undoAction, QKeySequence::keyBindings(QKeySequence::Undo));
    QtUtilities::setActionShortcuts(*selectAllAction, QKeySequence::keyBindings(QKeySequence::SelectAll));

    // set up delete action
    QList<QKeySequence> shortcuts = QKeySequence::keyBindings(QKeySequence::Delete);
    shortcuts.append(QKeySequence(Qt::Key_Backspace));
    QtUtilities::setActionShortcuts(*deleteSelectedAction, shortcuts);

    // script menu
    QtUtilities::setActionShortcuts(*findAction, QKeySequence::keyBindings(QKeySequence::Find));
    QtUtilities::setActionShortcuts(*replaceAction, QKeySequence::keyBindings(QKeySequence::Replace));

    shortcuts.clear();
    shortcuts.append(QKeySequence("F3"));
    QtUtilities::setActionShortcuts(*findNextAction, shortcuts);

    shortcuts.clear();
    shortcuts.append(QKeySequence("Shift+F3"));
    QtUtilities::setActionShortcuts(*findPreviousAction, shortcuts);

    // tools menu
    QtUtilities::setActionShortcuts(*settingsAction, QKeySequence::keyBindings(QKeySequence::Preferences));

    // camera toolbar
    QtUtilities::setActionShortcuts(*zoomInAction, QList<QKeySequence>() << QKeySequence(QKeySequence::ZoomIn) << QKeySequence("Ctrl+="));
    QtUtilities::setActionShortcuts(*zoomOutAction, QList<QKeySequence>() << QKeySequence(QKeySequence::ZoomOut) << QKeySequence("Ctrl+_"));

    shortcuts.clear();
#ifndef Q_WS_MAC
    // TODO: remove shortcut collision for debugger action - stepinto
    QtUtilities::setActionShortcuts(*stepIntoAction, shortcuts);
    // F11 works differently on Mac
    shortcuts.append(QKeySequence("F11"));
#endif
    QtUtilities::setActionShortcuts(*actionFullScreen, shortcuts);

    if (isRibbonStyle())
    {
        // remove shortcut set for startServerAction
        shortcuts.clear();
        QtUtilities::setActionShortcuts(*startServerAction, shortcuts);

        // add shortcut F7 for startServerAndPlayersAction
        QAction* pAction = findChild<QAction*>("startServerAndPlayersAction");
        if (pAction)
        {
            shortcuts.append(QKeySequence("F7"));
            QtUtilities::setActionShortcuts(*pAction, shortcuts);
        }
    }
}

void RobloxMainWindow::showInsertServiceDialog()
{
    if (!m_pInsertServiceDlg)
        m_pInsertServiceDlg = new InsertServiceDialog(this);

    if (RobloxDocManager::Instance().getCurrentDoc())
        RobloxDocManager::Instance().getCurrentDoc()->handleAction("actionUpdateInsertServiceDialog");

    m_pInsertServiceDlg->setVisible(true);
}

void RobloxMainWindow::showFindAllDialog()
{
    FindReplaceProvider::instance().getFindAllDialog()->show();
    FindReplaceProvider::instance().getFindAllDialog()->activateWindow();
    FindReplaceProvider::instance().getFindAllDialog()->setFocus();
}

void RobloxMainWindow::updateEmbeddedFindPosition()
{
    if (IRobloxDoc* doc = RobloxDocManager::Instance().getCurrentDoc())
    {
        if (RobloxScriptDoc* scriptDoc = dynamic_cast<RobloxScriptDoc*>(doc))
        {
            if (ScriptTextEditor* textEditor = scriptDoc->getTextEditor())
            {
                textEditor->updateEmbeddedFindPosition();
            }
        }
    }
}

void RobloxMainWindow::setupCommandToolBar()
{
    m_pScriptComboBox = new ScriptComboBox(commandToolBar);
    commandToolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    commandToolBar->addWidget(m_pScriptComboBox);

    connect(commandToolBar, SIGNAL(topLevelChanged(bool)), this, SLOT(onCommandToolBarTopLevelChanged(bool)));
}


bool RobloxMainWindow::handleFileOpen(const QString& fileName, IRobloxDoc::RBXDocType type, const QString& script)
{
    QString fileToOpen = fileName;

    if (type == IRobloxDoc::IDE)
    {
        // we aren't specifying a file to open or a script to open, we aren't opening anything
        if (fileName.isEmpty() && script.isEmpty())
            return false;

        if (fileName == NEW_PLACE_FILENAME)
        {
            fileToOpen = "";
        }

        // Gross, if it's a build button click, add the avatar (and guis)
        StudioUtilities::setAvatarMode(false);

        if (StudioUtilities::containsVisitScript(script) || StudioUtilities::containsJoinScript(script))
            StudioUtilities::setAvatarMode(true);

        // We already have an IDE doc open, launch a new version of Studio (SDI)
        IRobloxDoc* currentPlayDoc = RobloxDocManager::Instance().getPlayDoc();
        if (currentPlayDoc)
        {
            // If we're opening a branch new place in our new process, send over the new place file name so it can differentiate
            QString fileNameToOpen = fileName.isEmpty() ? NEW_PLACE_FILENAME : fileName;
            RobloxApplicationManager::instance().createNewStudioInstance(script, fileNameToOpen);
            return true;
        }
    }

    IRobloxDoc* newDoc = RobloxDocManager::Instance().getOrCreateDoc(type);

    if (type == IRobloxDoc::IDE)
    {
        // static cast ok because we know we made a IDEDoc due to type
        // only ideDoc needs a script, set it here before we open file
        RobloxIDEDoc* ideDoc = static_cast<RobloxIDEDoc*>(newDoc);
        ideDoc->setInitializationScript(script);

        m_AutoSaveAccum = 0;
    }

    if (!newDoc->open(this, fileToOpen))
    {
        QString text = tr("Error in opening file - %1").arg(fileToOpen);
        if (Aya::StandardOut::singleton() && (!fileToOpen.isEmpty()) && type == IRobloxDoc::IDE)
            Aya::StandardOut::singleton()->print(Aya::MESSAGE_ERROR, qPrintable(text));

        requestDocClose(*newDoc);

        if (type == IRobloxDoc::IDE)
            QMessageBox::critical(this, "Open File Failure", text);
        return false;
    }
    else
    {
        if (Aya::StandardOut::singleton() && (!fileToOpen.isEmpty()) && type == IRobloxDoc::IDE)
            Aya::StandardOut::singleton()->printf(Aya::MESSAGE_INFO, "Successfully opened file - %s", qPrintable(fileToOpen));
    }

    if (fileToOpen.isEmpty() && type == IRobloxDoc::IDE && script.isEmpty())
    {
        RobloxIDEDoc* idedoc = static_cast<RobloxIDEDoc*>(newDoc);
        idedoc->initializeNewPlace();
    }


    QWidget* widgetToAddInTab = newDoc->getViewer();
    if (widgetToAddInTab)
    {
        RobloxDocManager::Instance().configureDocWidget(*newDoc);

        // update build mode with new doc information
        // saveApplicationStates(); // save state in case update reloads it
        UpdateUIManager::Instance().updateBuildMode();
    }

    if (type == IRobloxDoc::IDE && !fileToOpen.isEmpty())
        updateRecentFile(fileToOpen);

    return true;
}

// for making the position of toolbar identical to old studio.
void RobloxMainWindow::setToolbarPosition()
{
    insertToolBar(runToolBar, advToolsToolBar);

    if (Aya::MouseCommand::isAdvArrowToolEnabled())
    {
        insertToolBar(advToolsToolBar, runToolBar);
    }
    else
    {
        insertToolBar(advToolsToolBar, oldToolsToolBar);
        insertToolBar(oldToolsToolBar, runToolBar);
    }

    insertToolBar(runToolBar, editCameraToolBar);
    insertToolBar(editCameraToolBar, viewToolsToolBar);
}

QAction* RobloxMainWindow::getActionByName(const QString& actionName)
{
    QList<QAction*> actionList = actions();
    for (QList<QAction*>::const_iterator iter = actionList.begin(); iter != actionList.end(); ++iter)
        if ((*iter)->objectName() == actionName)
            return *iter;

    return NULL;
}

void RobloxMainWindow::saveDefaultApplicationState()
{
    RobloxSettings settings;

    // save all the positions and sizes for all dockWidgets and toolbars.
    settings.setValue("default_window_state", saveState());
    settings.setValue("default_geometry_state", saveGeometry());
}

void RobloxMainWindow::saveApplicationStates()
{
    // don't save anything in basic mode
    if (m_BuildMode == BM_BASIC)
        return;

    if (!isVisible())
        return;

    UpdateUIManager::Instance().saveDocksGeometry();

    RobloxSettings settings;
    // Clear legacy values
    settings.remove("geometry");
    settings.remove("windowState");

    // save all the positions and sizes for all dockWidgets and toolbars.
    settings.setValue(sGeometryKey, saveGeometry());
    settings.setValue(sWindowStateKey, saveState());

    // Save command history from output window
    settings.setValue("rbxCommandHistory", m_pScriptComboBox->commandHistory());

    // TODO: Move this to RobloxRibbonMainWindow.cpp
    if (isRibbonStyle())
        RobloxQuickAccessConfig::singleton().saveQuickAccessConfig();
}

void RobloxMainWindow::loadDefaultApplicationState()
{
    RobloxSettings settings;
    restoreState(settings.value("default_window_state").toByteArray());
    restoreGeometry(settings.value("default_geometry_state").toByteArray());

    // since players dockwidget and chat dockwidget gets created only after saving application default state so it doesn't get restored
    QDockWidget* playersDockWidget = findChild<QDockWidget*>("playersDockWidget");
    if (playersDockWidget)
        playersDockWidget->setVisible(false);
    QDockWidget* chatDockWidget = findChild<QDockWidget*>("chatDockWidget");
    if (chatDockWidget)
        chatDockWidget->setVisible(false);

    UpdateUIManager::Instance().setDockVisibility(eDW_OBJECT_EXPLORER, true);

    saveApplicationStates();
}

void RobloxMainWindow::loadApplicationStates()
{
    if (isRibbonStyle()) // Ribbon mode saves to its own settings
    {
        sGeometryKey = sGeometryKey.append("_ribbon");
        sWindowStateKey = sWindowStateKey.append("_ribbon");
    }

    // load all the positions and sizes for all dockWidgets and toolbars from memory.
    RobloxSettings settings;
    restoreState(settings.value(sWindowStateKey).toByteArray());
    restoreGeometry(settings.value(sGeometryKey).toByteArray());

    m_pScriptComboBox->setCommandHistory(settings.value("rbxCommandHistory").toStringList());

    UpdateUIManager::Instance().loadDocksGeometry();

    setCurrentDirectory(settings.value("rbxl_last_directory").toString());
}

/**
 * Update the window title to show the current doc name.
 *  If a doc is not selected, show the default.
 *
 * @see getDialogTitle
 */
void RobloxMainWindow::updateWindowTitle()
{
    IRobloxDoc* pDoc = RobloxDocManager::Instance().getCurrentDoc();

    // if no document or we're in basic build mode just use the simple title
    if (!pDoc || m_BuildMode == BM_BASIC)
        setWindowTitle(sWindow_Title);
    else
    {
        setWindowTitle(pDoc->windowTitle() + " - " + QString(sWindow_Title));
    }
}

/**
 * Get the title string to be used for child dialogs.
 *  This is different than the main window's title because the main one can
 *  have the current file opened and [*] in the name.
 *
 * @see updateWindowTitle()
 * @return default child dialog title
 */
QString RobloxMainWindow::getDialogTitle() const
{
    return sDialog_Title;
}

/**
 * Callback every minute.
 */
void RobloxMainWindow::onMinuteTimer()
{
    // prevent re-entrancy
    static bool processing = false;
    if (processing)
        return;
    processing = true;

    // periodically save state in case we crash and lose all the layout changes
    saveApplicationStates();

    // auto-save
    if (AuthoringSettings::singleton().autoSaveEnabled)
    {
        ++m_AutoSaveAccum;

        // correct for bad values
        const int autoSaveMinutes = qMax(AuthoringSettings::singleton().autoSaveMinutesInterval, 1);
        AuthoringSettings::singleton().autoSaveMinutesInterval = autoSaveMinutes;

        if (m_AutoSaveAccum >= autoSaveMinutes)
        {
            m_AutoSaveAccum = 0;
            RobloxIDEDoc* playDoc = RobloxDocManager::Instance().getPlayDoc();
            if (playDoc && !playDoc->autoSave(false))
            {
                const int result = QMessageBox::question(this, tr("Failed to AutoSave"),
                    tr("Failed to AutoSave.  Do you want to temporarily disable AutoSave?"), QMessageBox::Yes | QMessageBox::No);
                if (result == QMessageBox::Yes)
                    AuthoringSettings::singleton().autoSaveEnabled = false;
            }
        }
    }

    processing = false;
}

/**
 * Deletes the splash screen.
 *  After deleting, shows the tip of the day dialog.
 */
void RobloxMainWindow::onDeleteSplashScreen()
{
    if (m_splashScreen)
    {
        delete m_splashScreen;
        m_splashScreen = NULL;
    }
}

void RobloxMainWindow::toggleBuildMode()
{
    if (m_BuildMode == BM_ADVANCED)
        setBuildMode(BM_BASIC);
    else if (m_BuildMode == BM_BASIC)
        setBuildMode(BM_ADVANCED);

    UpdateUIManager::Instance().updateBuildMode();

    // TODO - handle other build modes
}

/**
 * Sets the current mode for building.
 *  Basic mode disables all dock widgets and toolbars.
 *  Advanced mode shows everything.
 */
void RobloxMainWindow::setBuildMode(eBuildMode buildMode)
{
    // saveApplicationStates();

    QSettings settings;

    // if using the default, set it from the settings
    if (buildMode == BM_LASTMODE)
        buildMode = (eBuildMode)settings.value("BuildMode", BM_ADVANCED).toInt();

    m_BuildMode = buildMode;
    settings.setValue("BuildMode", m_BuildMode);
}

void RobloxMainWindow::closePlayDoc()
{
    IRobloxDoc* playDoc = RobloxDocManager::Instance().getPlayDoc();
    if (playDoc)
        requestDocClose(playDoc);
}

/**
 * Callback for the command toolbar changing floating/docking state.
 *  Handles setting the maximum size of the script input combobox so it works as expected.
 */
void RobloxMainWindow::onCommandToolBarTopLevelChanged(bool topLevel)
{
    if (topLevel)
        m_pScriptComboBox->setFixedWidth(800);
    else
    {
        m_pScriptComboBox->setMinimumWidth(300);
        m_pScriptComboBox->setMaximumWidth(QWIDGETSIZE_MAX);
    }

    commandToolBar->layout()->invalidate();
}

void RobloxMainWindow::onPropertyChanged(const Aya::Reflection::PropertyDescriptor* pDescriptor)
{
    if (AuthoringSettings::singleton().getOutputLayoutMode() == AuthoringSettings::OutputLayoutHorizontal)
    {
        setCorner(Qt::TopLeftCorner, Qt::TopDockWidgetArea);
        setCorner(Qt::TopRightCorner, Qt::TopDockWidgetArea);
        setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
        setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);
    }
    else
    {
        setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
        setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
    }

    if (AuthoringSettings::singleton().diagnosticsBarEnabled)
    {
        UpdateUIManager::Instance().updateStatusBar();
    }
}

void RobloxMainWindow::setupCustomToolButton()
{
    m_pFillColorToolButton = new FillColorPickerToolButton(this);
    m_pFillColorToolButton->setObjectName("actionFillColor");
    m_pFillColorToolButton->setDefaultAction(actionFillColor);
    connect(m_pFillColorToolButton, SIGNAL(changed(const QString&)), this, SLOT(onCustomToolButton(const QString&)));

    m_pMaterialToolButton = new MaterialPickerToolButton(this);
    m_pMaterialToolButton->setObjectName("actionMaterial");
    m_pMaterialToolButton->setDefaultAction(actionMaterial);
    connect(m_pMaterialToolButton, SIGNAL(changed(const QString&)), this, SLOT(onCustomToolButton(const QString&)));

    // add tool buttons into toolbar
    QAction* fillAction = advToolsToolBar->insertWidget(dropperAction, m_pFillColorToolButton);
    fillAction->setObjectName("actionFillColor_WidgetAction");

    QAction* materialAction = advToolsToolBar->insertWidget(smoothSurfaceAction, m_pMaterialToolButton);
    materialAction->setObjectName("actionMaterial_WidgetAction");

    advToolsToolBar->insertSeparator(materialAction);
    advToolsToolBar->insertSeparator(smoothSurfaceAction);
}


void RobloxMainWindow::cleanupPlayersAndServers()
{
    RobloxApplicationManager::instance().cleanupChildProcesses();
    cleanupServersAndPlayersAction->setEnabled(false);
}

void RobloxMainWindow::onIDEDocViewInitialized()
{
    if (m_splashScreen)
        onDeleteSplashScreen();
}

void RobloxMainWindow::cookieConstraintCheckerLoadFinished(bool ok)
{
    AYAASSERT(m_cookieConstraintChecker);
    if (!m_cookieConstraintChecker)
        return;

    if (ok && m_cookieConstraintChecker->url().toString().contains(FString::StudioCookieConstraintUrlFragment.c_str(), Qt::CaseInsensitive))
    {
        m_cookieConstraintChecker->raise();
        m_cookieConstraintChecker->show();
    }
    else
    {
        m_cookieConstraintChecker->hide();
        disconnect(m_cookieConstraintChecker, SIGNAL(loadFinished(bool)), this, SLOT(cookieConstraintCheckerLoadFinished(bool)));
        m_cookieConstraintChecker->deleteLater();
        m_cookieConstraintChecker = NULL;
        m_cookieConstraintCheckDone->exit();
    }
}
