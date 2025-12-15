

#pragma once

// Qt Headers
#include <QProcess>
#include <QMutex>

// Roblox Headers
#include "Tree/Verb.hpp"

#include "DataModel/Commands.hpp"

// Studio Headers
#include "RobloxIDEDoc.hpp"
#include "RobloxDocManager.hpp"
#include "ManageEmulationDeviceDialog.hpp"
#include "ScriptPickerDialog.hpp"

namespace Aya
{
class Instance;
class DataModel;
class VideoControl;
class ViewBase;
class ChangeHistoryService;
class LuaSourceContainer;
} // namespace Aya

class RobloxMainWindow;
class WebDialog;
class IRobloxDoc;
class InsertAdvancedObjectDialog;


class GroupSelectionVerb : public Aya::EditSelectionVerb
{
private:
    typedef Aya::EditSelectionVerb Super;

public:
    GroupSelectionVerb(Aya::DataModel* dataModel);
    virtual bool isEnabled() const;
    virtual void doIt(Aya::IDataState* dataState);
};

class UngroupSelectionVerb : public Aya::EditSelectionVerb
{
private:
    typedef Aya::EditSelectionVerb Super;

public:
    UngroupSelectionVerb(Aya::DataModel* dataModel);
    virtual bool isEnabled() const;
    virtual void doIt(Aya::IDataState* dataState);
};

//////////////////////////////////////////////////////////////////////////
// CSG Operations

class UnionSelectionVerb : public Aya::EditSelectionVerb
{
private:
    typedef Aya::EditSelectionVerb Super;

public:
    UnionSelectionVerb(Aya::DataModel* dataModel);

    void performUnion(Aya::IDataState* dataState);

    virtual bool isEnabled() const;
    virtual void doIt(Aya::IDataState* dataState);
};

class NegateSelectionVerb : public Aya::EditSelectionVerb
{
private:
    typedef Aya::EditSelectionVerb Super;

public:
    NegateSelectionVerb(Aya::DataModel* dataModel);

    virtual bool isEnabled() const;
    virtual void doIt(Aya::IDataState* dataState);
};

class SeparateSelectionVerb : public Aya::EditSelectionVerb
{
private:
    typedef Aya::EditSelectionVerb Super;

public:
    SeparateSelectionVerb(Aya::DataModel* dataModel);

    virtual bool isEnabled() const;
    virtual void doIt(Aya::IDataState* dataState);
    void performSeparate(Aya::IDataState* dataState);
};

class CutVerb : public Aya::DeleteSelectionVerb
{
public:
    CutVerb(Aya::DataModel* dataModel);
    virtual void doIt(Aya::IDataState* dataState);
};

class CopyVerb : public Aya::EditSelectionVerb
{
public:
    CopyVerb(Aya::DataModel* dataModel);
    virtual void doIt(Aya::IDataState* dataState);
};

class PasteVerb
    : public QObject
    , public Aya::Verb
{
    Q_OBJECT
public:
    PasteVerb(Aya::DataModel* dataModel, bool pasteInto);
    virtual bool isEnabled() const;
    virtual void doIt(Aya::IDataState* dataState);

private Q_SLOTS:
    void onClipboardModified();

private:
    void createInstancesFromClipboard(shared_ptr<Aya::Instances> items);
    void insertInstancesIntoParent(shared_ptr<Aya::Instances> items);
    void createInstancesFromClipboardDep(Aya::Instances& itemsToPaste);
    void insertInstancesIntoParentDep(Aya::Instances& itemsToPaste);

    bool isPasteInfoAvailable() const;

    Aya::DataModel* m_pDataModel;
    const bool m_bPasteInto;
    bool m_bIsPasteInfoAvailable;
};

class DuplicateSelectionVerb
    : public QObject
    , public Aya::EditSelectionVerb
{
    Q_OBJECT
public:
    DuplicateSelectionVerb(Aya::DataModel* dataModel);
    virtual void doIt(Aya::IDataState* dataState);
};

class UndoVerb : public Aya::Verb
{
public:
    UndoVerb(Aya::VerbContainer* pVerbContainer);
    void doIt(Aya::IDataState* dataState);

    virtual bool isChecked() const
    {
        return false;
    }

    virtual bool isEnabled() const;
    virtual std::string getText() const;

private:
    Aya::DataModel* m_pDataModel;
    boost::shared_ptr<Aya::ChangeHistoryService> m_pChangeHistory;
};

class RedoVerb : public Aya::Verb
{
public:
    RedoVerb(Aya::VerbContainer* pVerbContainer);
    void doIt(Aya::IDataState* dataState);

    virtual bool isChecked() const
    {
        return false;
    }

    virtual bool isEnabled() const;
    virtual std::string getText() const;

private:
    Aya::DataModel* m_pDataModel;
    boost::shared_ptr<Aya::ChangeHistoryService> m_pChangeHistory;
};

class InsertModelVerb : public Aya::Verb
{
public:
    InsertModelVerb(Aya::VerbContainer* pVerbContainer, bool insertInto);
    void doIt(Aya::IDataState* dataState);

    virtual bool isChecked() const
    {
        return false;
    }

    virtual bool isEnabled() const
    {
        return true;
    };

private:
    void insertModel();

private:
    Aya::DataModel* m_pDataModel;
    const bool m_bInsertInto;
};

class SelectionSaveToFileVerb : public Aya::Verb
{
public:
    SelectionSaveToFileVerb(Aya::VerbContainer* pVerbContainer);

    virtual void doIt(Aya::IDataState* dataState);
    virtual bool isEnabled() const;

private:
    void saveToFile();
    std::auto_ptr<XmlElement> writeSelection();

    Aya::DataModel* m_pDataModel;
};

class PublishToRobloxAsVerb : public Aya::Verb
{
public:
    PublishToRobloxAsVerb(Aya::VerbContainer* pVerbContainer, RobloxMainWindow* mainWnd);
    virtual ~PublishToRobloxAsVerb();

    void doIt(Aya::IDataState* dataState);

    virtual bool isChecked() const
    {
        return false;
    }

    virtual bool isEnabled() const;
    void initDialog();

    QDialog* getPublishDialog();

private:
    Aya::DataModel* m_pDataModel;
    RobloxMainWindow* m_pMainWindow;
    WebDialog* m_dlg;
};

class PublishToRobloxVerb : public Aya::Verb
{
public:
    PublishToRobloxVerb(Aya::VerbContainer* pVerbContainer, RobloxMainWindow* mainWnd);

    virtual void doIt(Aya::IDataState* dataState);
    virtual bool isChecked() const
    {
        return false;
    }
    virtual bool isEnabled() const;
    void onEventPublishingFinished();

private:
    void save(Aya::ContentId contentID, bool* outError, QString* outErrorTitle, QString* outErrorText);

    bool m_bIsPublishInProcess;
    Aya::DataModel* m_pDataModel;
    RobloxMainWindow* m_pMainWindow;
    QMutex publishingMutex;
};

class PublishSelectionToRobloxVerb : public Aya::Verb
{
public:
    PublishSelectionToRobloxVerb(Aya::VerbContainer* pVerbContainer, RobloxMainWindow* mainWnd);
    virtual ~PublishSelectionToRobloxVerb();

    void doIt(Aya::IDataState* dataState);

    virtual bool isChecked() const
    {
        return false;
    }

    virtual bool isEnabled() const;

private:
    Aya::DataModel* m_pDataModel;
    RobloxMainWindow* m_pMainWindow;
    WebDialog* m_dlg;
};

class CreateNewLinkedSourceVerb
    : public QObject
    , public Aya::Verb
{
    Q_OBJECT
public:
    CreateNewLinkedSourceVerb(Aya::DataModel* pVerbContainer);

    virtual bool isEnabled() const;

    virtual void doIt(Aya::IDataState* dataState);

private:
    Aya::DataModel* m_pDataModel;

    shared_ptr<Aya::LuaSourceContainer> getLuaSourceContainer() const;
    void doItThread(std::string source, int currentGameId, boost::optional<int> groupId, QString name, bool* success);
};

class PublishAsPluginVerb : public Aya::Verb
{
public:
    PublishAsPluginVerb(Aya::VerbContainer* pVerbContainer, RobloxMainWindow* mainWnd);

    virtual void doIt(Aya::IDataState* dataState);

    virtual bool isEnabled() const;

private:
    Aya::DataModel* m_pDataModel;
    RobloxMainWindow* m_pMainWindow;
    boost::scoped_ptr<WebDialog> m_dlg;
};

class LaunchInstancesVerb : public Aya::Verb
{
public:
    enum SimulationType
    {
        PLAYSOLO = 1,
        SERVERONEPLAYER = 2,
        SERVERFOURPLAYERS = 5
    };

    LaunchInstancesVerb(Aya::VerbContainer* pVerbContainer);
    void doIt(Aya::IDataState* dataState);

    virtual bool isEnabled() const
    {
        return true;
    }


private:
    Aya::VerbContainer* m_pVerbContainer;
};

class StartServerAndPlayerVerb : public Aya::Verb
{
public:
    StartServerAndPlayerVerb(Aya::VerbContainer* pVerbContainer);
    void doIt(Aya::IDataState* dataState);

    void launchPlayers(int numPlayers);

private:
    static void launchStudioInstances(Aya::VerbContainer* pVerbContainer, int numPlayers);
    Aya::VerbContainer* m_pVerbContainer;
};

class ServerPlayersStateInitVerb : public Aya::Verb
{
public:
    ServerPlayersStateInitVerb(Aya::VerbContainer* pVerbContainer);
    void doIt(Aya::IDataState* dataState);

    virtual bool isChecked() const;
};

class CreatePluginVerb : public Aya::Verb
{
public:
    CreatePluginVerb(Aya::VerbContainer* pVerbContainer);
    void doIt(Aya::IDataState* dataState);

    virtual bool isEnabled() const
    {
        return true;
    }

private:
    Aya::VerbContainer* m_pVerbContainer;
};

class ManageEmulationDevVerb : public Aya::Verb
{
public:
    ManageEmulationDevVerb(Aya::VerbContainer* pVerbContainer, QWidget* newParent);
    void doIt(Aya::IDataState* dataState);

    virtual bool isEnabled() const
    {
        return true;
    }

private:
    shared_ptr<ManageEmulationDeviceDialog> dialog;
    Aya::DataModel* m_pDataModel;
};

class AudioToggleVerb : public Aya::Verb
{
public:
    AudioToggleVerb(Aya::VerbContainer* pVerbContainer);
    void doIt(Aya::IDataState* dataState);

    virtual bool isEnabled() const
    {
        return true;
    }
    virtual bool isChecked() const;

private:
    Aya::DataModel* m_pDataModel;
};

class AnalyzePhysicsToggleVerb : public Aya::Verb
{
public:
    AnalyzePhysicsToggleVerb(Aya::DataModel*);
    virtual void doIt(Aya::IDataState* dataState);

    void startAnalyze();
    void stopAnalyze();

    virtual bool isEnabled() const;
    virtual bool isChecked() const;

private:
    Aya::DataModel* m_pDataModel;
};

class PlaySoloVerb : public Aya::Verb
{

public:
    PlaySoloVerb(Aya::VerbContainer* pVerbContainer);
    void doIt(Aya::IDataState* dataState);

    virtual bool isEnabled() const
    {
        return true;
    }


private:
    Aya::DataModel* m_pDataModel;
};

class StartServerVerb : public Aya::Verb
{

public:
    StartServerVerb(Aya::VerbContainer* pVerbContainer);
    void doIt(Aya::IDataState* dataState);

    virtual bool isEnabled() const
    {
        return true;
    }


private:
    Aya::DataModel* m_pDataModel;
};

class StartPlayerVerb : public Aya::Verb
{

public:
    StartPlayerVerb(Aya::VerbContainer* pVerbContainer);
    void doIt(Aya::IDataState* dataState);

    virtual bool isEnabled() const
    {
        return true;
    }


private:
    Aya::DataModel* m_pDataModel;
};

class ToggleFullscreenVerb : public Aya::Verb
{
public:
    ToggleFullscreenVerb(Aya::VerbContainer* container);
    virtual void doIt(Aya::IDataState* dataState);
    virtual bool isEnabled() const;
};

class ShutdownClientVerb : public Aya::Verb
{
public:
    ShutdownClientVerb(Aya::VerbContainer* container, IRobloxDoc* pDoc);
    virtual void doIt(Aya::IDataState* dataState);

private:
    IRobloxDoc* m_pIDEDoc;
};

class ShutdownClientAndSaveVerb : public Aya::Verb
{
public:
    ShutdownClientAndSaveVerb(Aya::VerbContainer* container, IRobloxDoc* pDoc);
    virtual void doIt(Aya::IDataState* dataState);

private:
    IRobloxDoc* m_pIDEDoc;
};

class LeaveGameVerb : public Aya::Verb
{
public:
    LeaveGameVerb(Aya::VerbContainer* container, IRobloxDoc* pDoc);
    virtual void doIt(Aya::IDataState* dataState);

private:
    IRobloxDoc* m_pIDEDoc;
};



class ToggleAxisWidgetVerb : public Aya::Verb
{
public:
    ToggleAxisWidgetVerb(Aya::DataModel*);
    virtual void doIt(Aya::IDataState* dataState);
    virtual bool isEnabled() const
    {
        return true;
    }
    virtual bool isChecked() const;

private:
    Aya::DataModel* m_pDataModel;
};

class Toggle3DGridVerb : public Aya::Verb
{
public:
    Toggle3DGridVerb(Aya::DataModel*);
    virtual void doIt(Aya::IDataState* dataState);
    virtual bool isEnabled() const;
    virtual bool isChecked() const;

private:
    Aya::DataModel* m_pDataModel;
};

class ToggleCollisionCheckVerb : public Aya::Verb
{
public:
    ToggleCollisionCheckVerb(Aya::DataModel*);
    virtual void doIt(Aya::IDataState* dataState);
    virtual bool isEnabled() const
    {
        return true;
    }
    virtual bool isChecked() const;
};

class ToggleLocalSpaceVerb : public Aya::Verb
{
public:
    ToggleLocalSpaceVerb(Aya::DataModel*);
    virtual void doIt(Aya::IDataState* dataState);
    virtual bool isEnabled() const
    {
        return true;
    }
    virtual bool isChecked() const;

private:
    Aya::DataModel* m_pDataModel;
};

class InsertAdvancedObjectViewVerb : public Aya::Verb
{
public:
    InsertAdvancedObjectViewVerb(Aya::VerbContainer* pVerbContainer);
    virtual ~InsertAdvancedObjectViewVerb();

    void doIt(Aya::IDataState* dataState);
    virtual bool isEnabled() const;
    virtual bool isChecked() const;
};

class JointToolHelpDialogVerb : public Aya::Verb
{

public:
    JointToolHelpDialogVerb(Aya::VerbContainer* pVerbContainer);
    void doIt(Aya::IDataState* dataState);

    virtual bool isEnabled() const
    {
        return true;
    }
    virtual bool isChecked() const
    {
        return false;
    }
};

class StudioMaterialVerb : public Aya::MaterialVerb
{
public:
    StudioMaterialVerb(Aya::DataModel* dataModel);
    void doIt(Aya::IDataState* dataState);

    virtual bool isEnabled() const
    {
        return true;
    }
    virtual bool isChecked() const;

    static bool sMaterialActionActAsTool;
};

class StudioColorVerb : public Aya::ColorVerb
{
public:
    StudioColorVerb(Aya::DataModel* dataModel);
    void doIt(Aya::IDataState* dataState);

    virtual bool isEnabled() const
    {
        return true;
    }
    virtual bool isChecked() const;

    static bool sColorActionActAsTool;

private:
    void addColorToIcon();
};

class OpenToolBoxWithOptionsVerb
    : public QObject
    , public Aya::Verb
{
    Q_OBJECT
public:
    OpenToolBoxWithOptionsVerb(Aya::VerbContainer* pVerbContainer);
    ~OpenToolBoxWithOptionsVerb();

    void doIt(Aya::IDataState* dataState);
    virtual bool isEnabled();

private Q_SLOTS:
    void handleDockVisibilityChanged(bool isVisible);
};

class InsertBasicObjectVerb : public Aya::Verb
{
public:
    InsertBasicObjectVerb(Aya::DataModel* dataModel);
    void doIt(Aya::IDataState* dataState);

    virtual bool isEnabled();

private:
    Aya::DataModel* m_pDataModel;
};

class JointCreationModeVerb : public Aya::Verb
{
public:
    JointCreationModeVerb(Aya::DataModel* dataModel);
    void doIt(Aya::IDataState* dataState);

private:
    void updateMenuIcon();
    void updateMenuActions();
};

class ExportSelectionVerb
    : public QObject
    , public Aya::Verb
{
    Q_OBJECT
public:
    ExportSelectionVerb(Aya::DataModel*);

    virtual bool isEnabled() const
    {
        return (RobloxDocManager::Instance().getPlayDoc() != NULL);
    }
    virtual bool isChecked() const
    {
        return false;
    }
    virtual bool isSelected() const
    {
        return false;
    }
    virtual void doIt(Aya::IDataState* dataState);

private:
    Aya::DataModel* m_pDataModel;
};

class ExportPlaceVerb
    : public QObject
    , public Aya::Verb
{
    Q_OBJECT
public:
    ExportPlaceVerb(Aya::DataModel*);

    virtual bool isEnabled() const
    {
        return (RobloxDocManager::Instance().getPlayDoc() != NULL);
    }
    virtual bool isChecked() const
    {
        return false;
    }
    virtual bool isSelected() const
    {
        return false;
    }
    virtual void doIt(Aya::IDataState* dataState);

private:
    Aya::DataModel* m_pDataModel;
};

class DummyVerb : public Aya::Verb
{
public:
    DummyVerb(Aya::VerbContainer* container, const char* name)
        : Verb(container, name)
    {
    }
    virtual bool isEnabled() const
    {
        return false;
    }

    virtual void doIt(Aya::IDataState* dataState) {}
};
