/**
 *  RobloxStudioVerbs.cpp
 */


#include "RobloxStudioVerbs.hpp"

// Qt Headers
#include <QByteArray>
#include <QDataStream>
#include <QClipboard>
#include <QApplication>
#include <QMimeData>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QProcess>
#include <QTextStream>
#include <QImage>
#include <QFile>
#include <QUrl>
#include <QDialogButtonBox>
#include <QTime>
#include <QDockWidget>
#include <QDialog>
#include <QPainter>
#include <QActionGroup>

// 3rd Party Headers
#include "System.hpp"
#include <boost/unordered_map.hpp>

// Roblox Headers
#include "DataModel/Animation.hpp"

#include "DataModel/BasicPartInstance.hpp"

#include "DataModel/ChangeHistory.hpp"

#include "DataModel/ContentProvider.hpp"

#include "DataModel/CSGDictionaryService.hpp"

#include "DataModel/DataModel.hpp"

#include "DataModel/Decal.hpp"

#include "DataModel/GameBasicSettings.hpp"

#include "DataModel/KeyframeSequence.hpp"

#include "DataModel/KeyframeSequenceProvider.hpp"

#include "DataModel/NonReplicatedCSGDictionaryService.hpp"

#include "DataModel/PartOperation.hpp"

#include "DataModel/PartOperationAsset.hpp"

#include "DataModel/PhysicsSettings.hpp"

#include "DataModel/PlayerGui.hpp"

#include "DataModel/SpecialMesh.hpp"

#include "DataModel/Tool.hpp"

#include "DataModel/Visit.hpp"

#include "DataModel/Workspace.hpp"

#include "DataModel/ToolsPart.hpp"

#include "Tree/Instance.hpp"

#include "Xml/SerializerBinary.hpp"
#include "Xml/WebParser.hpp"
#include "Xml/XmlSerializer.hpp"
#include "API.hpp"

#include "Players.hpp"

#include "Tool/AdvMoveTool.hpp"

#include "Tool/AdvRotateTool.hpp"

#include "Utility/Statistics.hpp"

#include "Script/LuaSourceContainer.hpp"
#include "Script/ScriptContext.hpp"
#include "Script/script.hpp"
#include "Log.hpp"

#include "Tool/ToolsArrow.hpp"

#include "Render/GeometryGenerator.hpp"
#include "Utility/CSGKernel.hpp"

#include "Utility/RobloxServicesTools.hpp"
#include "Tool/MoveResizeJoinTool.hpp"


// Roblox Studio Headers
#include "StudioUtilities.hpp"
#include "WebDialog.hpp"
#include "RbxWorkspace.hpp"
#include "RobloxSettings.hpp"
#include "RobloxDocManager.hpp"
#include "CommonInsertWidget.hpp"
#include "DataModel/RenderSettingsItem.hpp"
#include "QtUtilities.hpp"
#include "RobloxMainWindow.hpp"
#include "UpdateUIManager.hpp"
#include "RobloxApplicationManager.hpp"
#include "RobloxIDEDoc.hpp"
#include "StudioSerializerHelper.hpp"
#include "AuthenticationHelper.hpp"
#include "Roblox.hpp"
#include "NameValueStoreManager.hpp"
#include "RobloxUser.hpp"
#include "RobloxToolBox.hpp"
#include "Base/ViewBase.hpp"
#include "CSGOperations.hpp"
#include "RobloxGameExplorer.hpp"
#include "ScriptPickerDialog.hpp"

#include "Xml/SerializerBinary.hpp"

// Video record related includes
#ifdef _WIN32
// #include "VideoControl.hpp"
//  #include "DSVideoCaptureEngine.hpp"
#endif
#include "ManageEmulationDeviceDialog.hpp"

FASTFLAG(PrefetchResourcesEnabled)
FASTFLAG(LuaDebugger)

FASTFLAG(StudioCSGAssets)
FASTFLAG(GameExplorerUseV2AliasEndpoint)
FASTFLAG(StudioEnableGameAnimationsTab)
FASTFLAG(PhysicsAnalyzerEnabled)

DYNAMIC_FASTFLAG(DraggerUsesNewPartOnDuplicate)
DYNAMIC_FASTFLAG(RestoreTransparencyOnToolChange)

FASTFLAGVARIABLE(StudioMimeDataContainsInstancePath, false)
FASTFLAGVARIABLE(StudioOperationFailureHangFix, true)

FASTINTVARIABLE(StudioWebDialogMinimumWidth, 970)
FASTINTVARIABLE(StudioWebDialogMinimumHeight, 480)

DYNAMIC_FASTFLAG(UseRemoveTypeIDTricks)

static const char* sCollisionToggleModeSetting = "CollisionToggleMode";
static const char* sLocalTranslationModeSetting = "LocalTranslationMode";
static const char* sLocalRotationModeSetting = "LocalRotationMode";
static const char* sRibbonJointCreationMode = "rbxRibbonJointCreationMode";
static const char* sRibbonStartServerSetting = "rbxRibbonStartServer";
static const char* sRibbonNumPlayerSetting = "rbxRibbonNumPlayer";

static const char* sRobloxMimeType = "application/x-roblox-studio";

bool StudioMaterialVerb::sMaterialActionActAsTool = false;
bool StudioColorVerb::sColorActionActAsTool = false;

using namespace Aya;

GroupSelectionVerb::GroupSelectionVerb(DataModel* dataModel)
    : EditSelectionVerb("Group", dataModel)
{
    ;
}

bool GroupSelectionVerb::isEnabled() const
{
    if (!Super::isEnabled())
        return false;
    else
    {
        Selection* sel = ServiceProvider::find<Selection>(dataModel);
        return sel && sel->size() > 0;
    }
}

void GroupSelectionVerb::doIt(IDataState* dataState)
{
    FASTLOG(FLog::Verbs, "Gui:GroupSelection");

    Selection* sel = ServiceProvider::create<Selection>(dataModel);

    ModelInstance* groupInstance = workspace->group(sel->begin(), sel->end()).get();

    sel->setSelection(groupInstance);

    // TODO: Undo/Redo...
    {
        DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
        ChangeHistoryService::requestWaypoint(getName().c_str(), workspace.get());
    }

    dataState->setDirty(true);
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

namespace
{
static void operationFailed(const char* title, const char* msg)
{
    QMetaObject::invokeMethod(&UpdateUIManager::Instance(), "showErrorMessage",
        FFlag::StudioOperationFailureHangFix ? Qt::QueuedConnection : Qt::BlockingQueuedConnection, Q_ARG(QString, title), Q_ARG(QString, msg));
}
} // anonymous namespace
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

UngroupSelectionVerb::UngroupSelectionVerb(DataModel* dataModel)
    : EditSelectionVerb("UnGroup", dataModel)
{
    // Force creation here, rather than in isEnabled
    ServiceProvider::create<FilteredSelection<ModelInstance>>(dataModel);
}

bool canUngroup(ModelInstance* wInstance)
{
    return (wInstance->numChildren() > 0);
}

bool UngroupSelectionVerb::isEnabled() const
{
    if (!Super::isEnabled())
        return false;

    // enabled if one ore more selected items has children and can be ungrouped
    Selection* selection = ServiceProvider::create<Selection>(dataModel);

    for (std::vector<boost::shared_ptr<Aya::Instance>>::const_iterator iter = selection->begin(); iter != selection->end(); ++iter)
    {
        if (shared_ptr<ModelInstance> m = Aya::Instance::fastSharedDynamicCast<Aya::ModelInstance>(*iter))
            return true;
    }
    return false;
}

class Ungroup
{
public:
    std::vector<Instance*>& ungroupedItems;
    bool& didSomething;
    Ungroup(std::vector<Instance*>& ungroupedItems, bool& didSomething)
        : ungroupedItems(ungroupedItems)
        , didSomething(didSomething)
    {
    }

    void operator()(const shared_ptr<Instance>& wInstance)
    {
        if (wInstance->numChildren() > 0)
        {
            for (size_t j = 0; j < wInstance->numChildren(); ++j)
            {
                ungroupedItems.push_back(wInstance->getChild(j));
            }
            wInstance->promoteChildren();
            wInstance->setParent(NULL);
            didSomething = true;
        }
    }
};

void UngroupSelectionVerb::doIt(IDataState* dataState)
{
    FASTLOG(FLog::Verbs, "Gui:UngroupSelection");

    Selection* selection = ServiceProvider::create<Selection>(dataModel);

    // First make a copy of the selection list
    std::vector<shared_ptr<Instance>> itemsToUngroup;
    for (std::vector<boost::shared_ptr<Aya::Instance>>::const_iterator iter = selection->begin(); iter != selection->end(); ++iter)
    {
        if (shared_ptr<ModelInstance> m = Aya::Instance::fastSharedDynamicCast<Aya::ModelInstance>(*iter))
            itemsToUngroup.push_back(m);
    }

    // Now iterate over the objects to be ungrouped and ungroup them
    std::vector<Instance*> ungroupedItems;
    bool didSomething = false;
    std::for_each(itemsToUngroup.begin(), itemsToUngroup.end(), Ungroup(ungroupedItems, didSomething));

    if (didSomething)
    {
        selection->setSelection(ungroupedItems.begin(), ungroupedItems.end());
    }
    else
    {
        debugAssertM(0, "Calling ungroup command without checking is-enabled");
    }

    // TODO: Undo/Redo...
    {
        DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
        ChangeHistoryService::requestWaypoint(getName().c_str(), workspace.get());
    }

    dataState->setDirty(true);
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

UnionSelectionVerb::UnionSelectionVerb(DataModel* dataModel)
    : EditSelectionVerb("UnionSelection", dataModel)
{
}

bool UnionSelectionVerb::isEnabled() const
{
    if (!Super::isEnabled())
        return false;
    else
    {
        Selection* sel = ServiceProvider::find<Selection>(dataModel);
        return sel && ((sel->size() > 0 && !(sel->size() == 1 && Aya::Instance::fastSharedDynamicCast<Aya::PartOperation>(*sel->begin()))) ||
                          (sel->size() == 1 && Aya::Instance::fastSharedDynamicCast<Aya::BasicPartInstance>(*sel->begin())));
    }
}

void UnionSelectionVerb::performUnion(IDataState* dataState)
{
    DataModel::LegacyLock lock(dataModel, Aya::DataModelJob::Write);
    FASTLOG(FLog::Verbs, "Gui:UnionSelection");

    CSGOperations csgOps(dataModel, operationFailed);
    Selection* selection = ServiceProvider::create<Selection>(dataModel);
    shared_ptr<Aya::PartOperation> partOperation;
    if (csgOps.doUnion(selection->begin(), selection->end(), partOperation))
        ChangeHistoryService::requestWaypoint(getName().c_str(), workspace.get());
    if (partOperation)
        selection->setSelection(partOperation.get());
    dataState->setDirty(true);
}

void UnionSelectionVerb::doIt(IDataState* dataState)
{
    UpdateUIManager::Instance().waitForLongProcess("Union", boost::bind(&UnionSelectionVerb::performUnion, this, dataState));
}

NegateSelectionVerb::NegateSelectionVerb(DataModel* dataModel)
    : EditSelectionVerb("NegateSelection", dataModel)
{
}

bool NegateSelectionVerb::isEnabled() const
{
    if (!Super::isEnabled())
        return false;
    else
    {
        Selection* sel = ServiceProvider::find<Selection>(dataModel);
        return sel && sel->size() > 0;
    }
}

void NegateSelectionVerb::doIt(IDataState* dataState)
{
    FASTLOG(FLog::Verbs, "Gui:NegateSelection");

    CSGOperations csgOps(dataModel, operationFailed);
    Selection* selection = ServiceProvider::create<Selection>(dataModel);
    std::vector<shared_ptr<Aya::Instance>> toSelect;
    if (csgOps.doNegate(selection->begin(), selection->end(), toSelect))
    {
        DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
        ChangeHistoryService::requestWaypoint(getName().c_str(), workspace.get());
    }
    selection->setSelection(toSelect.begin(), toSelect.end());
    dataState->setDirty(true);
}

SeparateSelectionVerb::SeparateSelectionVerb(DataModel* dataModel)
    : EditSelectionVerb("SeparateSelection", dataModel)
{
}

bool SeparateSelectionVerb::isEnabled() const
{
    if (!Super::isEnabled())
        return false;

    // enabled if one ore more selected items has children and can be ungrouped
    Selection* selection = ServiceProvider::create<Selection>(dataModel);

    for (std::vector<boost::shared_ptr<Aya::Instance>>::const_iterator iter = selection->begin(); iter != selection->end(); ++iter)
        if (shared_ptr<PartOperation> o = Aya::Instance::fastSharedDynamicCast<PartOperation>(*iter))
            return true;

    return false;
}

void SeparateSelectionVerb::doIt(IDataState* dataState)
{
    UpdateUIManager::Instance().waitForLongProcess("Separate", boost::bind(&SeparateSelectionVerb::performSeparate, this, dataState));
}

void SeparateSelectionVerb::performSeparate(IDataState* dataState)
{
    DataModel::LegacyLock lock(dataModel, Aya::DataModelJob::Write);
    FASTLOG(FLog::Verbs, "Gui:SeparateSelection");

    CSGOperations csgOps(dataModel, operationFailed);
    Selection* selection = ServiceProvider::create<Selection>(dataModel);
    std::vector<shared_ptr<Instance>> ungroupedItems;

    if (csgOps.doSeparate(selection->begin(), selection->end(), ungroupedItems))
        Aya::ChangeHistoryService::requestWaypoint(getName().c_str(), workspace.get());

    selection->setSelection(ungroupedItems.begin(), ungroupedItems.end());
    dataState->setDirty(true);
}

CutVerb::CutVerb(Aya::DataModel* dataModel)
    : DeleteSelectionVerb(dataModel, dataModel, "Cut")
{
}

void CutVerb::doIt(Aya::IDataState* dataState)
{
    // First copy selection to clipboard
    Aya::Verb* pCopyVerb = getContainer()->getVerb("Copy");
    if (!pCopyVerb)
        return;

    pCopyVerb->doIt(dataState);

    // now call delete
    DeleteSelectionVerb::doIt(dataState);
}

CopyVerb::CopyVerb(Aya::DataModel* dataModel)
    : EditSelectionVerb("Copy", dataModel)
{
}

void CopyVerb::doIt(Aya::IDataState*)
{
    QClipboard* pClipboard = QApplication::clipboard();
    pClipboard->clear();

    Aya::Selection* pSelection = Aya::ServiceProvider::create<Aya::Selection>(dataModel);

    Aya::CSGDictionaryService* dictionaryService = Aya::ServiceProvider::create<Aya::CSGDictionaryService>(dataModel);
    Aya::NonReplicatedCSGDictionaryService* nrDictionaryService = Aya::ServiceProvider::create<Aya::NonReplicatedCSGDictionaryService>(dataModel);
    for (std::vector<shared_ptr<Aya::Instance>>::const_iterator iter = pSelection->begin(); iter != pSelection->end(); ++iter)
    {
        dictionaryService->retrieveAllDescendants(*iter);
        nrDictionaryService->retrieveAllDescendants(*iter);
    }

    std::ostringstream selectionStream;

    Aya::Instances instances(pSelection->begin(), pSelection->end());
    Aya::SerializerBinary::serialize(selectionStream, instances);

    std::string contents = selectionStream.str();

    QMimeData* pMimeDataForClipboard = new QMimeData;
    pMimeDataForClipboard->setData(sRobloxMimeType, QByteArray(contents.data(), contents.length()));


    for (std::vector<shared_ptr<Aya::Instance>>::const_iterator iter = pSelection->begin(); iter != pSelection->end(); ++iter)
    {
        dictionaryService->storeAllDescendants(*iter);
        nrDictionaryService->storeAllDescendants(*iter);
    }

    // enable following code for debugging
    if (FFlag::StudioMimeDataContainsInstancePath && pSelection->size() == 1)
        pMimeDataForClipboard->setText(("Game." + (*pSelection->begin())->getFullName()).c_str());

    // set mime data in clipboard
    pClipboard->setMimeData(pMimeDataForClipboard);
}

PasteVerb::PasteVerb(Aya::DataModel* dataModel, bool pasteInto)
    : Aya::Verb(dataModel, pasteInto ? "PasteInto" : "Paste")
    , m_pDataModel(dataModel)
    , m_bPasteInto(pasteInto)
{
}

bool PasteVerb::isEnabled() const
{
    if (!isPasteInfoAvailable())
        return false;

    if (!m_bPasteInto)
        return true;

    // for paste into (used in tree view contextual menu)
    Aya::Selection* pSelection = Aya::ServiceProvider::create<Aya::Selection>(m_pDataModel);
    return (pSelection && pSelection->size() == 1);
}

void PasteVerb::onClipboardModified()
{
    m_bIsPasteInfoAvailable = isPasteInfoAvailable();
}

bool PasteVerb::isPasteInfoAvailable() const
{
    QClipboard* pClipboard = QApplication::clipboard();
    if (pClipboard)
    {
        const QMimeData* pMimeData = pClipboard->mimeData();
        if (pMimeData && pMimeData->hasFormat(sRobloxMimeType))
            return true;
    }
    return false;
}

void PasteVerb::doIt(Aya::IDataState* dataState)
{
    shared_ptr<Aya::Instances> itemsToPaste(new Instances);
    createInstancesFromClipboard(itemsToPaste);

    if (itemsToPaste->empty())
        return;

    insertInstancesIntoParent(itemsToPaste);

    // Save Undo/Redo state
    {
        DataModel::LegacyLock lock(m_pDataModel, DataModelJob::Write);
        Aya::ChangeHistoryService::requestWaypoint(getName().c_str(), m_pDataModel);
    }

    dataState->setDirty(true);
}

void PasteVerb::createInstancesFromClipboard(shared_ptr<Aya::Instances> itemsToPaste)
{
    QClipboard* pClipboard = QApplication::clipboard();
    const QMimeData* pMimeData = pClipboard->mimeData();
    if (!pMimeData || !pMimeData->hasFormat(sRobloxMimeType))
        return;

    QByteArray robloxData = pMimeData->data(sRobloxMimeType);
    std::istringstream stream(std::string(robloxData.data(), robloxData.data() + robloxData.size()));

    try
    {
        Serializer().loadInstances(stream, *itemsToPaste);
    }
    catch (...)
    {
        // clear clipboard
        pClipboard->clear();
    }
}

void PasteVerb::insertInstancesIntoParent(shared_ptr<Aya::Instances> itemsToPaste)
{
    Aya::Selection* pSelection = Aya::ServiceProvider::create<Aya::Selection>(m_pDataModel);
    if (!pSelection)
        return;

    shared_ptr<Aya::Instance> sel = pSelection->front();
    Aya::Instance* pParentInstance;

    bool isDecal = false;

    isDecal = itemsToPaste->size() == 1 && Aya::Instance::fastSharedDynamicCast<Aya::Decal>(*itemsToPaste->begin());

    if (pSelection->size() != 1 || (isDecal && !m_bPasteInto))
        pParentInstance = m_pDataModel->getWorkspace();
    else
        pParentInstance = m_bPasteInto || !sel->getParent() || !sel->getParent()->getParent() ? sel.get() : sel->getParent();

    if (!pParentInstance)
        return;

    // insert items into appropriate parent
    Aya::InsertMode insertMode = m_bPasteInto ? Aya::INSERT_TO_TREE : Aya::INSERT_TO_3D_VIEW;
    if (isDecal)
        insertMode = Aya::INSERT_TO_TREE;

    m_pDataModel->getWorkspace()->insertPasteInstances(*itemsToPaste, pParentInstance, insertMode, Aya::SUPPRESS_PROMPTS);
    pSelection->setSelection(itemsToPaste);
}

void PasteVerb::createInstancesFromClipboardDep(Aya::Instances& itemsToPaste)
{
    QClipboard* pClipboard = QApplication::clipboard();
    const QMimeData* pMimeData = pClipboard->mimeData();
    if (!pMimeData || !pMimeData->hasFormat(sRobloxMimeType))
        return;

    QByteArray robloxData = pMimeData->data(sRobloxMimeType);
    std::istringstream stream(std::string(robloxData.data(), robloxData.data() + robloxData.size()));

    try
    {
        Serializer().loadInstances(stream, itemsToPaste);
    }
    catch (...)
    {
        // clear clipboard
        pClipboard->clear();
    }
}

void PasteVerb::insertInstancesIntoParentDep(Aya::Instances& itemsToPaste)
{
    Aya::Selection* pSelection = Aya::ServiceProvider::create<Aya::Selection>(m_pDataModel);
    if (!pSelection)
        return;

    shared_ptr<Aya::Instance> sel = pSelection->front();
    Aya::Instance* pParentInstance;

    bool isDecal = false;

    isDecal = itemsToPaste.size() == 1 && Aya::Instance::fastSharedDynamicCast<Aya::Decal>(*itemsToPaste.begin());

    if (pSelection->size() != 1 || (isDecal && !m_bPasteInto))
        pParentInstance = m_pDataModel->getWorkspace();
    else
        pParentInstance = m_bPasteInto || !sel->getParent() || !sel->getParent()->getParent() ? sel.get() : sel->getParent();

    if (!pParentInstance)
        return;

    // insert items into appropriate parent
    Aya::InsertMode insertMode = m_bPasteInto ? Aya::INSERT_TO_TREE : Aya::INSERT_TO_3D_VIEW;
    if (isDecal)
        insertMode = Aya::INSERT_TO_TREE;

    m_pDataModel->getWorkspace()->insertPasteInstances(itemsToPaste, pParentInstance, insertMode, Aya::SUPPRESS_PROMPTS);
    pSelection->setSelection(itemsToPaste.begin(), itemsToPaste.end());
}

DuplicateSelectionVerb::DuplicateSelectionVerb(Aya::DataModel* dataModel)
    : EditSelectionVerb("Duplicate", dataModel)
{
}

void DuplicateSelectionVerb::doIt(Aya::IDataState*)
{
    Aya::Selection* selection = Aya::ServiceProvider::create<Aya::Selection>(dataModel);
    if (!selection)
        return;

    if (DFFlag::RestoreTransparencyOnToolChange)
        Aya::AdvArrowToolBase::restoreSavedPartsTransparency();

    shared_ptr<Instances> instances(new Instances);
    std::vector<Aya::PVInstance*> pvInstances;

    CSGDictionaryService* dictionaryService = Aya::ServiceProvider::create<CSGDictionaryService>(dataModel);
    NonReplicatedCSGDictionaryService* nrDictionaryService = Aya::ServiceProvider::create<NonReplicatedCSGDictionaryService>(dataModel);

    for (std::vector<shared_ptr<Aya::Instance>>::const_iterator iter = selection->begin(); iter != selection->end(); ++iter)
    {
        if (shared_ptr<PartOperation> partOperation = Aya::Instance::fastSharedDynamicCast<PartOperation>(*iter))
        {
            dictionaryService->retrieveData(*partOperation);
            nrDictionaryService->retrieveData(*partOperation);
        }

        if ((*iter)->getParent() && (*iter)->getParent()->getParent() && !(*iter)->getIsParentLocked())
        {
            if (shared_ptr<Aya::Instance> clonedInstance = (*iter)->clone(Aya::SerializationCreator))
            {
                clonedInstance->setParent((*iter)->getParent());

                instances->push_back(clonedInstance);

                if (DFFlag::DraggerUsesNewPartOnDuplicate)
                    MoveResizeJoinTool::setSelection(*iter, clonedInstance);

                if (Aya::AdvArrowTool::advCollisionCheckMode && clonedInstance->isDescendantOf(dataModel->getWorkspace()))
                    if (shared_ptr<Aya::PVInstance> pvInstance = Aya::Instance::fastSharedDynamicCast<Aya::PVInstance>(clonedInstance))
                        pvInstances.push_back(pvInstance.get());
            }
        }

        if (shared_ptr<PartOperation> partOperation = Aya::Instance::fastSharedDynamicCast<PartOperation>(*iter))
        {
            dictionaryService->storeData(*partOperation);
            nrDictionaryService->storeData(*partOperation);
        }
    }

    if (Aya::AdvArrowTool::advCollisionCheckMode && pvInstances.size() > 0)
    {
        Aya::MegaDragger megaDragger(NULL, pvInstances, dataModel->getWorkspace());
        megaDragger.startDragging();
        megaDragger.safeMoveNoDrop(Aya::Vector3(0.0f, 0.0f, 0.0f));
        megaDragger.finishDragging();
    }

    if (instances->size() == 0)
        return;

    selection->setSelection(instances);

    {
        DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
        Aya::ChangeHistoryService::requestWaypoint(getName().c_str(), dataModel);
    }

    dataModel->setDirty(true);
}

UndoVerb::UndoVerb(Aya::VerbContainer* pVerbContainer)
    : Verb(pVerbContainer, "UndoVerb")
    , m_pDataModel(dynamic_cast<Aya::DataModel*>(pVerbContainer))
{
    m_pChangeHistory = Aya::shared_from(m_pDataModel->create<Aya::ChangeHistoryService>());
    AYAASSERT(m_pChangeHistory);
}

void UndoVerb::doIt(Aya::IDataState*)
{
    UpdateUIManager::Instance().waitForLongProcess("Undo", boost::bind(&Aya::ChangeHistoryService::unplay, m_pChangeHistory.get()));
}

bool UndoVerb::isEnabled() const
{
    return m_pChangeHistory->canUnplay();
}

std::string UndoVerb::getText() const
{
    std::string name;
    m_pChangeHistory->getUnplayWaypoint(name);

    return std::string("Undo " + name);
}

RedoVerb::RedoVerb(Aya::VerbContainer* pVerbContainer)
    : Verb(pVerbContainer, "RedoVerb")
    , m_pDataModel(dynamic_cast<Aya::DataModel*>(pVerbContainer))
{
    m_pChangeHistory = Aya::shared_from(m_pDataModel->create<Aya::ChangeHistoryService>());
    AYAASSERT(m_pChangeHistory);
}

void RedoVerb::doIt(Aya::IDataState*)
{
    UpdateUIManager::Instance().waitForLongProcess("Redo", boost::bind(&Aya::ChangeHistoryService::play, m_pChangeHistory.get()));
}

bool RedoVerb::isEnabled() const
{
    return m_pChangeHistory->canPlay();
}

std::string RedoVerb::getText() const
{
    std::string name;
    m_pChangeHistory->getPlayWaypoint(name);

    return std::string("Redo " + name);
}

InsertModelVerb::InsertModelVerb(Aya::VerbContainer* pVerbContainer, bool insertInto)
    : Verb(pVerbContainer, insertInto ? "InsertIntoFromFileVerb" : "InsertModelVerb")
    , m_pDataModel(dynamic_cast<Aya::DataModel*>(pVerbContainer))
    , m_bInsertInto(insertInto)
{
}

void InsertModelVerb::doIt(Aya::IDataState*)
{
    insertModel();
}

void InsertModelVerb::insertModel()
{
    RobloxSettings settings;

    QString rbxmLastDir = settings.value("rbxm_last_directory").toString();
    if (rbxmLastDir.isEmpty())
        rbxmLastDir = RobloxMainWindow::getDefaultSavePath();

    QString dlgTitle = m_bInsertInto ? QObject::tr("Open file to Insert") : QObject::tr("Open Roblox Model");
    QString fileExtn = m_bInsertInto ? QObject::tr("Roblox Model Files (*.rbxm *.rbxmx);;Scripts (*.rbxs *.lua *.txt)")
                                     : QObject::tr("Roblox Model Files (*.rbxm *.rbxmx)");

    QString fileName;

    fileName = QFileDialog::getOpenFileName(&UpdateUIManager::Instance().getMainWindow(), dlgTitle, rbxmLastDir, fileExtn);

    if (fileName.isEmpty())
        return;

    if (fileName.endsWith(".rbxm", Qt::CaseInsensitive) || fileName.endsWith(".rbxmx", Qt::CaseInsensitive))
        StudioUtilities::insertModel(Aya::shared_from(m_pDataModel), fileName, m_bInsertInto);
    else
        StudioUtilities::insertScript(Aya::shared_from(m_pDataModel), fileName);

    settings.setValue("rbxm_last_directory", QFileInfo(fileName).absolutePath());
}

SelectionSaveToFileVerb::SelectionSaveToFileVerb(Aya::VerbContainer* pVerbContainer)
    : Verb(pVerbContainer, "SelectionSaveToFile")
    , m_pDataModel(dynamic_cast<Aya::DataModel*>(pVerbContainer))
{
}

void SelectionSaveToFileVerb::doIt(Aya::IDataState*)
{
    saveToFile();
}

void SelectionSaveToFileVerb::saveToFile()
{
    RobloxSettings settings;

    QString rbxmLastDir = settings.value("rbxm_last_directory").toString();
    if (rbxmLastDir.isEmpty())
        rbxmLastDir = RobloxMainWindow::getDefaultSavePath();

    QString fileExtnModels("Roblox XML Model Files (*.rbxmx);;Roblox Model Files (*.rbxm)");
    QString fileExtnScripts("Roblox Lua Scripts (*.lua)");

    QString fileExtn = fileExtnModels;

    Aya::Selection* pSelection = Aya::ServiceProvider::create<Aya::Selection>(m_pDataModel);

    if (pSelection && (pSelection->size() == 1) && Aya::Instance::fastDynamicCast<Aya::Script>(pSelection->front().get()))
    {
        // update default file name for save
        rbxmLastDir.append("/");
        rbxmLastDir.append(pSelection->front()->getName().c_str());

        // if script doesn't have any children then default to lua
        if (!pSelection->front()->numChildren())
        {
            fileExtn = fileExtnScripts + ";;" + fileExtnModels;
            rbxmLastDir.append(".lua");
        }
        else
        {
            fileExtn = fileExtnModels + ";;" + fileExtnScripts;
            rbxmLastDir.append(".rbxm");
        }
    }

    QString fileName = QFileDialog::getSaveFileName(&UpdateUIManager::Instance().getMainWindow(), "Save As", rbxmLastDir, fileExtn);

    if (fileName.isEmpty())
        return;

    DataModel::LegacyLock lock(m_pDataModel, DataModelJob::Write);

    Aya::CSGDictionaryService* dictionaryService = Aya::ServiceProvider::create<Aya::CSGDictionaryService>(m_pDataModel);
    Aya::NonReplicatedCSGDictionaryService* nrDictionaryService = Aya::ServiceProvider::create<Aya::NonReplicatedCSGDictionaryService>(m_pDataModel);

    for (std::vector<shared_ptr<Aya::Instance>>::const_iterator iter = pSelection->begin(); iter != pSelection->end(); ++iter)
    {
        dictionaryService->retrieveAllDescendants(*iter);
        nrDictionaryService->retrieveAllDescendants(*iter);
    }

    if (fileName.endsWith(".rbxm", Qt::CaseInsensitive) || fileName.endsWith(".rbxmx", Qt::CaseInsensitive))
    {
        QByteArray ba = fileName.toUtf8();
        const char* c_str = ba.constData();

        // Stream the XML data
        std::ofstream stream;
        stream.open(c_str, std::ios_base::out | std::ios::binary);

        const bool useBinaryFormat = !(fileName.endsWith(".rbxmx", Qt::CaseInsensitive));

        if (useBinaryFormat)
        {
            Aya::Selection* pSelection = Aya::ServiceProvider::create<Aya::Selection>(m_pDataModel->getWorkspace());

            Aya::Instances instances(pSelection->begin(), pSelection->end());
            Aya::SerializerBinary::serialize(stream, instances);
        }
        else
        {
            TextXmlWriter machine(stream);

            UpdateUIManager::Instance().waitForLongProcess("Saving", boost::bind(&TextXmlWriter::serialize, &machine, writeSelection().get()));
        }
    }
    else
    {
        QString scriptText;
        {
            shared_ptr<Aya::Script> spScriptInstance = Aya::Instance::fastSharedDynamicCast<Aya::Script>(pSelection->front());
            if (spScriptInstance)
            {
                if (spScriptInstance->isCodeEmbedded())
                {
                    scriptText = spScriptInstance->getEmbeddedCodeSafe().getSource().c_str();
                }
                else
                {
                    Aya::ContentProvider* pContentProvider = Aya::ServiceProvider::create<Aya::ContentProvider>(spScriptInstance.get());
                    if (pContentProvider)
                    {
                        std::auto_ptr<std::istream> stream = pContentProvider->getContent(spScriptInstance->getScriptId());
                        std::string data = std::string(static_cast<std::stringstream const&>(std::stringstream() << stream->rdbuf()).str());
                        scriptText = data.c_str();
                    }
                }
            }
        }

        QFile file(fileName);
        if (!scriptText.isEmpty() && file.open(QFile::WriteOnly | QFile::Text))
        {
            QTextStream out(&file);
            out << scriptText;
        }
    }

    settings.setValue("rbxm_last_directory", QFileInfo(fileName).absolutePath());

    for (std::vector<shared_ptr<Aya::Instance>>::const_iterator iter = pSelection->begin(); iter != pSelection->end(); ++iter)
    {
        dictionaryService->storeAllDescendants(*iter);
        nrDictionaryService->storeAllDescendants(*iter);
    }
}

bool SelectionSaveToFileVerb::isEnabled() const
{
    Aya::Selection* pSelection = Aya::ServiceProvider::create<Aya::Selection>(m_pDataModel);
    return (pSelection && (pSelection->size() > 0));
}

std::auto_ptr<XmlElement> SelectionSaveToFileVerb::writeSelection()
{
    std::auto_ptr<XmlElement> root(Serializer::newRootElement());
    {
        Aya::DataModel::LegacyLock lock(m_pDataModel, Aya::DataModelJob::Write);
        Aya::Selection* pSelection = Aya::ServiceProvider::create<Aya::Selection>(m_pDataModel->getWorkspace());
        Aya::AddSelectionToRoot(root.get(), pSelection, Aya::SerializationCreator);
    }

    return root;
}

PublishToRobloxAsVerb::PublishToRobloxAsVerb(Aya::VerbContainer* pVerbContainer, RobloxMainWindow* mainWnd)
    : Verb(pVerbContainer, "PublishToRobloxAsVerb")
    , m_pDataModel(dynamic_cast<Aya::DataModel*>(pVerbContainer))
    , m_pMainWindow(mainWnd)
    , m_dlg(NULL)
{
}

PublishToRobloxAsVerb::~PublishToRobloxAsVerb()
{
    if (m_dlg)
        delete m_dlg;
}

bool PublishToRobloxAsVerb::isEnabled() const
{
    return true;
}

void PublishToRobloxAsVerb::initDialog()
{
    static QMutex mutex;

    mutex.lock();
    try
    {
        QString initialUrl = QString("%1/IDE/publishas").arg(RobloxSettings::getBaseURL());

        if (!m_dlg)
        {
            m_dlg = new WebDialog(m_pMainWindow, initialUrl, m_pDataModel);
            m_dlg->setMinimumSize(FInt::StudioWebDialogMinimumWidth, FInt::StudioWebDialogMinimumHeight);
        }
        else
            m_dlg->load(initialUrl);
    }
    catch (std::exception& e)
    {
        // Aya::Log::current()->writeEntry(Aya::Log::Error, e.what());
    }
    mutex.unlock();
}

void PublishToRobloxAsVerb::doIt(Aya::IDataState*)
{
    // autosave before we publish just in case
    RobloxDocManager::Instance().getPlayDoc()->autoSave(true);

    if (!StudioUtilities::checkNetworkAndUserAuthentication())
        return;

    Aya::DataModel::LegacyLock lock(m_pDataModel, Aya::DataModelJob::Write);

    if (FFlag::PrefetchResourcesEnabled)
        initDialog();
    else
    {
        // "http://www.roblox.com/IDE/Upload.aspx"
        QString initialUrl = QString("%1/IDE/Upload.aspx").arg(RobloxSettings::getBaseURL());

        if (!m_dlg)
        {
            m_dlg = new WebDialog(m_pMainWindow, initialUrl, m_pDataModel);
            m_dlg->setMinimumSize(FInt::StudioWebDialogMinimumWidth, FInt::StudioWebDialogMinimumHeight);
        }
        else
            m_dlg->load(initialUrl);
    }

    m_dlg->show();
    m_dlg->raise();
    m_dlg->activateWindow();
}

QDialog* PublishToRobloxAsVerb::getPublishDialog()
{
    return qobject_cast<QDialog*>(m_dlg);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Bind fn for Publish Selection to Roblox
static void AnimationResponse(std::string* response, std::exception*, shared_ptr<Aya::Animation> animation)
{
    if (response)
    {
        Aya::DataModel::LegacyLock lock(Aya::DataModel::get(animation.get()), Aya::DataModelJob::Write);
        int newAssetId;
        std::stringstream istream(*response);
        istream >> newAssetId;

        QString baseUrl = RobloxSettings::getBaseURL();
        animation->setAssetId(Aya::format("%s/Asset?ID=%d", qPrintable(baseUrl), newAssetId));
    }
}

static void SaveDecalAssetId(shared_ptr<Aya::Decal> decal, std::string assetId)
{
    decal->setTexture(assetId);
}

static void SaveMeshMeshId(shared_ptr<Aya::SpecialShape> mesh, std::string assetId)
{
    mesh->setMeshId(assetId);
}

static void SaveMeshTextureId(shared_ptr<Aya::SpecialShape> mesh, std::string assetId)
{
    mesh->setTextureId(assetId);
}

static void PostSaveHelper(boost::function<void(std::string)> saveFunction, std::string assetId)
{
    saveFunction(assetId);
}

static void PostFromContentProvider(Aya::AsyncHttpQueue::RequestResult result, std::istream* stream, int type, std::string name,
    weak_ptr<Aya::DataModel> weakDataModel, boost::function<void(std::string)> saveFunction)
{
    QString baseUrl = RobloxSettings::getBaseURL();
    if (result == Aya::AsyncHttpQueue::Succeeded)
    {
        Aya::Http http(Aya::format("%s/Data/NewAsset.ashx?type=%d&Name=%s&Description=%s", qPrintable(baseUrl), type, name.c_str(), name.c_str()));
        try
        {
            std::string response;
            http.post(*stream, Aya::Http::kContentTypeDefaultUnspecified, true, response);

            int newAssetId;
            std::stringstream istream(response);
            istream >> newAssetId;

            if (shared_ptr<Aya::DataModel> dataModel = weakDataModel.lock())
            {
                dataModel->submitTask(boost::bind(&PostSaveHelper, saveFunction, Aya::format("%s/Asset?ID=%d", qPrintable(baseUrl), newAssetId)),
                    Aya::DataModelJob::Write);
            }
        }
        catch (std::exception&)
        {
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////

PublishSelectionToRobloxVerb::PublishSelectionToRobloxVerb(Aya::VerbContainer* pVerbContainer, RobloxMainWindow* mainWnd)
    : Verb(pVerbContainer, "PublishSelectionToRobloxVerb")
    , m_pDataModel(dynamic_cast<Aya::DataModel*>(pVerbContainer))
    , m_pMainWindow(mainWnd)
    , m_dlg(NULL)
{
}

PublishSelectionToRobloxVerb::~PublishSelectionToRobloxVerb()
{
    if (m_dlg)
        delete m_dlg;
}

bool PublishSelectionToRobloxVerb::isEnabled() const
{
    Aya::Selection* pSelection = Aya::ServiceProvider::create<Aya::Selection>(m_pDataModel);
    return (pSelection && (pSelection->size() > 0));
}

void PublishSelectionToRobloxVerb::doIt(Aya::IDataState*)
{
    // autosave before we publish just in case
    RobloxDocManager::Instance().getPlayDoc()->autoSave(true);

    if (!StudioUtilities::checkNetworkAndUserAuthentication())
        return;

    Aya::DataModel::LegacyLock lock(m_pDataModel, Aya::DataModelJob::Write);

    bool isScript = false;
    if (RbxWorkspace::isScriptAssetUploadEnabled)
    {

        Aya::Selection* sel = Aya::ServiceProvider::find<Aya::Selection>(m_pDataModel);
        if (sel && sel->size() == 1)
            if (dynamic_cast<Aya::BaseScript*>(sel->front().get()))
                isScript = true;
    }
    if (RbxWorkspace::isImageModelAssetUploadEnabled)
    {
        Aya::Selection* sel = Aya::ServiceProvider::find<Aya::Selection>(m_pDataModel);
        if (sel && sel->size() == 1)
        {
            if (Aya::Decal* decal = dynamic_cast<Aya::Decal*>(sel->front().get()))
            {
                if (!decal->getTexture().isHttp())
                {
                    boost::function<void(std::string)> foo = boost::bind(&SaveDecalAssetId, shared_from(decal), _1);
                    Aya::ServiceProvider::create<Aya::ContentProvider>(m_pDataModel)
                        ->getContent(decal->getTexture(), Aya::ContentProvider::PRIORITY_MFC,
                            boost::bind(&PostFromContentProvider, _1, _2, 1, decal->getName(), weak_from(Aya::DataModel::get(decal)), foo));

                    return;
                }
            }
            if (Aya::SpecialShape* mesh = dynamic_cast<Aya::SpecialShape*>(sel->front().get()))
            {
                bool uploadedSomething = false;
                if (!mesh->getMeshId().isHttp())
                {
                    boost::function<void(std::string)> foo = boost::bind(&SaveMeshMeshId, shared_from(mesh), _1);
                    Aya::ServiceProvider::create<Aya::ContentProvider>(m_pDataModel)
                        ->getContent(mesh->getMeshId(), Aya::ContentProvider::PRIORITY_MFC,
                            boost::bind(&PostFromContentProvider, _1, _2, 4, mesh->getName(), weak_from(Aya::DataModel::get(mesh)), foo));
                    uploadedSomething = true;
                }
                if (!mesh->getTextureId().isHttp())
                {
                    boost::function<void(std::string)> foo = boost::bind(&SaveMeshTextureId, shared_from(mesh), _1);
                    Aya::ServiceProvider::create<Aya::ContentProvider>(m_pDataModel)
                        ->getContent(mesh->getTextureId(), Aya::ContentProvider::PRIORITY_MFC,
                            boost::bind(&PostFromContentProvider, _1, _2, 1, mesh->getName(), weak_from(Aya::DataModel::get(mesh)), foo));
                    uploadedSomething = true;
                }
                if (uploadedSomething)
                {
                    return;
                }
            }
        }
    }

    bool isAnimation = false;
    Aya::Selection* sel = Aya::ServiceProvider::find<Aya::Selection>(m_pDataModel);
    if (sel && sel->size() == 1)
    {
        if (dynamic_cast<Aya::KeyframeSequence*>(sel->front().get()))
        {
            isAnimation = true;
        }
    }

    // "http://www.roblox.com/UI/Save.aspx"
    QString initialUrl;
    if (isScript)
        initialUrl = QString("%1/UI/Save.aspx?type=Lua").arg(RobloxSettings::getBaseURL());
    else if (isAnimation)
    {
        initialUrl = QString("%1/studio/animations/publish").arg(RobloxSettings::getBaseURL());
        if (FFlag::StudioEnableGameAnimationsTab)
        {
            RobloxGameExplorer& gameExplorer = UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER);
            if (gameExplorer.getCurrentGameId() > 0)
                initialUrl.append(QString("?universeId=%1").arg(gameExplorer.getCurrentGameId()));
        }
    }
    else
        initialUrl = QString("%1/UI/Save.aspx?type=Model").arg(RobloxSettings::getBaseURL());

    if (!m_dlg)
    {
        m_dlg = new WebDialog(m_pMainWindow, initialUrl, m_pDataModel);
        m_dlg->setMinimumSize(FInt::StudioWebDialogMinimumWidth, FInt::StudioWebDialogMinimumHeight);
    }
    else
        m_dlg->load(initialUrl);

    m_dlg->show();
    m_dlg->raise();
    m_dlg->activateWindow();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

CreateNewLinkedSourceVerb::CreateNewLinkedSourceVerb(DataModel* pVerbContainer)
    : Verb(pVerbContainer, "CreateNewLinkedSourceVerb")
    , m_pDataModel(pVerbContainer)
{
}

bool CreateNewLinkedSourceVerb::isEnabled() const
{
    DataModel::LegacyLock lock(m_pDataModel, DataModelJob::Read);
    shared_ptr<LuaSourceContainer> lsc = getLuaSourceContainer();

    return (UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER).getCurrentGameId() > 0) && lsc &&
           lsc->getScriptId().isNull() && (RobloxUser::singleton().getUserId() > 0);
}

void CreateNewLinkedSourceVerb::doIt(IDataState*)
{
    std::string instanceName;
    std::string source;
    {
        DataModel::LegacyLock lock(m_pDataModel, DataModelJob::Read);
        shared_ptr<Instance> instance = getLuaSourceContainer();
        instanceName = instance->getName();
        source = LuaSourceBuffer::fromInstance(instance).getScriptText();
    }

    ScriptPickerDialog::CompletedState state;
    QString newName;
    ScriptPickerDialog dialog;
    dialog.runModal(NULL, QString::fromStdString(instanceName), &state, &newName);

    if (state != ScriptPickerDialog::Completed)
    {
        AYAASSERT(state == ScriptPickerDialog::Abandoned);
        return;
    }

    bool success;
    RobloxGameExplorer& rge = UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER);
    UpdateUIManager::Instance().waitForLongProcess("Creating LinkedSource",
        boost::bind(&CreateNewLinkedSourceVerb::doItThread, this, source, rge.getCurrentGameId(), rge.getCurrentGameGroupId(), newName, &success));
    rge.reloadDataFromWeb();

    if (success)
    {
        try
        {
            DataModel::LegacyLock lock(m_pDataModel, DataModelJob::Write);

            shared_ptr<LuaSourceContainer> lsc = getLuaSourceContainer();
            LuaSourceBuffer lsb = LuaSourceBuffer::fromInstance(lsc);
            ContentId scriptName = ContentId::fromGameAssetName(newName.toStdString());
            lsc->setScriptId(scriptName);

            ContentProvider* cp = m_pDataModel->create<ContentProvider>();
            // clear content provider cache, in case the script has been fetched previously
            cp->invalidateCache(scriptName);

            m_pDataModel->create<ChangeHistoryService>()->requestWaypoint("Store LinkedSource in cloud");
        }
        catch (const Aya::base_exception&)
        {
            // catch failure to grab legacy lock, do nothing
        }
    }
    else
    {
        QMessageBox mb;
        mb.setText("Unable to create LinkedSource, see output for details.");
        mb.setStandardButtons(QMessageBox::Ok);
        mb.exec();
    }
}

void CreateNewLinkedSourceVerb::doItThread(std::string source, int currentGameId, boost::optional<int> groupId, QString newName, bool* success)
{
    *success = false;
    try
    {
        EntityProperties createScriptAssetResponse;
        createScriptAssetResponse.setFromJsonFuture(RobloxGameExplorer::publishScriptAsset(source, boost::optional<int>(), groupId));

        int assetId = createScriptAssetResponse.get<int>("AssetId").get();

        EntityProperties createNameRequest;
        createNameRequest.set("Name", newName.toStdString());
        if (FFlag::GameExplorerUseV2AliasEndpoint)
        {
            createNameRequest.set("Type", (int)ALIAS_TYPE_Asset);
            createNameRequest.set("TargetId", assetId);
        }
        else
        {
            createNameRequest.set("AssetId", assetId);
        }

        QString postUrl;
        postUrl =
            QString(FFlag::GameExplorerUseV2AliasEndpoint ? "%1/universes/create-alias-v2?universeId=%2" : "%1/universes/create-alias?universeId=%2")
                .arg(QString::fromStdString(ContentProvider::getApiBaseUrl(RobloxSettings::getBaseURL().toStdString())))
                .arg(currentGameId);

        Http http(postUrl.toStdString());
        std::string propertiesJson = createNameRequest.asJson();
        std::istringstream propertiesStream(propertiesJson);
        std::string postResponse;
        // perform synchronous post
        http.post(propertiesStream, Http::kContentTypeApplicationJson, false, postResponse);

        *success = true;
        Aya::StandardOut::singleton()->print(MESSAGE_INFO, "Successfully created new LinkedSource");
    }
    catch (const Aya::base_exception&)
    {
        *success = false;
    }
}

shared_ptr<LuaSourceContainer> CreateNewLinkedSourceVerb::getLuaSourceContainer() const
{
    if (Selection* selection = m_pDataModel->find<Selection>())
    {
        if (selection->size() == 1)
        {
            return Instance::fastSharedDynamicCast<LuaSourceContainer>(selection->front());
        }
    }
    return shared_ptr<LuaSourceContainer>();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

PublishAsPluginVerb::PublishAsPluginVerb(Aya::VerbContainer* pVerbContainer, RobloxMainWindow* mainWnd)
    : Verb(pVerbContainer, "PublishAsPluginVerb")
    , m_pDataModel(dynamic_cast<Aya::DataModel*>(pVerbContainer))
    , m_pMainWindow(mainWnd)
    , m_dlg(NULL)
{
}

bool PublishAsPluginVerb::isEnabled() const
{
    return true;
}

void PublishAsPluginVerb::doIt(Aya::IDataState*)
{
    // autosave before we publish just in case
    RobloxDocManager::Instance().getPlayDoc()->autoSave(false /*do not force autosave*/);

    if (!StudioUtilities::checkNetworkAndUserAuthentication())
        return;

    Aya::DataModel::LegacyLock lock(m_pDataModel, Aya::DataModelJob::Write);

    QString initialUrl = QString("%1/studio/plugins/publish").arg(RobloxSettings::getBaseURL());

    if (!m_dlg)
        m_dlg.reset(new WebDialog(m_pMainWindow, initialUrl, m_pDataModel));
    else
        m_dlg->load(initialUrl);

    m_dlg->show();
    m_dlg->raise();
    m_dlg->activateWindow();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

LaunchInstancesVerb::LaunchInstancesVerb(Aya::VerbContainer* pVerbContainer)
    : Verb(pVerbContainer, "LaunchInstancesVerb")
    , m_pVerbContainer(pVerbContainer)
{
}

void LaunchInstancesVerb::doIt(Aya::IDataState* dataState)
{
    Aya::Verb* pVerb;
    switch (NameValueStoreManager::singleton().getValue("clientsAndServersOptions", "user_value").toInt())
    {
    case PLAYSOLO:
        pVerb = m_pVerbContainer->getVerb("PlaySoloVerb");
        pVerb->doIt(dataState);
        break;
    case SERVERONEPLAYER:
        pVerb = m_pVerbContainer->getVerb("StartServerVerb");
        pVerb->doIt(dataState);
        pVerb = m_pVerbContainer->getVerb("StartPlayerVerb");
        pVerb->doIt(dataState);
        break;
    case SERVERFOURPLAYERS:
        pVerb = m_pVerbContainer->getVerb("StartServerVerb");
        pVerb->doIt(dataState);
        pVerb = m_pVerbContainer->getVerb("StartPlayerVerb");
        pVerb->doIt(dataState);
        pVerb->doIt(dataState);
        pVerb->doIt(dataState);
        pVerb->doIt(dataState);
        break;
    default:
        break;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

StartServerAndPlayerVerb::StartServerAndPlayerVerb(Aya::VerbContainer* pVerbContainer)
    : Verb(pVerbContainer, "StartServerAndPlayerVerb")
    , m_pVerbContainer(pVerbContainer)
{
}

void StartServerAndPlayerVerb::doIt(Aya::IDataState* dataState)
{
    bool startServer = NameValueStoreManager::singleton().getValue("startServerCB", "checked").toBool();
    int numPlayers = NameValueStoreManager::singleton().getValue("playersMode", "user_value").toInt();

    // save values in settings (so it can be used for launching players from launched server)
    RobloxSettings settings;
    settings.setValue(sRibbonStartServerSetting, startServer);
    settings.setValue(sRibbonNumPlayerSetting, numPlayers);

    if (startServer)
    {
        // cleanup existing servers and players
        QAction* cleanupAction = UpdateUIManager::Instance().getMainWindow().cleanupServersAndPlayersAction;
        if (cleanupAction && cleanupAction->isEnabled())
            cleanupAction->trigger();

        // server will launch the players (this will make sure players are launched once the server is loaded)
        Aya::Verb* pVerb = m_pVerbContainer->getVerb("StartServerVerb");
        if (pVerb)
            pVerb->doIt(dataState);
    }
    else if (numPlayers > 0)
    {
        // if there's no server to be launched then directly launch the number of players
        launchPlayers(numPlayers);
    }
}

void StartServerAndPlayerVerb::launchPlayers(int numPlayers)
{
    UpdateUIManager::Instance().waitForLongProcess(
        "Starting players", boost::bind(&StartServerAndPlayerVerb::launchStudioInstances, m_pVerbContainer, numPlayers));
}

void StartServerAndPlayerVerb::launchStudioInstances(Aya::VerbContainer* pVerbContainer, int numPlayers)
{
    // this function will be called from a new thread
    if (numPlayers > 0)
    {
        Aya::Verb* pVerb = pVerbContainer->getVerb("StartPlayerVerb");
        if (pVerb)
        {
            for (int ii = 1; ii <= numPlayers; ++ii)
            {
                pVerb->doIt(NULL);
                if (ii < numPlayers)
                    QtUtilities::sleep(1000);
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

ServerPlayersStateInitVerb::ServerPlayersStateInitVerb(Aya::VerbContainer* pVerbContainer)
    : Verb(pVerbContainer, "ServerPlayersStateInitVerb")
{
    RobloxSettings settings;
    // initialize start server mode
    NameValueStoreManager::singleton().setValue("startServerCB", "checked", settings.value(sRibbonStartServerSetting, true).toBool());
    // initialize default number of players
    int index = settings.value(sRibbonNumPlayerSetting, 1).toInt();
    NameValueStoreManager::singleton().setValue("playersMode", "user_value", index);
    QComboBox* pComboBox = UpdateUIManager::Instance().getMainWindow().findChild<QComboBox*>("playersMode");
    if (pComboBox)
        pComboBox->setCurrentIndex(index);
}

void ServerPlayersStateInitVerb::doIt(Aya::IDataState*)
{
    RobloxSettings().setValue(sRibbonStartServerSetting, NameValueStoreManager::singleton().getValue("startServerCB", "checked").toBool());
}

bool ServerPlayersStateInitVerb::isChecked() const
{
    return RobloxSettings().value(sRibbonStartServerSetting, true).toBool();
}

CreatePluginVerb::CreatePluginVerb(Aya::VerbContainer* pVerbContainer)
    : Verb(pVerbContainer, "CreatePluginVerb")
    , m_pVerbContainer(pVerbContainer)
{
}

void CreatePluginVerb::doIt(Aya::IDataState* dataState) {}

PlaySoloVerb::PlaySoloVerb(Aya::VerbContainer* pVerbContainer)
    : Verb(pVerbContainer, "PlaySoloVerb")
    , m_pDataModel(dynamic_cast<Aya::DataModel*>(pVerbContainer))
{
}

void PlaySoloVerb::doIt(Aya::IDataState*)
{
    // These should instead come from the Settings infra, once it is ready to go to the temp location
    QString fileSaveLocation = QString("%1/visit.rbxl").arg(GetAssetFolder().c_str());
    QString errorMessage;

    // Remove debug data file, before starting new process
    if (FFlag::LuaDebugger)
        QFile::remove(StudioUtilities::getDebugInfoFile(fileSaveLocation));

    if (!StudioSerializerHelper::saveAs(fileSaveLocation, "Play Solo", false, true, m_pDataModel, errorMessage, true))
    {
        QMessageBox::critical(&UpdateUIManager::Instance().getMainWindow(), "Play Solo - Save Failure", errorMessage);
        return;
    }

    // loadfile('http://www.roblox.com/game/visit.ashx')()
    QString script;
    script = QString("loadfile(\"%1/game/visit.ashx?IsPlaySolo=1&placeId=%2&universeId=%3\")()\n")
                 .arg(RobloxSettings::getBaseURL())
                 .arg(m_pDataModel->getPlaceID())
                 .arg(m_pDataModel->getUniverseId());
    RobloxApplicationManager::instance().createNewStudioInstance(script, fileSaveLocation, true, true);
}

ManageEmulationDevVerb::ManageEmulationDevVerb(Aya::VerbContainer* pVerbContainer, QWidget* newParent)
    : Verb(pVerbContainer, "ManageEmulationDevVerb")
    , m_pDataModel(dynamic_cast<Aya::DataModel*>(pVerbContainer))
{
}

void ManageEmulationDevVerb::doIt(Aya::IDataState* dataState)
{
    ManageEmulationDeviceDialog dialog(NULL);
    dialog.exec();
}

AudioToggleVerb::AudioToggleVerb(Aya::VerbContainer* pVerbContainer)
    : Verb(pVerbContainer, "AudioEnableVerb")
    , m_pDataModel(dynamic_cast<Aya::DataModel*>(pVerbContainer))
{
}

void AudioToggleVerb::doIt(Aya::IDataState* dataState)
{
    if (m_pDataModel)
    {
        if (Aya::Soundscape::SoundService* soundService = Aya::ServiceProvider::find<Aya::Soundscape::SoundService>(m_pDataModel))
        {
            soundService->muteAllChannels(!soundService->isMuted());
        }
    }
}

bool AudioToggleVerb::isChecked() const
{
    if (m_pDataModel)
    {
        if (Aya::Soundscape::SoundService* soundService = Aya::ServiceProvider::find<Aya::Soundscape::SoundService>(m_pDataModel))
        {
            return soundService->isMuted();
        }
    }

    return false;
}

AnalyzePhysicsToggleVerb::AnalyzePhysicsToggleVerb(Aya::DataModel* pDataModel)
    : Aya::Verb(pDataModel, "AnalyzeEnableVerb")
    , m_pDataModel(pDataModel)
{
}

bool AnalyzePhysicsToggleVerb::isEnabled() const
{
    return FFlag::PhysicsAnalyzerEnabled;
}

bool AnalyzePhysicsToggleVerb::isChecked() const
{
    return PhysicsSettings::singleton().getPhysicsAnalyzerState();
}

void AnalyzePhysicsToggleVerb::startAnalyze()
{
    PhysicsSettings::singleton().setPhysicsAnalyzerState(true);
}

void AnalyzePhysicsToggleVerb::stopAnalyze()
{
    PhysicsSettings::singleton().setPhysicsAnalyzerState(false);
}

void AnalyzePhysicsToggleVerb::doIt(Aya::IDataState*)
{
    PhysicsSettings::singleton().getPhysicsAnalyzerState() ? stopAnalyze() : startAnalyze();
}

StartServerVerb::StartServerVerb(Aya::VerbContainer* pVerbContainer)
    : Verb(pVerbContainer, "StartServerVerb")
    , m_pDataModel(dynamic_cast<Aya::DataModel*>(pVerbContainer))
{
}

void StartServerVerb::doIt(Aya::IDataState*)
{
    // These should instead come from the Settings infra, once it is ready to go to the temp location
    QString fileSaveLocation = QString("ayaasset://server.rbxl");
    QString errorMessage;

    if (!StudioSerializerHelper::saveAs(fileSaveLocation, "Start Server", false, true, m_pDataModel, errorMessage, true))
    {
        QMessageBox::critical(&UpdateUIManager::Instance().getMainWindow(), "Start Server Failure", errorMessage);
        return;
    }

    // loadfile('http://www.roblox.com/game/gameserver.ashx')(<placeid>, 53640)
    QString script;
    script = QString("loadfile(\"%1/game/gameserver.ashx\")("
                     "%2, 53640, nil, nil, nil, "
                     "\"%1\", nil, nil, nil, nil, "
                     "nil, nil, nil, nil, nil, "
                     "nil, nil, %3)\n")
                 .arg(::trim_trailing_slashes(RobloxSettings::getBaseURL().toStdString()).c_str())
                 .arg(m_pDataModel->getPlaceID())
                 .arg(m_pDataModel->getUniverseId());

    if (RobloxIDEDoc::isEditMode(m_pDataModel))
        RobloxApplicationManager::instance().createNewStudioInstance(script, fileSaveLocation, true, false, NewInstanceMode_Server);
}


StartPlayerVerb::StartPlayerVerb(Aya::VerbContainer* pVerbContainer)
    : Verb(pVerbContainer, "StartPlayerVerb")
    , m_pDataModel(dynamic_cast<Aya::DataModel*>(pVerbContainer))
{
}

void StartPlayerVerb::doIt(Aya::IDataState*)
{
    // loadfile('http://www.roblox.com//game/join.ashx?UserID=0&serverPort=53640')()
    QString script;
    script = QString("loadfile(\"%1/game/join.ashx?UserID=0&serverPort=53640&universeId=%2\")()\n")
                 .arg(RobloxSettings::getBaseURL())
                 .arg(m_pDataModel->getUniverseId());

    if ((!Aya::Network::Players::clientIsPresent(m_pDataModel) && !Aya::Network::Players::findConstLocalPlayer(m_pDataModel)))
        RobloxApplicationManager::instance().createNewStudioInstance(script, QString(), true, true, NewInstanceMode_Player);
}


ToggleFullscreenVerb::ToggleFullscreenVerb(Aya::VerbContainer* container)
    : Verb(container, "ToggleFullScreen")
{
}

void ToggleFullscreenVerb::doIt(Aya::IDataState*)
{
    QAction& action = UpdateUIManager::Instance().getMainWindow().fullScreenAction();
    bool checkedState = action.isChecked();

    QMetaObject::invokeMethod(&action, "setChecked", Qt::QueuedConnection, Q_ARG(bool, !checkedState));
}

bool ToggleFullscreenVerb::isEnabled() const
{
    return true;
}

ShutdownClientVerb::ShutdownClientVerb(Aya::VerbContainer* container, IRobloxDoc* pDoc)
    : Verb(container, "ShutdownClient")
    , m_pIDEDoc(pDoc)
{
}

void ShutdownClientVerb::doIt(Aya::IDataState*)
{
    RobloxMainWindow* pMainWindow = &UpdateUIManager::Instance().getMainWindow();
    if (pMainWindow)
    {
        QMetaObject::invokeMethod(pMainWindow, "forceClose", Qt::QueuedConnection);
    }
}

ShutdownClientAndSaveVerb::ShutdownClientAndSaveVerb(Aya::VerbContainer* container, IRobloxDoc* pDoc)
    : Verb(container, "ShutdownClientAndSave")
    , m_pIDEDoc(pDoc)
{
}

void ShutdownClientAndSaveVerb::doIt(Aya::IDataState*)
{
    RobloxMainWindow* pMainWindow = &UpdateUIManager::Instance().getMainWindow();
    if (pMainWindow)
    {
        QMetaObject::invokeMethod(pMainWindow, "saveAndClose", Qt::QueuedConnection);
    }
}

LeaveGameVerb::LeaveGameVerb(Aya::VerbContainer* container, IRobloxDoc* pDoc)
    : Verb(container, "Exit")
    , m_pIDEDoc(pDoc)
{
}

void LeaveGameVerb::doIt(Aya::IDataState*)
{
    if (RobloxDocManager::Instance().getPlayDoc())
    {
        RobloxMainWindow& mainWindow = UpdateUIManager::Instance().getMainWindow();
        QMetaObject::invokeMethod(&mainWindow, "closePlayDoc", Qt::QueuedConnection);
    }
}


ToggleAxisWidgetVerb::ToggleAxisWidgetVerb(Aya::DataModel* dataModel)
    : Aya::Verb(dataModel, "ToggleAxisWidget")
    , m_pDataModel(dataModel)
{
    Aya::Workspace* pWorkspace = m_pDataModel->getWorkspace();
    if (pWorkspace)
        pWorkspace->setShowAxisWidget(UpdateUIManager::Instance().get3DAxisEnabled());
}

void ToggleAxisWidgetVerb::doIt(Aya::IDataState*)
{
    Aya::Workspace* pWorkspace = m_pDataModel->getWorkspace();
    if (pWorkspace)
    {
        bool newValue = !pWorkspace->getShowAxisWidget();
        pWorkspace->setShowAxisWidget(newValue);
        UpdateUIManager::Instance().set3DAxisEnabled(newValue);
    }
}

bool ToggleAxisWidgetVerb::isChecked() const
{
    Aya::Workspace* pWorkspace = m_pDataModel->getWorkspace();
    if (pWorkspace)
        return pWorkspace->getShowAxisWidget();
    return false;
}

Toggle3DGridVerb::Toggle3DGridVerb(Aya::DataModel* dataModel)
    : Aya::Verb(dataModel, "Toggle3DGrid")
    , m_pDataModel(dataModel)
{
    Aya::Workspace* workspace = m_pDataModel->getWorkspace();
    if (workspace)
        workspace->setShow3DGrid(UpdateUIManager::Instance().get3DGridEnabled());
}

void Toggle3DGridVerb::doIt(Aya::IDataState*)
{
    Aya::Workspace* pWorkspace = m_pDataModel->getWorkspace();
    if (pWorkspace)
    {
        bool newValue = !pWorkspace->getShow3DGrid();
        pWorkspace->setShow3DGrid(newValue);
        UpdateUIManager::Instance().set3DGridEnabled(newValue);
    }
}

bool Toggle3DGridVerb::isEnabled() const
{
    Aya::Network::Player* pLocalPlayer = Aya::Network::Players::findLocalPlayer(m_pDataModel);
    return pLocalPlayer == NULL;
}

bool Toggle3DGridVerb::isChecked() const
{
    if (isEnabled())
    {
        Aya::Workspace* pWorkspace = m_pDataModel->getWorkspace();
        if (pWorkspace)
            return pWorkspace->getShow3DGrid();
    }
    return false;
}

ToggleCollisionCheckVerb::ToggleCollisionCheckVerb(Aya::DataModel* dataModel)
    : Aya::Verb(dataModel, "ToggleCollisionCheckVerb")
{
    RobloxSettings settings;
    Aya::AdvArrowTool::advCollisionCheckMode = settings.value(sCollisionToggleModeSetting, true).toBool();
}

void ToggleCollisionCheckVerb::doIt(Aya::IDataState* dataState)
{
    Aya::AdvArrowTool::advCollisionCheckMode = !Aya::AdvArrowTool::advCollisionCheckMode;

    RobloxSettings settings;
    settings.setValue(sCollisionToggleModeSetting, Aya::AdvArrowTool::advCollisionCheckMode);
}

bool ToggleCollisionCheckVerb::isChecked() const
{
    return Aya::AdvArrowTool::advCollisionCheckMode;
}


ToggleLocalSpaceVerb::ToggleLocalSpaceVerb(Aya::DataModel* dataModel)
    : Aya::Verb(dataModel, "ToggleLocalSpaceVerb")
    , m_pDataModel(dataModel)
{
    RobloxSettings settings;
    Aya::AdvArrowTool::advLocalTranslationMode = settings.value(sLocalTranslationModeSetting, false).toBool();
    Aya::AdvArrowTool::advLocalRotationMode = settings.value(sLocalRotationModeSetting, true).toBool();
}

void ToggleLocalSpaceVerb::doIt(Aya::IDataState* dataState)
{
    Aya::MouseCommand* mouseCommand = m_pDataModel->getWorkspace()->getCurrentMouseCommand();

    if (!mouseCommand)
        return;

    if (DFFlag::UseRemoveTypeIDTricks)
    {
        if (Aya::AdvMoveTool::name() == mouseCommand->getName())
        {
            Aya::AdvArrowTool::advLocalTranslationMode = !Aya::AdvArrowTool::advLocalTranslationMode;

            RobloxSettings settings;
            settings.setValue(sLocalTranslationModeSetting, Aya::AdvArrowTool::advLocalTranslationMode);
        }
        else if (Aya::AdvRotateTool::name() == mouseCommand->getName())
        {
            Aya::AdvArrowTool::advLocalRotationMode = !Aya::AdvArrowTool::advLocalRotationMode;

            RobloxSettings settings;
            settings.setValue(sLocalRotationModeSetting, Aya::AdvArrowTool::advLocalRotationMode);
        }
    }
    else
    {
        if (typeid(Aya::AdvMoveTool) == typeid(*mouseCommand))
        {
            Aya::AdvArrowTool::advLocalTranslationMode = !Aya::AdvArrowTool::advLocalTranslationMode;

            RobloxSettings settings;
            settings.setValue(sLocalTranslationModeSetting, Aya::AdvArrowTool::advLocalTranslationMode);
        }
        else if (typeid(Aya::AdvRotateTool) == typeid(*mouseCommand))
        {
            Aya::AdvArrowTool::advLocalRotationMode = !Aya::AdvArrowTool::advLocalRotationMode;

            RobloxSettings settings;
            settings.setValue(sLocalRotationModeSetting, Aya::AdvArrowTool::advLocalRotationMode);
        }
    }
}

bool ToggleLocalSpaceVerb::isChecked() const
{
    Aya::MouseCommand* mouseCommand = m_pDataModel->getWorkspace()->getCurrentMouseCommand();

    if (!mouseCommand)
        return false;

    if (typeid(Aya::AdvMoveTool) == typeid(*mouseCommand))
    {
        return Aya::AdvArrowTool::advLocalRotationMode;
    }
    else if (typeid(Aya::AdvRotateTool) == typeid(*mouseCommand))
    {
        return Aya::AdvArrowTool::advLocalTranslationMode;
    }

    return false;
}

ExportSelectionVerb::ExportSelectionVerb(Aya::DataModel* pDataModel)
    : Aya::Verb(pDataModel, "ExportSelectionVerb")
    , m_pDataModel(pDataModel)
{
}

void ExportSelectionVerb::doIt(Aya::IDataState*)
{
    QTimer::singleShot(0, RobloxDocManager::Instance().getPlayDoc(), SLOT(exportSelection()));
}

ExportPlaceVerb::ExportPlaceVerb(Aya::DataModel* pDataModel)
    : Aya::Verb(pDataModel, "ExportPlaceVerb")
    , m_pDataModel(pDataModel)
{
}

void ExportPlaceVerb::doIt(Aya::IDataState*)
{
    QTimer::singleShot(0, RobloxDocManager::Instance().getPlayDoc(), SLOT(exportPlace()));
}


PublishToRobloxVerb::PublishToRobloxVerb(Aya::VerbContainer* pVerbContainer, RobloxMainWindow* pMainWindow)
    : Verb(pVerbContainer, "PublishToRobloxVerb")
    , m_pDataModel(dynamic_cast<Aya::DataModel*>(pVerbContainer))
    , m_pMainWindow(pMainWindow)
    , m_bIsPublishInProcess(false)
{
    // Hook up our signal from the datamodel
    m_pDataModel->saveFinishedSignal.connect(boost::bind(&PublishToRobloxVerb::onEventPublishingFinished, this));
}

void PublishToRobloxVerb::onEventPublishingFinished()
{
    m_bIsPublishInProcess = false;
}

void PublishToRobloxVerb::doIt(Aya::IDataState* dataState)
{
    // autosave before publish just in case
    RobloxDocManager::Instance().getPlayDoc()->autoSave(true);
    const char* failedToPublishErrorMsg = "Failed to Publish";

    if (!StudioUtilities::checkNetworkAndUserAuthentication())
        return;

    // check if mac banned
    if (!AuthenticationHelper::validateMachine())
        throw std::runtime_error(qPrintable(failedToPublishErrorMsg));

    std::string uploadUrl;
    if (Aya::Visit* visit = Aya::ServiceProvider::find<Aya::Visit>(m_pDataModel))
        uploadUrl = visit->getUploadUrl();

    if (!uploadUrl.empty())
    {
        { // Concurrency guard
            publishingMutex.lock();
            if (m_bIsPublishInProcess)
            {
                publishingMutex.unlock();
                throw std::runtime_error(qPrintable(failedToPublishErrorMsg));
            }

            m_bIsPublishInProcess = true;
            publishingMutex.unlock();
        }


        bool error;
        QString errorTitle;
        QString errorText;

        UpdateUIManager::Instance().waitForLongProcess(
            "Publishing", boost::bind(&PublishToRobloxVerb::save, this, Aya::ContentId(uploadUrl), &error, &errorTitle, &errorText));

        if (error)
        {
            QMessageBox::critical(m_pMainWindow, errorTitle, errorText);
        }
        else
        {
            if (!RobloxDocManager::Instance().getPlayDoc()->isLocalDoc())
            {
                RobloxDocManager::Instance().getPlayDoc()->resetDirty(m_pDataModel);
            }
            Aya::StandardOut::singleton()->printf(
                Aya::MESSAGE_INFO, "Successfully published - %s", qPrintable(RobloxDocManager::Instance().getPlayDoc()->displayName()));
        }
    }
    else
        throw Aya::runtime_error("Cannot publish without a visit script!");
}

bool PublishToRobloxVerb::isEnabled() const
{
    return (!m_bIsPublishInProcess && m_pDataModel && Aya::ServiceProvider::find<Aya::Visit>(m_pDataModel) && Aya::DataModel::canSave(m_pDataModel));
}

void PublishToRobloxVerb::save(Aya::ContentId contentID, bool* outError, QString* outErrorTitle, QString* outErrorText)
{
    AYAASSERT(outError);
    AYAASSERT(outErrorTitle);
    AYAASSERT(outErrorText);

    *outError = true;
    *outErrorTitle = "Failed to Publish";
    *outErrorText = "Failed to publish place!";

    try
    {
        if (FFlag::StudioCSGAssets)
            PartOperationAsset::publishAll(m_pDataModel);

        // it is possible to publish from build mode, where game explorer isn't initialized
        RobloxGameExplorer& rge = UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER);
        if (rge.getCurrentGameId() > 0)
            rge.publishNamedAssetsToCurrentSlot();

        *outError = false;
    }
    catch (Aya::DataModel::SerializationException e)
    {
        m_bIsPublishInProcess = false;

        shared_ptr<const Aya::Reflection::ValueTable> values(new Aya::Reflection::ValueTable);

        std::stringstream jsonStream(e.what());
        bool parsed = Aya::WebParser::parseJSONTable(e.what(), values);

        if (parsed)
        {
            Aya::Reflection::ValueTable::const_iterator itTitle = values->find("title");
            Aya::Reflection::ValueTable::const_iterator itError = values->find("error");

            if (itTitle != values->end())
                *outErrorTitle = itTitle->second.get<std::string>().c_str();

            if (itError != values->end())
                *outErrorText = itError->second.get<std::string>().c_str();

            if (itError == values->end())
            {
                Aya::StandardOut::singleton()->printf(Aya::MESSAGE_ERROR, "Error while publishing: %s", e.what());
            }
        }
        else
        {
            *outErrorText = e.what();
        }
    }
    catch (const Aya::base_exception& e)
    {
        m_bIsPublishInProcess = false;
        *outErrorTitle = "Error while publishing";
        *outErrorText = e.what();
        Aya::StandardOut::singleton()->printf(Aya::MESSAGE_ERROR, "Error while publishing: %s", e.what());
    }
    catch (...)
    {
        m_bIsPublishInProcess = false;
        // NOTE: because this function is called in waitForLong process, all exceptions are swallowed
        // without notifying user.
        throw;
    }
}
InsertAdvancedObjectViewVerb::InsertAdvancedObjectViewVerb(Aya::VerbContainer* pVerbContainer)
    : Verb(pVerbContainer, "InsertAdvancedObjectDialogVerb")
{
    // make sure we remain in sync with dockwidget's toggled state
    UpdateUIManager& uiManager = UpdateUIManager::Instance();
    QObject::connect(
        uiManager.getDockAction(eDW_BASIC_OBJECTS), SIGNAL(toggled(bool)), uiManager.getAction("actionInsertAdvancedObject"), SLOT(toggle()));
}

InsertAdvancedObjectViewVerb::~InsertAdvancedObjectViewVerb()
{
    UpdateUIManager& uiManager = UpdateUIManager::Instance();
    QObject::disconnect(
        uiManager.getDockAction(eDW_BASIC_OBJECTS), SIGNAL(toggled(bool)), uiManager.getAction("actionInsertAdvancedObject"), SLOT(toggle()));
}

void InsertAdvancedObjectViewVerb::doIt(Aya::IDataState* dataState)
{
    if (UpdateUIManager::Instance().getDockAction(eDW_BASIC_OBJECTS))
        UpdateUIManager::Instance().getDockAction(eDW_BASIC_OBJECTS)->trigger();
}

bool InsertAdvancedObjectViewVerb::isEnabled() const
{
    return RobloxDocManager::Instance().getPlayDoc() != NULL;
}

bool InsertAdvancedObjectViewVerb::isChecked() const
{
    return UpdateUIManager::Instance().getDockWidget(eDW_BASIC_OBJECTS)->isVisible();
}

JointToolHelpDialogVerb::JointToolHelpDialogVerb(Aya::VerbContainer* pVerbContainer)
    : Verb(pVerbContainer, "JointToolHelpDialogVerb")
{
}

void JointToolHelpDialogVerb::doIt(Aya::IDataState* dataState) {}

StudioMaterialVerb::StudioMaterialVerb(Aya::DataModel* dataModel)
    : MaterialVerb(dataModel, "StudioMaterialVerb")
{
    // initialize default value
    StudioMaterialVerb::sMaterialActionActAsTool = RobloxSettings().value("rbxMaterialActionActAsTool", false).toBool();
    ;
}

void StudioMaterialVerb::doIt(Aya::IDataState* dataState)
{
    // set material
    QString currentMaterial = NameValueStoreManager::singleton().getValue("actionMaterialSelector", "user_value").toString();
    if (currentMaterial.isEmpty())
        return;

    // check if we need to execute action as tool
    if (StudioMaterialVerb::sMaterialActionActAsTool)
    {
        // set material
        Aya::MaterialTool::material = Aya::MaterialVerb::parseMaterial(currentMaterial.toStdString());
        // get verb associated with tool and execute
        Aya::Verb* materialVerb = dataModel->getVerb("MaterialTool");
        if (materialVerb)
            materialVerb->doIt(dataState);
    }
    else
    {
        Aya::MaterialVerb::setCurrentMaterial(Aya::MaterialVerb::parseMaterial(currentMaterial.toStdString()));
        // execute verb
        Aya::MaterialVerb::doIt(dataState);
    }
}

bool StudioMaterialVerb::isChecked() const
{
    if (StudioMaterialVerb::sMaterialActionActAsTool)
    {
        Aya::Verb* materialVerb = dataModel->getVerb("MaterialTool");
        if (materialVerb)
            return materialVerb->isChecked();
    }

    return false;
}

StudioColorVerb::StudioColorVerb(Aya::DataModel* dataModel)
    : ColorVerb(dataModel, "StudioColorVerb")
{
    addColorToIcon();
    // initialize default value
    StudioColorVerb::sColorActionActAsTool = RobloxSettings().value("rbxColorActionActAsTool", false).toBool();
}

void StudioColorVerb::doIt(Aya::IDataState* dataState)
{
    // set color
    Aya::BrickColor selectedBrickColor(NameValueStoreManager::singleton().getValue("actionColorSelector", "user_value").toInt());
    Aya::ColorVerb::setCurrentColor(selectedBrickColor);

    if (StudioColorVerb::sColorActionActAsTool)
    {
        // color for fill tool
        Aya::FillTool::color.set(selectedBrickColor);
        // execute fill tool
        Aya::Verb* fillToolVerb = dataModel->getVerb("FillTool");
        if (fillToolVerb)
            fillToolVerb->doIt(dataState);
    }
    else
    {
        // execute verb
        Aya::ColorVerb::doIt(dataState);
    }

    // updata action icon
    addColorToIcon();
}

bool StudioColorVerb::isChecked() const
{
    if (StudioColorVerb::sColorActionActAsTool)
    {
        Aya::Verb* fillToolVerb = dataModel->getVerb("FillTool");
        if (fillToolVerb)
            return fillToolVerb->isChecked();
    }

    return false;
}

void StudioColorVerb::addColorToIcon()
{
    // update icon
    QList<QAction*> colorActions = UpdateUIManager::Instance().getMainWindow().findChildren<QAction*>("actionColorSelector");
    for (int i = 0; i < colorActions.count(); i++)
    {
        QAction* pColorAction = colorActions[i];

        QColor selectedQColor = QtUtilities::toQColor(Aya::ColorVerb::getCurrentColor().color3());

        QPixmap pix = pColorAction->icon().pixmap(pColorAction->icon().availableSizes()[0]);
        QImage image = pix.toImage(); // Convert QPixmap to QImage
        image.fill(Qt::transparent);  // Fill the image with transparency

        QPainter p;
        p.begin(&image);
        QRect rect = image.rect();

        // Adjust the rectangle to make the circle smaller
        int adjustment = 2; // Adjust this value to make the circle smaller or larger
        QRect adjustedRect = rect.adjusted(adjustment, adjustment, -adjustment, -adjustment);

        p.setRenderHint(QPainter::Antialiasing); // Enable anti-aliasing
        p.setBrush(selectedQColor);
        p.drawEllipse(adjustedRect);

        // Draw a circle with a black outline
        QPen pen(QColor(91, 91, 91)); // Set the pen color to rgb(91,91,91)
        pen.setWidth(2);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(adjustedRect);

        p.end();

        pColorAction->setIcon(QIcon(QPixmap::fromImage(image))); // Convert QImage back to QPixmap
    }
}

OpenToolBoxWithOptionsVerb::OpenToolBoxWithOptionsVerb(Aya::VerbContainer* pVerbContainer)
    : Verb(pVerbContainer, "OpenToolBoxWithOptionsVerb")
{
    connect(UpdateUIManager::Instance().getDockWidget(eDW_TOOLBOX), SIGNAL(visibilityChanged(bool)), this, SLOT(handleDockVisibilityChanged(bool)));
}

OpenToolBoxWithOptionsVerb::~OpenToolBoxWithOptionsVerb()
{
    disconnect(
        UpdateUIManager::Instance().getDockWidget(eDW_TOOLBOX), SIGNAL(visibilityChanged(bool)), this, SLOT(handleDockVisibilityChanged(bool)));
}

void OpenToolBoxWithOptionsVerb::doIt(Aya::IDataState* dataState)
{
    QString setUrl = NameValueStoreManager::singleton().getValue("openToolBoxWithOptions", "user_value").toString();
    if (!setUrl.isEmpty())
    {
        RobloxMainWindow& rbxMainWindow = UpdateUIManager::Instance().getMainWindow();
        if (NameValueStoreManager::singleton().getValue("openToolBoxWithOptions", "requiresauthentication").toBool() &&
            !RobloxUser::singleton().getUserId())
        {
            QMessageBox::information(&rbxMainWindow, "Log in required", "You must log in to perform this action!", QDialogButtonBox::Ok);
            if (!rbxMainWindow.actionStartPage->isChecked())
                rbxMainWindow.actionStartPage->setChecked(true);

            rbxMainWindow.openStartPage(true, "showlogin=True");
            return;
        }

        RobloxToolBox& robloxToolBox = UpdateUIManager::Instance().getViewWidget<RobloxToolBox>(eDW_TOOLBOX);
        robloxToolBox.loadUrl(QString("%1/%2").arg(RobloxSettings::getBaseURL()).arg(setUrl));

        UpdateUIManager::Instance().setDockVisibility(eDW_TOOLBOX, true);
    }
    else
    {
        UpdateUIManager::Instance().setDockVisibility(eDW_TOOLBOX, false);
    }
}

bool OpenToolBoxWithOptionsVerb::isEnabled()
{
    return RobloxDocManager::Instance().getPlayDoc() != NULL;
}

void OpenToolBoxWithOptionsVerb::handleDockVisibilityChanged(bool isVisible)
{
    if (!isVisible)
    {
        QActionGroup* pActionGroup = UpdateUIManager::Instance().getMainWindow().findChild<QActionGroup*>("openToolBoxWithOptions");
        if (pActionGroup)
        {
            QList<QAction*> actions = pActionGroup->actions();
            for (int i = 0; i < actions.count(); i++)
                actions.at(i)->setChecked(false);
        }
    }
}

InsertBasicObjectVerb::InsertBasicObjectVerb(Aya::DataModel* dataModel)
    : Verb(dataModel, "InsertBasicObjectVerb")
    , m_pDataModel(dataModel)
{
}

void InsertBasicObjectVerb::doIt(Aya::IDataState* dataState)
{
    QString instanceName = NameValueStoreManager::singleton().getValue("insertBasicObject", "user_value").toString();
    if (instanceName.isEmpty())
        return;

    boost::shared_ptr<Aya::Instance> pInstance =
        Aya::Creatable<Aya::Instance>::createByName(Aya::Name::lookup(instanceName.toStdString()), Aya::EngineCreator);
    if (!pInstance)
    {
        // Custom logic here for data types that aren't in our instances
        if (instanceName == "Cylinder")
        {
            shared_ptr<Aya::BasicPartInstance> part = Aya::Creatable<Aya::Instance>::create<Aya::BasicPartInstance>();
            part->setLegacyPartTypeUi(Aya::BasicPartInstance::CYLINDER_LEGACY_PART);
            part->getPartPrimitive()->setSurfaceType(Aya::NORM_Y, Aya::NO_SURFACE);
            part->getPartPrimitive()->setSurfaceType(Aya::NORM_Y_NEG, Aya::NO_SURFACE);
            pInstance = part;
        }
        else if (instanceName == "Sphere")
        {
            shared_ptr<Aya::BasicPartInstance> part = Aya::Creatable<Aya::Instance>::create<Aya::BasicPartInstance>();
            part->setLegacyPartTypeUi(Aya::BasicPartInstance::BALL_LEGACY_PART);
            part->getPartPrimitive()->setSurfaceType(Aya::NORM_Y, Aya::NO_SURFACE);
            part->getPartPrimitive()->setSurfaceType(Aya::NORM_Y_NEG, Aya::NO_SURFACE);
            part->getPartPrimitive()->setSize(G3D::Vector3(4.0f, 4.0f, 4.0f));
            pInstance = part;
        }
        else
            return;
    }

    // Insert it
    InsertObjectWidget::InsertObject(pInstance, shared_from(m_pDataModel), InsertObjectWidget::InsertMode_RibbonAction);
}

bool InsertBasicObjectVerb::isEnabled()
{
    return RobloxDocManager::Instance().getPlayDoc() != NULL;
}

JointCreationModeVerb::JointCreationModeVerb(Aya::DataModel* dataModel)
    : Verb(dataModel, "JointCreationModeVerb")
{
    RobloxSettings settings;
    int jointCreationMode = settings.value(sRibbonJointCreationMode, 0).toInt();

    if (jointCreationMode == 2)
    {
        Aya::AdvArrowTool::advManualJointMode = false;
        Aya::AdvArrowTool::advCreateJointsMode = false;
    }
    else
    {
        Aya::AdvArrowTool::advManualJointMode = (bool)jointCreationMode;
        Aya::AdvArrowTool::advCreateJointsMode = true;
    }

    updateMenuActions();
    updateMenuIcon();
}

void JointCreationModeVerb::doIt(Aya::IDataState* dataState)
{
    QString jointCreationMode = NameValueStoreManager::singleton().getValue("jointCreationMode", "user_value").toString();
    if (jointCreationMode.isEmpty())
        return;

    RobloxSettings settings;

    if (jointCreationMode == "Never")
    {
        Aya::AdvArrowTool::advManualJointMode = false;
        Aya::AdvArrowTool::advCreateJointsMode = false;

        settings.setValue(sRibbonJointCreationMode, 2);
    }
    else if (jointCreationMode == "Always")
    {
        Aya::AdvArrowTool::advManualJointMode = true;
        Aya::AdvArrowTool::advCreateJointsMode = true;

        settings.setValue(sRibbonJointCreationMode, 1);
    }
    else
    {
        Aya::AdvArrowTool::advManualJointMode = false;
        Aya::AdvArrowTool::advCreateJointsMode = true;

        settings.setValue(sRibbonJointCreationMode, 0);
    }

    updateMenuIcon();
}

void JointCreationModeVerb::updateMenuIcon()
{
    QList<QMenu*> menuList = UpdateUIManager::Instance().getMainWindow().findChildren<QMenu*>("jointCreationMode");
    QActionGroup* pActionGroup = UpdateUIManager::Instance().getMainWindow().findChild<QActionGroup*>("jointCreationMode");

    if (!menuList.size() || !pActionGroup || !pActionGroup->checkedAction())
        return;

    for (int ii = 0; ii < menuList.size(); ++ii)
        menuList[ii]->setIcon(pActionGroup->checkedAction()->icon());
}

void JointCreationModeVerb::updateMenuActions()
{
    QActionGroup* pActionGroup = UpdateUIManager::Instance().getMainWindow().findChild<QActionGroup*>("jointCreationMode");
    if (pActionGroup)
    {
        QList<QAction*> actions = pActionGroup->actions();
        if (actions.count() == 3)
        {
            actions.at(0)->setChecked(Aya::AdvArrowTool::advManualJointMode && Aya::AdvArrowTool::advCreateJointsMode);
            actions.at(1)->setChecked(!Aya::AdvArrowTool::advManualJointMode && Aya::AdvArrowTool::advCreateJointsMode);
            actions.at(2)->setChecked(!Aya::AdvArrowTool::advManualJointMode && !Aya::AdvArrowTool::advCreateJointsMode);
        }
    }
}