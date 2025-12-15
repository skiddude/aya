

#pragma once

// Qt Headers
#include <QObject>
#include <QString>
#include <QMutex>
#include <QMap>
#include <QVector>
#include <QAction>
#include <QFrame>
#include <QLabel>
#include <QPushButton>

// Roblox Headers
#include "Base/ViewBase.hpp"
#include "DataModel/Game.hpp"
#include "GrayChatBar.hpp"
#include "Tool/DragTypes.hpp"
#include "DataModel/PlayerGui.hpp"
#include "signal.hpp"

#include "BaldPtr.hpp"

#include "Utility/G3DCore.hpp"

#include "Tool/DragTypes.hpp"


// Roblox Studio Headers
#include "RobloxBasicDoc.hpp"
#include "SelectionHighlightAdornable.hpp"

class XmlElement;
class RobloxView;
class RobloxTreeWidget;
class QOgreWidget;
class RobloxMainWindow;
class RobloxDiagnosticsView;
class EntityProperties;

class WorkspaceAnnouncementTooltip : public QFrame
{
    Q_OBJECT
public:
    WorkspaceAnnouncementTooltip(QWidget* parent);

    void showText();
    void hideText();

private:
    QPushButton* closeButton;
    QLabel* toolTipLabel;
    QString lastClosedMessage;

    void setLastClosedMessage(const QString& messageID)
    {
        lastClosedMessage = messageID;
    }
    QString& getLastClosedMessage()
    {
        return lastClosedMessage;
    }

private Q_SLOTS:
    void closeText();
};

namespace Aya
{

namespace Graphics
{
class Framebuffer;
}

class Verb;
class Name;
class DataModel;
class SelectionChanged;
class Heartbeat;
class Instance;
class ViewBase;
class ViewRbxGfx;
class LuaSourceContainer;

namespace Reflection
{
class Variant;
class PropertyDescriptor;
} // namespace Reflection
namespace Scripting
{
class ScriptDebugger;
}
} // namespace Aya

class RecentlySavedFilesHandler;

typedef QMap<QString, const Aya::Name*> tActionIDVerbMap;

struct GameState
{
    shared_ptr<Aya::Game> m_Game;
    QVector<Aya::Verb*> m_Verbs;
    tActionIDVerbMap m_VerbMap;
    Aya::BaldPtr<RobloxView> m_View;
    Aya::BaldPtr<RobloxTreeWidget> m_TreeWidget;
    Aya::BaldPtr<Aya::Verb> m_UndoVerb;
    Aya::BaldPtr<Aya::Verb> m_RedoVerb;
    Aya::signals::scoped_connection m_ChangeHistoryConnection;
    SelectionHighlightAdornable m_SelectionHighlightAdornable;
};

class RobloxIDEDoc
    : public QObject
    , public RobloxBasicDoc
{
    Q_OBJECT

public:
    RobloxIDEDoc(RobloxMainWindow* pMainWindow);
    virtual ~RobloxIDEDoc();

    bool openFile(const QString& fileName, bool asNew);
    bool openStream(const QString& fileName, std::istream* stream, bool asNew);

    bool open(RobloxMainWindow* pMainWindow, const QString& fileName);

    void displayWorkspaceMessage();

    void initializeNewPlace();

    void initializeRobloxView();
    void showChatBar();
    void hideChatBar();
    void onChatBarEnteredText(const QString& text);
    void focusChatBar();
    bool chatBarShown() const;

    IRobloxDoc::RBXCloseRequest requestClose();

    IRobloxDoc::RBXDocType docType()
    {
        return IRobloxDoc::IDE;
    }

    QString fileName() const
    {
        return m_fileName;
    }
    QString displayName() const;
    QString windowTitle() const;
    QString keyName() const
    {
        return "RobloxIDEDoc";
    }
    QString initializationScript() const
    {
        return m_initializationScript;
    }
    virtual const QIcon& titleIcon() const;
    virtual const QString& titleTooltip() const;

    QWidget* getViewer();

    Aya::DataModel* getDataModel() const
    {
        if (m_CurrentGame && m_CurrentGame->m_Game && m_CurrentGame->m_Game->getDataModel())
            return m_CurrentGame->m_Game->getDataModel().get();
        return NULL;
    }
    shared_ptr<Aya::DataModel> getEditDataModel() const
    {
        return m_EditGame.m_Game ? m_EditGame.m_Game->getDataModel() : shared_ptr<Aya::DataModel>();
    }

    bool isPlaySolo()
    {
        return (m_PlayGame.m_Game && m_PlayGame.m_Game->getDataModel());
    }

    bool isModified();
    bool isSimulating();
    bool isLocalDoc()
    {
        return m_bIsLocalDocument;
    }

    bool save();
    bool saveAs(const QString& filename);
    bool autoSave(bool force);
    void setAutoSaveLoad();

    void resetDirty(Aya::DataModel* dataModel);

    static const char* getOpenFileFilters();
    static const char* getSaveFileFilters();
    QString openFileFilters()
    {
        return getOpenFileFilters();
    }
    QString saveFileFilters()
    {
        return getSaveFileFilters();
    }

    // Material Properties
    static bool displayAskConvertPlaceToNewMaterialsIfInsertNewModel();
    void onWorkspacePropertyChanged(const Aya::Reflection::PropertyDescriptor* pDescriptor);

    void activate();
    void deActivate();

    virtual bool doHandleAction(const QString& actionID, bool isChecked);
    bool actionState(const QString& actionID, bool& enableState, bool& checkedState);

    bool handleDrop(const QString& fileName);

    bool handlePluginAction(void* pNotifier, void* pAction);

    void handleScriptCommand(const QString& execCommand);
    void setInitializationScript(const QString& script)
    {
        m_initializationScript = script;
    }

    void onDMItemChanged(shared_ptr<Aya::Instance> item, const Aya::Reflection::PropertyDescriptor* descriptor);

    void enableUndo(bool enable);

    void publish();
    void setFullScreen(bool fullScreen);

    Aya::Reflection::Variant evaluateCommandBarItem(const char* itemToEvaluateshared, shared_ptr<Aya::LuaSourceContainer> script);

    Aya::Verb* getVerb(const QString& name) const;

    void initializeDataModeHash();

    void teleportToURL(QString url, bool play);
    void loadPlayDataModel(QString url, bool play, bool cloneDataModel, bool isTeleport = false);
    void closePlayDataModel();

    void notifyScriptEdited(shared_ptr<Aya::Instance> modifiedScript);

    static bool isEditMode(const Aya::Instance* context);

    void forceViewSize(QSize viewSize);

    void setReopenLastSavedPlace(bool reopenLastSavedPlace)
    {
        m_ReopenLastSavedPlace = reopenLastSavedPlace;
    }
    void updateDisplayName(const QString& newDisplayName);

    static const char* lastOpenedPlaceFileSetting()
    {
        return "LastOpenedPlaceFile";
    }
    static const char* lastOpenedPlaceScriptSetting()
    {
        return "LastOpenedPlaceScript";
    }
    static const char* lastOpenedPlaceUserID()
    {
        return "LastOpenedPlaceUserID";
    }

    void forceReloadImages(const QStringList& contentIds);

    void initServerAudioBehavior();
    void setMuteAllSounds(bool mute);

    void promptForExistingAssetId(
        const std::string& assetType, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);
    boost::shared_ptr<Aya::Instance> getEditScriptByPlayInstance(Aya::Instance* instance);

public Q_SLOTS:
    void exportSelection(std::string filePath = "", Aya::ExporterSaveType exportType = Aya::ExporterSaveType_Selection);
    void exportPlace()
    {
        exportSelection("", Aya::ExporterSaveType_Everything);
    }
    void updateFromPlaceID(int placeId);
    void refreshDisplayName();

private Q_SLOTS:
    void onDelayedInitializeDebuggerData();
    void showImportAssetDialog(QVariant resumeFunctionVar, QVariant errorFunctionVar);

private:
    virtual bool doClose();
    bool loadFromStream(std::istream* stream);

    void createActionIDVerbMap(GameState& gameState);

    void mapFilesMenu(GameState& gameState);
    void mapEditMenu(GameState& gameState);
    void mapViewMenu(GameState& gameState);
    void mapFormatMenu(GameState& gameState);
    void mapToolMenu(GameState& gameState);
    void mapAdvToolsToolbar(GameState& gameState);
    void mapRunToolbar(GameState& gameState);
    void mapCameraToolbar(GameState& gameState);
    void mapStandardToolbar(GameState& gameState);
    void mapAdvanceBuildToolbar(GameState& gameState);
    void mapInsertMenu(GameState& gameState);

    void preProcessVerb(const QStringList& tokens);
    void applyScriptChanges();

    void onIdeRun(bool play);
    void onIdePause();
    void onIdeReset();
    bool isIdeRunEnabled(bool play);
    bool isIdePauseEnabled();
    bool isIdeResetEnabled();

    bool isObjectRenameEnabled();
    void onObjectRename();

    void onWaypointChanged();
    Q_INVOKABLE void updateUndoRedo();

    void openUrl(const std::string url);
    void InitializeDefaultsFromSettings();

    void handleVirtualVersionChange(Aya::GameBasicSettings::VirtualVersion version);

    void setDockViewsEnabled(bool state = true);
    void preFetchResources();

    void toggleStats(const QString& stats);
    void setStatsEnabled(const QString& stats, bool enabled, QAction* verbAction = NULL);

    void createOptionalDMServices();

    void setGridMode(Aya::DRAG::DraggerGridMode gridMode);

    void setDraggerGridMode(Aya::DRAG::DraggerGridMode gridMode);
    void setGridSize(int gridSize);

    void mapActionIDWithVerb(GameState& gameState, const QString& actionID, const Aya::Name& verbName);
    void mapActionIDWithVerb(GameState& gameState, const QString& actionID, const Aya::Verb* pVerb);
    void mapActionIDWithVerb(GameState& gameState, const QString& actionID, const char* verbName);
    Aya::Verb* getVerb(GameState& gameState, const QString& name) const;

    void cleanupGameState(GameState& gameState);

    void onContentDataLoaded();
    void onContentDataSaved();

    QString getUploadURL();
    QString getDebugInfoFile(bool isAutoSaveFile = false, const QString& debuggerFileExt = QString());

    bool saveScriptsChangedWhileTesting();
    void patchPlayModelScripts();
    void restoreEditModelScripts();
    void createScriptMapping(shared_ptr<Aya::Instance> instance);

    // @mdolli
    void takeThumbnail(char* thumbnail, size_t& thumbnailSize, size_t bufferSize);

    class PlayModifiedScriptItem
    {
    public:
        PlayModifiedScriptItem() {}

        boost::shared_ptr<Aya::Instance> instance;
        Aya::signals::scoped_connection ancestryChangedConnection;
    };

    typedef std::list<PlayModifiedScriptItem*> tPlayModifiedScriptList;
    typedef std::map<Aya::Instance*, boost::shared_ptr<Aya::Instance>> tScriptMap;

    void onPlayModifiedAncestryChanged(boost::shared_ptr<Aya::Instance> script, boost::shared_ptr<Aya::Instance> newParent);

    tPlayModifiedScriptList& getPlayModifiedScriptList()
    {
        return m_playModifiedScriptList;
    }
    void clearPlayModifiedScriptList();

    tScriptMap& getScriptMap()
    {
        return m_scriptMap;
    }
    void addScriptMapping(shared_ptr<Aya::Instance> script, shared_ptr<Aya::DataModel> playDataModel);
    void clearScriptMap()
    {
        m_scriptMap.clear();
    }

    std::string getScriptIndexHierarchy(shared_ptr<Aya::Instance>, std::vector<int>& indexHierarchy);
    shared_ptr<Aya::Instance> getScriptByIndexHierarchy(
        shared_ptr<Aya::DataModel> dataModel, const std::vector<int>& indexHierarchy, const std::string& serviceName);

    Q_INVOKABLE void placeNameLoaded(QString json);

    void resetMouseCommand(const char* mouseCommandToReset);

    struct DebuggerData
    {
        struct BreakpointData
        {
            int line;
            bool enabled;
            // std::string condition;
        };

        struct BreakpointLineNumberOrder
        {
            bool operator()(const BreakpointData& lhs, const BreakpointData& rhs) const
            {
                return lhs.line < rhs.line;
            }
        };

        std::list<BreakpointData> breakpoints;
        std::list<std::string> watches;
    };
    typedef std::map<boost::shared_ptr<Aya::Instance>, DebuggerData> tDebuggersMap;
    void saveEditModelDebuggerData();
    void updateEditModelDebuggerData();

    void initializeDebuggerData(const QString& fileName, const shared_ptr<Aya::DataModel> dm, const tDebuggersMap& debuggersMap = tDebuggersMap());
    void updateDebuggerManager(const tDebuggersMap& debuggers);
    void onCoreGuiChanged(Aya::StarterGuiService::CoreGuiType type, bool enabled);
    void updateDebuggersMap(shared_ptr<Aya::Instance> instance, tDebuggersMap& debuggersMap);
    void addToDebuggersMap(shared_ptr<Aya::Instance> script, tDebuggersMap& debuggersMap);
    DebuggerData getDebuggerData(Aya::Scripting::ScriptDebugger* debugger);

    QString getRecentSaveFile();
    void clearOutputWidget();

    void searchToolboxByDefaultSearchString();
    void onLocalPlayerAdded(shared_ptr<Aya::Instance> newPlayer);
    void onLocalPlayerNameChanged(const Aya::Reflection::PropertyDescriptor* pDescriptor);
    Q_INVOKABLE void syncPlayerName();

    void appendTimeStampToDisplayName();
    void removeTimeStampFromDisplayName();

    void updateOnSimulationStateChange(bool run);

    GameState m_EditGame;
    GameState m_PlayGame;
    GameState* m_CurrentGame;

    Aya::signals::scoped_connection m_ChangeHistoryConnection;
    Aya::signals::scoped_connection m_openUrlConnection;
    Aya::signals::scoped_connection m_contentDataLoadedConnection;
    Aya::signals::scoped_connection m_localPlayerConnection;
    Aya::signals::scoped_connection m_workspacePropertyChangedConnection;
    Aya::signals::scoped_connection m_dataModelItemChangedConnection;


    boost::shared_ptr<Aya::ViewBase> m_GameView;

    QWidget* m_WrapperWidget;
    QOgreWidget* m_pQOgreWidget;
    std::unique_ptr<GrayChatBar> m_chatBar;
    Aya::signals::connection m_vvConn;
    Aya::signals::connection m_coreGuiChangedSignal;
    RobloxDiagnosticsView* m_pDiagViewWidget;
    WorkspaceAnnouncementTooltip* m_AnnouncementWidget;

    RobloxMainWindow* m_pMainWindow;

    boost::scoped_ptr<RecentlySavedFilesHandler> m_pRSFHandler;

    QString m_fileName;
    QString m_displayName;
    QString m_initializationScript;
    QString m_windowTitlePrefix;

    bool m_bIsLocalDocument;

    QString m_AutoSaveName;
    bool m_IsAutoSaveLoad;
    bool m_RequiresSave;
    bool m_TestModeEnabled;
    bool m_ReopenLastSavedPlace;
    QRecursiveMutex m_IOMutex;

    static int sIDEDocCount;

    QByteArray m_initialDataModelHash;
    QByteArray m_autoSaveDataModelHash;

    tPlayModifiedScriptList m_playModifiedScriptList;
    tScriptMap m_scriptMap;

    tDebuggersMap m_EditDebuggersMap;
};

class RecentlySavedFilesHandler
{
public:
    RecentlySavedFilesHandler();
    ~RecentlySavedFilesHandler();

    void addFile(const QString& fileName);
    void removeFile(const QString& fileName);

private:
    void updateFileList();
    void saveFileList();
    void readFileList();

    QStringList m_RecentlySavedFiles;
};