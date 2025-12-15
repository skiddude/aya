#pragma once

// 3rd Party Headers
#include "boost/shared_ptr.hpp"

// Roblox Headers
#include "signal.hpp"

#include "Script/DebuggerManager.hpp"

// Qt Headers
#include <QObject>
#include <QTreeWidget>

// Roblox Studio Headers
#include "RobloxScriptDoc.hpp"

namespace Aya
{
class Script;
class DataModel;
class Heartbeat;
class RunTransition;
namespace Scripting
{
class ScriptDebugger;
}
} // namespace Aya

class QTextBlock;
class QDockWidget;
class ScriptTextEditor;
class DebuggerToolTipWidget;
class DebuggerClient;
class IRobloxDoc;
class QDebuggerClient;

class DebuggerClientManager : public QObject
{
    Q_OBJECT
public:
    enum eStepMode
    {
        STEP_INTO,
        STEP_OUT,
        STEP_OVER
    };

    static DebuggerClientManager& Instance();

    void setDataModel(shared_ptr<Aya::DataModel> spDataModel);
    boost::shared_ptr<Aya::DataModel> getDataModel();

    bool actionState(const QString& actionID, bool& enableState, bool& checkedState);
    bool handleAction(const QString& actionID, bool checkedState);

    void addDebuggerClient(DebuggerClient* pDebuggerClient);
    void removeDebuggerClient(DebuggerClient* pDebuggerClient);
    void setActiveDebuggerClient(DebuggerClient* pDebuggerClient);

    void syncBreakpointState(shared_ptr<Aya::Scripting::DebuggerBreakpoint> spBreakpoint);

    Aya::Reflection::Variant getWatchValue(boost::shared_ptr<Aya::Scripting::DebuggerWatch> spWatch);
    void addWatch(const QString& watchKey);

    DebuggerClient* getOrCreateDebuggerClient(shared_ptr<Aya::Instance> spScript);
    DebuggerClient* getDebuggerClient(shared_ptr<Aya::Instance> spScript);
    const std::vector<DebuggerClient*>& getDebuggerClients();

    DebuggerClient* getCurrentDebuggerClient()
    {
        return m_pCurrentDebuggerClient;
    }

    void breakpointEncounter(DebuggerClient* pDebuggerClient);
    void onRunTransition(Aya::RunTransition evt);

    // signals emitted
    Aya::signal<void(int, Aya::Scripting::ScriptDebugger::Stack)> clientActivated;
    Aya::signal<void()> clientDeactivated;

    Aya::signal<void()> clearAll;

    Aya::signal<void()> debuggingStarted;
    Aya::signal<void()> debuggingStopped;

    Aya::signal<void()> executionDataCleared;
    Aya::signal<void(int, DebuggerClient*)> breakpointEncountered;

    Aya::signal<void(shared_ptr<Aya::Scripting::DebuggerBreakpoint>)> breakpointAdded;
    Aya::signal<void(shared_ptr<Aya::Scripting::DebuggerBreakpoint>)> breakpointRemoved;

    Aya::signal<void(shared_ptr<Aya::Scripting::DebuggerWatch>)> watchAdded;
    Aya::signal<void(shared_ptr<Aya::Scripting::DebuggerWatch>)> watchRemoved;

    Aya::signal<void(DebuggerClient*)> debuggersListUpdated;

private Q_SLOTS:
    void onStepAction();
    void updateScriptDocument();
    void updateDebuggersList();

private:
    DebuggerClientManager();

    void displayBreakOnErrorDialog();
    void updateDocumentData(bool openDoc = true, bool updateMarkers = true);

    void onDebuggerAdded(boost::shared_ptr<Aya::Instance> spInstance);
    void onDebuggerRemoved(boost::shared_ptr<Aya::Instance> spInstance);

    void startDebugging(Aya::RunTransition evt);
    void pauseDebugging(Aya::RunTransition evt);
    void stopDebugging(Aya::RunTransition evt);

    void resumeAllClients(bool forceAll = false);
    void pauseAllClients(bool forceAll = false);
    void resetAllClients();

    Q_INVOKABLE void updateDebugActions(bool add);

    std::vector<DebuggerClient*> debuggerClients;
    boost::shared_ptr<Aya::DataModel> m_spDataModel;

    Aya::signals::connection m_cRunTransitionConnection;
    Aya::signals::connection m_cDebuggerAddedConnection;
    Aya::signals::connection m_cDebuggerRemovedConnection;

    DebuggerClient* m_pCurrentDebuggerClient;
    DebuggerClient* m_pActiveDebuggerClient;

    QAction* m_pSeparatorAction;

    QMutex m_debuggerClientMutex;

    bool m_bIgnorePauseExecution;
    bool m_bDebuggerListUpdateRequested;
};

class DebuggerClient
{
public:
    struct BreakpointDetail
    {
        BreakpointDetail(int iLine, bool iIsEnabled)
            : line(iLine)
            , isEnabled(iIsEnabled)
        {
        }
        int line;
        bool isEnabled;
        // std::string condition;
    };

    typedef std::vector<BreakpointDetail> BreakpointDetails;

    DebuggerClient(boost::shared_ptr<Aya::Instance> script);
    ~DebuggerClient();

    void activate();
    void deActivate();

    void setDocument(IRobloxDoc* pDocument);
    boost::shared_ptr<Aya::Instance> getScript();
    boost::shared_ptr<Aya::Instance> getScript(int frameNo);

    bool updateDocument(bool openDoc = true, bool updateMarkers = true);
    void setExternalDebuggerClient(DebuggerClient* pExtDebuggerClient);

    void highlightLine(int textEditLine);
    int getCurrentLine();

    void setCurrentLine(int textEditLine);
    void setMarker(int textEditLine, const QString& marker, bool setBlockCurrent);

    void setCurrentFrame(int frame);
    int getCurrentFrame()
    {
        return m_CurrentFrame;
    }

    Aya::Scripting::ScriptDebugger::Stack getCurrentCallStack()
    {
        return m_CallStackAtPausedLine;
    }

    bool getValue(const QString& key, Aya::Reflection::Variant& value, int frame = -1);

    Aya::Reflection::Variant getWatchValue(boost::shared_ptr<Aya::Scripting::DebuggerWatch> spWatch);
    void addWatch(const QString& watchKey);

    QString getSourceCodeAtLine(int breakpointLine);
    void syncBreakpointState(int breakpointLine);

    void syncScriptWithTextEditor(const BreakpointDetails& textEditBreakpoints);
    void syncTextEdtiorWithScript();

    void toggleBreakpoint(int breakpointLine);
    void toggleBreakpointState(int breakpointLine);

    void pauseExecution();
    void resumeExecution();
    void resetExecution();

    bool isDebugging();
    bool isPaused();

    bool hasError();
    std::string getErrorMessage()
    {
        return m_ErrorMessage;
    }

    const Aya::Scripting::ScriptDebugger::PausedThreads& getPausedThreads();
    int getCurrentThread();
    void setCurrentThread(int threadID);

    bool isHighlightingRequired();
    bool isTopFrameCurrent();

    bool actionState(const QString& actionID, bool& enableState, bool& checkedState);
    bool handleAction(const QString& actionID, bool checkedState);

    // signals emitted
    Aya::signal<void()> debuggerResuming;

private:
    bool isValid();

    void onDebuggerResume();
    void onEncounteredBreakpoint(int line);
    void onBreakpointAdded(shared_ptr<Aya::Instance> spInstance);
    void onBreakpointRemoved(shared_ptr<Aya::Instance> spInstance);
    void onWatchAdded(shared_ptr<Aya::Instance> spInstance);
    void onWatchRemoved(shared_ptr<Aya::Instance> spInstance);
    void onScriptErrorDetected(int errorLine, std::string errorMessage, Aya::Scripting::ScriptDebugger::Stack stack);

    void onScriptStopped();

    bool isKeyDefined(boost::shared_ptr<const Aya::Reflection::ValueMap> variables, const QString& wordUnderCursor, Aya::Reflection::Variant& value);

    boost::shared_ptr<Aya::Instance> m_spScript;
    boost::shared_ptr<Aya::DataModel> m_spDataModel;
    boost::shared_ptr<Aya::Scripting::ScriptDebugger> m_spDebugger;

    std::list<Aya::signals::connection> m_cConnections;

    DebuggerClient* m_pExtDebuggerClient;
    QDebuggerClient* m_pQDebuggerClient;

    QStringList m_ScriptLines;

    Aya::Scripting::ScriptDebugger::Stack m_CallStackAtPausedLine;
    int m_CurrentFrame;

    std::string m_ErrorMessage;

    int m_currentThreadID;

    bool m_bIgnoreBreakpointAddRemove;
    bool m_bIsActive;
};

class QDebuggerClient : public QObject
{
    Q_OBJECT
public:
    QDebuggerClient(DebuggerClient* pDebuggerClient, ScriptTextEditor* pTextEdit);
    ~QDebuggerClient();

    void activate();
    void deActivate();

    QString getSourceCodeAtLine(int textEditLine);
    void updateBreakpointState(int textEditLine, RBXTextUserData::eBreakpointState state = RBXTextUserData::NO_BREAKPOINT);

    void highlightLine(int textEditLine);
    void clearLineHighlight();
    void setCurrentLine(int textEditLine);
    void setMarker(int textEditLine, const QString& marker, bool setBlockCurrent);

    void setExternalDebuggerClient(DebuggerClient* pExtDebuggerClient)
    {
        m_pExtDebuggerClient = pExtDebuggerClient;
    }

    void updateEditor();

private Q_SLOTS:
    void onInsertBreakpoint();
    void onDeleteBreakpoint();
    void onToggleBreakpoint(int textEditLine);
    void onToggleBreakpointState();
    void onAddWatch();

    // slot methods to ensure public functions are called from main thread
    void highlightLine_MT(int textEditLine);
    void clearLineHighlight_MT();
    void setCurrentLine_MT(int textEditLine);
    void setMarker_MT(int textEditLine, QString marker, bool setBlockCurrent);

    bool onShowToolTip(const QPoint& pos, const QString& key);
    void onContentsChanged();

    void onUpdateContextualMenu(QMenu* pMenu, QPoint pos);

private:
    void updateTextUserData(int textEditLineNumber, RBXTextUserData::eBreakpointState state);
    void modifyContextualMenu(QMenu* pContextualMenu);
    void cleanupContextMenu();

    void setCurrentBlock(const QTextBlock& blockToSet);
    void highlightBlock(const QTextBlock& blockToHighlight);

    DebuggerClient::BreakpointDetails getBreakpointDetails();
    void removeBreakpointState();

    bool eventFilter(QObject* obj, QEvent* evt);

    DebuggerClient* m_pDebuggerClient;
    DebuggerClient* m_pExtDebuggerClient;

    QAction* m_pBreakpointMenuAction;
    QAction* m_pSeparatorAction1;
    QAction* m_pSeparatorAction2;

    ScriptTextEditor* m_pTextEdit;
    DebuggerToolTipWidget* m_pToolTipWidget;

    QList<QTextEdit::ExtraSelection> m_extraSelections;

    int m_lastBlockCount;
    bool m_isModifiedByKeyBoard;
    bool m_ignoreAutoRepeatEvent;
};
