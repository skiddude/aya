

#pragma once

// Qt Headers
#include <QMutex>
#include <QEvent>
#include <QThread>
#include <QWaitCondition>
#include <QStringList>
#include <QBitArray>
#include <QTimer>
#include <QMainWindow>
#include <QComboBox>
#include <QString>
#include <QTime>

// Boost Headers
#include <boost/scoped_ptr.hpp>

// Roblox Headers
#include "DataModel/StudioPluginHost.hpp"

#include "Debug.hpp"

#include "BaldPtr.hpp"

#include "Tool/DragTypes.hpp"

#include "Roblox.hpp"
#include "Utility/Http.hpp"
#include "AppSettings.hpp"

// Roblox Studio Headers
#include "ui_RBXMainWindow.h"
#include "IRobloxDoc.hpp"
#include "ScriptComboBox.hpp"
#include "RobloxRibbonMainWindow.hpp"

static const char* FileLocationArgument = "-fileLocation"; // File to open
static const char* ScriptArgument = "-script";             // Script to execute (LUA)
static const char* AuthUrlArgument = "-url";               // Url to hit to authenticate (passing auth from website)
static const char* AuthTicketArgument = "-ticket";         // Ticket to tack onto url to authenticate (passing auth from website)
static const char* StartEventArgument = "-startEvent";     // Callback to notify launcher of start of application
static const char* ReadyEventArgument = "-readyEvent";     // Callback to notify launcher that application is loaded and ready
static const char* ShowEventArgument = "-showEvent";       // Event to wait for before showing the window
static const char* TestModeArgument = "-testMode";         // Play solo, Start Server, and Start Player
static const char* IDEArgument = "-ide";                   // Indicates launching studio in advanced mode (full IDE)
static const char* BuildArgument = "-build";               // Indicates launching studio in build mode (limited IDE docks)
static const char* DebuggerArgument = "-debugger";         // Causes the program to wait for a debugger to attach on startup
static const char* AvatarModeArgument = "-avatar";         // Avatar Mode sets up the in-game GUI with an avatar to run around with
static const char* RbxDevArgument =
    "-rbxdev"; // RbxDev starts the mobile development deployer (currently in development as of 1/17/2014), allows mobile devices to connect to a game
static const char* BrowserTrackerId = "-browserTrackerId"; // Passed in from website used to log launch status

static const char* UrlArgument = "-url";
static const char* TicketArgument = "-ticket";
static const char* EventArgument = "-event";
static const char* LaunchPlayerArgument = "-launchPlayerMode";

enum LaunchMode
{
    Play,
    Play_Protocol,
    Build,
    Edit
};
class QLabel;
class QSplashScreen;
class QWebEngineView;

class CustomToolButton;
class RobloxInputConfigDialog;
class RobloxPropertyWidget;
class RobloxPluginHost;
class RobloxSettingsDialog;
class RobloxDiagnosticsView;
class RobloxScriptReview;
class RobloxTaskScheduler;
class RobloxWebDoc;
class InsertObjectWidget;
class InsertServiceDialog;
class RobloxToolBox;
class RobloxTextOutputWidget;
class ShortcutHelpDialog;
class WebDialog;

namespace Aya
{
class CEvent;
} // namespace Aya

enum eBuildMode
{
    BM_BASIC,    // build mode
    BM_ADVANCED, // edit mode
    BM_LASTMODE,
};

struct InsertObjectItem
{
    std::string contentId;
    std::vector<weak_ptr<Aya::Instance>> weakInstances;

    InsertObjectItem(std::string contentId, std::vector<weak_ptr<Aya::Instance>> weakInstances)
    {
        this->contentId = contentId;
        this->weakInstances = weakInstances;
    }
};

class RobloxMainWindow : public RobloxRibbonMainWindow
{
    Q_OBJECT

public:
    static const int MAX_DOC_WINDOWS = 10; // the app will only remember the first opened doc windows in window menu.

    RobloxMainWindow(const QMap<QString, QString> argMap);
    virtual ~RobloxMainWindow();

    QStackedWidget& treeWidgetStack() const
    {
        return *stackedWidget;
    }
    CustomToolButton& materialToolButton() const
    {
        return *m_pMaterialToolButton;
    }
    CustomToolButton& fillColorToolButton() const
    {
        return *m_pFillColorToolButton;
    }
    RobloxPluginHost& pluginHost() const
    {
        return *m_pPluginHost;
    }
    QAction& fullScreenAction() const
    {
        return *actionFullScreen;
    }
    void enableScriptCommandInput(bool enabled)
    {
        m_pScriptComboBox->setEnabled(enabled);
    }
    void setGridMode(Aya::DRAG::DraggerGridMode gridMode);

    static RobloxMainWindow* get(QObject* context);

    QString getDialogTitle() const;

    InsertServiceDialog& insertServiceDialog() const
    {
        return *m_pInsertServiceDlg;
    }

    void saveApplicationStates();
    void loadApplicationStates();

    bool requestDocClose(IRobloxDoc& doc, bool closeIfLastDoc = true);
    Q_INVOKABLE void closePlayDoc();
    Q_INVOKABLE void saveAndClose();
    Q_INVOKABLE void forceClose();

    // pass pointer in invokable mode since making it a metatype is a pain
    Q_INVOKABLE bool requestDocClose(IRobloxDoc* doc)
    {
        return requestDocClose(*doc);
    }

    bool handleFileOpen(const QString& fileName, IRobloxDoc::RBXDocType type, const QString& script = "");

    void updateWindowTitle();
    void updateRecentFilesList(const QStringList& files);
    void updateInternalWidgetsState(QAction* pAction, bool enabledState, bool checkedState);

    void setToolbarPosition();
    RobloxWebDoc* getConfigureWebDoc();
    void closeConfigureWebDoc();
    void closePublishGameWindow();

    RobloxTextOutputWidget* getOutputWidget()
    {
        return m_pTextOutput;
    }

    RobloxPluginHost* getPluginHost() const
    {
        return m_pPluginHost;
    }
    void setPluginHost(RobloxPluginHost* val)
    {
        m_pPluginHost = val;
    }

    eBuildMode getBuildMode() const
    {
        return m_BuildMode;
    }
    bool isRibbonStyle() const
    {
        return m_isRibbon;
    }

    QAction* getActionByName(const QString& actionName);

    QAction* currentOpenedfiles[MAX_DOC_WINDOWS];

    bool commonSlotShortcut(QAction* action, bool isChecked);

    void onIDEDocViewInitialized();

    void updateShortcutSet();
    bool isShortcut(const QKeySequence& keySequence);

    void updateEmbeddedFindPosition();

    virtual void moveEvent(QMoveEvent* event);

public Q_SLOTS:

    bool fileSave(IRobloxDoc* pDoc = NULL);

    bool openRecentFile(const QString& fileName);
    void openStartPage(bool checked, QString optionalQueryParams = "");

    // common slot for handling document related actions
    void commonSlot(bool isChecked);

    bool commonSlotQuickAccess(QAction* action);

private Q_SLOTS:

    // function marshaling
    void processAppEvent(void* pClosure);

    void fileNew();
    void fileOpen();
    void fileClose();
    bool fileSaveAs(IRobloxDoc* pDoc = NULL);
    bool filePublishedProjects();
    bool openRecentFile();
    void publishGame();
    void fileOpenRecentSaves();

    void showInsertServiceDialog();
    void showFindAllDialog();

    void instanceDump();

    void openSettingsDialog();
    void openPluginsFolder();
    void managePlugins();

    void about();
    void shortcutHelp();
    void openObjectBrowser(bool checked);
    void fastLogDump();

    void toggleFullScreen(bool change);

    void executeScriptFile();

    void causeCrash();

    void onCustomToolButton(const QString& selectedItem);

    void onMenuActionHovered(QAction* action);

    void onMinuteTimer();

    void onDeleteSplashScreen();

    void toggleBuildMode();

    void onCommandToolBarTopLevelChanged(bool topLevel);

    void loadDefaultApplicationState();

    void cleanupPlayersAndServers();

    void cookieConstraintCheckerLoadFinished(bool ok);

private:
    std::set<QString> shortcutSet;

    virtual bool eventFilter(QObject* watched, QEvent* evt);
    virtual void closeEvent(QCloseEvent* evt);
    virtual void contextMenuEvent(QContextMenuEvent* evt);

    // drag-drop related override
    virtual void dragEnterEvent(QDragEnterEvent* evt);
    virtual void dragMoveEvent(QDragMoveEvent* evt);
    virtual void dropEvent(QDropEvent* evt);
    virtual void dragLeaveEvent(QDragLeaveEvent* evt);

    void setupLogging();
    void setupViewMenu();
    void setupSlots();
    void setupCommandToolBar();
    void initializeUI();

    void assignAccelerators();

    void setWindowLayout();

    void parseCommandLineOptions(const QMap<QString, QString> argMap);
    void handleCommandLineOptions();
    bool verifyFilePermissions(const QString& fileName);

    QAction& getViewAction(EDockWindowID ID) const;

    void setBuildMode(eBuildMode buildMode);
    void setupCustomToolButton();

    void onPropertyChanged(const Aya::Reflection::PropertyDescriptor* pDescriptor);

    void saveDefaultApplicationState();

    void updateRecentFilesUI();

    bool commonSlotHelper(const QString& objectName, bool isChecked);

    // Args that may be passed in
    QString fileLocationArg;
    QString scriptArg;
    QString urlArg;
    QString ticketArg;
    QString startEventArg;
    QString showEventArg;
    QString readyEventArg;

    ScriptComboBox* m_pScriptComboBox;
    CustomToolButton* m_pFillColorToolButton;
    CustomToolButton* m_pMaterialToolButton;

    Aya::BaldPtr<InsertServiceDialog> m_pInsertServiceDlg;
    Aya::BaldPtr<RobloxSettingsDialog> m_pSettingsDialog;
    Aya::BaldPtr<ShortcutHelpDialog> m_pShortcutHelpDialog;
    Aya::BaldPtr<RobloxInputConfigDialog> m_pInputConfigDialog;
    Aya::BaldPtr<QSplashScreen> m_splashScreen;

    RobloxPluginHost* m_pPluginHost;
    boost::scoped_ptr<RobloxWebDoc> m_managePluginsDoc;
    boost::scoped_ptr<RobloxWebDoc> m_configureGameDoc;
    WebDialog* m_publishGameDialog;
    AppSettings* m_appSettings;

    // Application state args
    static QString sGeometryKey;
    static QString sWindowStateKey;
    static bool sIsAppRunning;

    static const QString
        NEW_PLACE_FILENAME; // Used to indicate to the new process (when opening multiple docs) that this is an empty place we're opening
    static const int MAX_RECENT_FILES = 6; // the app will remember the 15 most recent opened files, but only show the 6 latest in the file menu
    static const int MAX_RECENT_FILES_SHOWN = 6;

    QAction* recentOpenedFiles[MAX_RECENT_FILES];
    QAction* separator; // a separator for recently opened files.
    QTimer* m_pMinutesPlayedTimer;

    Aya::signals::scoped_connection m_PropertyChangedConnection;

    RobloxTextOutputWidget* m_pTextOutput;


    eBuildMode m_BuildMode;

    WebDialog* m_pPublishedProjectsWebDialog;

    bool m_IsInitialized;
    int m_AutoSaveAccum;

    bool m_isRibbon; // whether we're in ribbon bar mode or classic
    bool m_fileOpenHandled;
    QWebEngineView* m_cookieConstraintChecker;
    QEventLoop* m_cookieConstraintCheckDone;

    std::queue<InsertObjectItem> m_insertObjectItems;

    // TODO - fix this, friends bad
    friend class RbxWorkspace;
    friend class ShutdownClientVerb;
    friend class RobloxBrowser;
};
