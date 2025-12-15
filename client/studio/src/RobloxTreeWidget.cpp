

// d9mz - why the FUCK does this point to BulletPhysics stdafx??

#include "RobloxTreeWidget.hpp"

// Qt Headers
#include <QApplication>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QMenu>
#include <QScrollBar>
#include <QLineEdit>
#include <QMetaObject>
#include <QMovie>
#include <QHeaderView>
#include <QStyledItemDelegate>
#include <QDrag>

#include <algorithm>

// Roblox Headers
#include "DataModel/DataModel.hpp"

#include "DataModel/Selection.hpp"

#include "DataModel/Workspace.hpp"

#include "DataModel/ChangeHistory.hpp"

#include "DataModel/GuiObject.hpp"

#include "Script/ModuleScript.hpp"
#include "Script/script.hpp"
#include "Utility/ScopedAssign.hpp"

#include "Utility/BrickColor.hpp"

#include "Reflection/ReflectionMetadata.hpp"
#include "DataModel/PlayerGui.hpp"


// Roblox Studio Headers
#include "CommonInsertWidget.hpp"
#include "LuaSourceBuffer.hpp"
#include "QtUtilities.hpp"
#include "RobloxMainWindow.hpp"
#include "RobloxCustomWidgets.hpp"
#include "RobloxDocManager.hpp"
#include "UpdateUIManager.hpp"
#include "RobloxIDEDoc.hpp"
#include "boost/algorithm/string.hpp"

#include "RobloxSettings.hpp"

const static std::string classSearchString = "classname:";
const static std::string nameSearchString = "name:";

static int nearestWidgetItemIndex(RobloxTreeWidgetItem* pParentTWI, int first, int last, int key);
static QItemSelection mergeModelIndexes(const QList<QModelIndex>& indexes);

FASTINTVARIABLE(StudioTreeWidgetProcessingTime, 100)
FASTINTVARIABLE(StudioTreeWidgetFilterTime, 30)
FASTINTVARIABLE(StudioTreeWidgetEventProcessingTime, 10)

FASTFLAGVARIABLE(StudioDE8774CrashFixEnabled, false)
FASTFLAGVARIABLE(StudioPushTreeWidgetUpdatesToMainThread, false)
FASTFLAGVARIABLE(StudioTreeWidgetCheckDeletingFlagWhenDoingUpdates, false)

FASTFLAG(StudioMimeDataContainsInstancePath)
FASTFLAG(StudioSeparateActionByActivationMethod)

LOGGROUP(Explorer)

bool DepthCompare::operator()(const RobloxTreeWidgetItem* lhs, const RobloxTreeWidgetItem* rhs) const
{
    if (lhs->getTreeWidgetDepth() == rhs->getTreeWidgetDepth())
        return lhs < rhs;

    return rhs->getTreeWidgetDepth() < lhs->getTreeWidgetDepth();
}

InstanceUpdateHandler::InstanceUpdateHandler(boost::shared_ptr<Aya::Instance> pInstance)
    : m_pInstance(pInstance)
{
    // Attach listeners
    if (FFlag::StudioPushTreeWidgetUpdatesToMainThread)
    {
        m_cChildAddedConnection =
            m_pInstance->getOrCreateChildAddedSignal()->connect(boost::bind(&InstanceUpdateHandler::childAddedSignalHandler, this, _1));
        m_cChildRemovedConnection =
            m_pInstance->getOrCreateChildRemovedSignal()->connect(boost::bind(&InstanceUpdateHandler::childRemovedSignalHandler, this, _1));
        m_cPropertyChangedConnection =
            m_pInstance->propertyChangedSignal.connect(boost::bind(&InstanceUpdateHandler::propertyChangedSignalHandler, this, _1));
    }
    else
    {
        m_cChildAddedConnection = m_pInstance->getOrCreateChildAddedSignal()->connect(boost::bind(&InstanceUpdateHandler::onChildAdded, this, _1));
        m_cChildRemovedConnection =
            m_pInstance->getOrCreateChildRemovedSignal()->connect(boost::bind(&InstanceUpdateHandler::onChildRemoved, this, _1));
        m_cPropertyChangedConnection = m_pInstance->propertyChangedSignal.connect(boost::bind(&InstanceUpdateHandler::onPropertyChanged, this, _1));
    }
}

InstanceUpdateHandler::~InstanceUpdateHandler()
{
    m_cChildRemovedConnection.disconnect();
    m_cChildAddedConnection.disconnect();
    m_cPropertyChangedConnection.disconnect();

    m_PendingItemsToAdd.clear();
}

void InstanceUpdateHandler::populateChildren(RobloxTreeWidget* pTreeWidget)
{
    shared_ptr<Aya::Instance> inst = getInstance();

    RobloxTreeWidgetItem* pParent = getItemParent();

    for (size_t i = 0; i < inst->numChildren(); ++i)
    {
        if (Aya::Instance* pChildInst = inst->getChild(i))
        {
            RobloxTreeWidgetItem* pChildItem = pTreeWidget->findItemFromInstance(pChildInst);

            if (pChildItem)
            {
                bool currentlySelected = false;

                if (Aya::Selection* selection = Aya::ServiceProvider::find<Aya::Selection>(getInstance().get()))
                {
                    for (Aya::Instances::const_iterator iter = selection->begin(); iter != selection->end(); ++iter)
                    {
                        if ((*iter).get() == pChildInst)
                        {
                            currentlySelected = true;
                            break;
                        }
                    }
                }

                if (pParent)
                {
                    int index = pParent->getIndexToInsertAt(shared_from(pChildInst));
                    pParent->insertChild(index < 0 ? 0 : index, pChildItem);
                }
                else if (pTreeWidget)
                {
                    int index = pTreeWidget->getIndexToInsertAt(shared_from(pChildInst));
                    pTreeWidget->insertTopLevelItem(index < 0 ? 0 : index, pChildItem);
                }

                if (currentlySelected)
                    pTreeWidget->addItemToSelection(pChildInst);

                pChildItem->populateChildren(pTreeWidget);
            }
        }
    }
}

bool InstanceUpdateHandler::onChildAdded(shared_ptr<Aya::Instance> pChild)
{
    FASTLOG1(FLog::Explorer, "InstanceUpdateHander::onChildAdded, child: %p", pChild.get());

    if (!pChild || !RobloxTreeWidgetItem::isExplorerItem(pChild))
        return false;

    FASTLOGS(FLog::Explorer, "Child name: %s", pChild->getName());

    RobloxTreeWidget* pTreeWidget = getTreeWidget();
    if (!pTreeWidget || pTreeWidget->isDeletionRequested())
        return false;

    RobloxTreeWidgetItem* pTreeWidgetParentItem = pTreeWidget->findItemFromInstance(pChild->getParent());

    if (pTreeWidgetParentItem)
        pTreeWidgetParentItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

    QMutexLocker lock(pTreeWidget->treeWidgetMutex());


    RobloxTreeWidgetItem* pChildItem = pTreeWidget->findItemFromInstance(pChild.get());

    if (pChildItem && (pTreeWidgetParentItem || pTreeWidget))
    {
        pTreeWidget->removeItemFromRemovalList(pChildItem);
        bool currentlySelected = pChildItem->removeItemFromTreeWidget();

        if (pTreeWidgetParentItem)
        {
            int index = pTreeWidgetParentItem->getIndexToInsertAt(pChild);
            pTreeWidgetParentItem->insertChild(index < 0 ? 0 : index, pChildItem);
        }
        else if (pTreeWidget)
        {
            int index = pTreeWidget->getIndexToInsertAt(pChild);
            pTreeWidget->insertTopLevelItem(index < 0 ? 0 : index, pChildItem);
        }

        pChildItem->setExpandedOnce(false);

        if (currentlySelected)
            pTreeWidget->addItemToSelection(pChild.get());
    }
    else
    {
        m_PendingItemsToAdd.insert(pChild);
    }

    if (pTreeWidgetParentItem && pTreeWidgetParentItem->isExpanded())
    {
        pTreeWidgetParentItem->updateFilterItems();
    }
    pTreeWidget->addToUpdateList(pTreeWidgetParentItem);
    return true;
}

bool InstanceUpdateHandler::onChildRemoved(shared_ptr<Aya::Instance> pChild)
{
    FASTLOG1(FLog::Explorer, "InstanceUpdateHander::onChildRemoved, child: %p", pChild.get());
    if (!pChild)
        return false;

    FASTLOGS(FLog::Explorer, "Child name: %s", pChild->getName());

    RobloxTreeWidget* pTreeWidget = getTreeWidget();
    if (!pTreeWidget || pTreeWidget->isDeletionRequested())
        return false;

    QMutexLocker lock(pTreeWidget->treeWidgetMutex());
    m_PendingItemsToAdd.erase(pChild);
    m_FilterItemsToAdd.erase(pChild);
    RobloxTreeWidgetItem* pTreeWidgetItem = pTreeWidget->findItemFromInstance(pChild.get());
    if (pTreeWidgetItem)
    {
        pTreeWidget->requestItemDelete(pTreeWidgetItem);

        if (pTreeWidgetItem == pTreeWidget->lastSelectedItem())
            pTreeWidget->eraseLastSelectedItem();
    }

    return true;
}

void InstanceUpdateHandler::onPropertyChanged(const Aya::Reflection::PropertyDescriptor* pDescriptor)
{
    bool isNameChanged = (*pDescriptor == Aya::Instance::desc_Name);
    if (isNameChanged)
    {
        RobloxTreeWidget* pTreeWidget = getTreeWidget();
        if (pTreeWidget && !pTreeWidget->isDeletionRequested())
        {
            QMutexLocker lock(pTreeWidget->treeWidgetMutex());
            processPropertyChange();
        }
    }
    else if (*pDescriptor == Aya::Instance::propParent)
    {
        if (RobloxTreeWidget* pTreeWidget = getTreeWidget())
            if (!pTreeWidget->isFilterEmpty() && getInstance()->getParent())
                pTreeWidget->filterWidget();
    }
}

RobloxTreeWidgetItem* InstanceUpdateHandler::processChildAdd(shared_ptr<Aya::Instance> pInstance)
{
    RobloxTreeWidget* pTreeWidget = getTreeWidget();
    RobloxTreeWidgetItem* pWidgetItem = NULL;

    if (pTreeWidget->findItemFromInstance(pInstance.get()))
        return NULL;

    if (getItemParent())
    {
        int index = getItemParent()->getIndexToInsertAt(pInstance);
        pWidgetItem = new RobloxTreeWidgetItem((index > 0) ? index : 0, getItemParent(), pInstance);
    }
    else
    {
        int index = pTreeWidget->getIndexToInsertAt(pInstance);
        pWidgetItem = new RobloxTreeWidgetItem((index > 0) ? index : 0, pTreeWidget, pInstance);
    }

    if (pWidgetItem && pInstance->numChildren() > 0)
        pWidgetItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

    if (pWidgetItem)
        pWidgetItem->setViewState();

    pTreeWidget->updateSelectionState(pInstance.get());

    m_FilterItemsToAdd.erase(pInstance);
    m_PendingItemsToAdd.erase(pInstance);

    return pWidgetItem;
}

bool InstanceUpdateHandler::processChildrenAdd()
{
    RobloxTreeWidget* pTreeWidget = getTreeWidget();
    AYAASSERT(pTreeWidget);
    if (!pTreeWidget)
        return false;

    InstanceList_SPTR* instanceList = pTreeWidget->isFilterEmpty() ? &m_PendingItemsToAdd : &m_FilterItemsToAdd;

    if (instanceList->empty())
        return false;

    bool itemsAdded = false;

    // Lock for a few items
    QMutexLocker lock(pTreeWidget->treeWidgetMutex());
    // Do a few items
    for (int i = 0; i < 10; ++i)
    {
        InstanceList_SPTR::iterator iter = instanceList->begin();
        if (iter == instanceList->end())
            return itemsAdded;
        shared_ptr<Aya::Instance> pInstance = *iter;

        processChildAdd(pInstance);

        itemsAdded = true;
    }
    return itemsAdded;
}

void InstanceUpdateHandler::removeFromRemovalList(RobloxTreeWidgetItem* pTreeWidgetItem)
{
    m_PendingItemsToRemove.erase(pTreeWidgetItem);

    int numChild = pTreeWidgetItem->childCount();
    if (!numChild)
        return;

    int currentChild = 0;
    RobloxTreeWidgetItem* pCurrentItem = NULL;

    while (currentChild < numChild)
    {
        pCurrentItem = static_cast<RobloxTreeWidgetItem*>(pTreeWidgetItem->child(currentChild));
        if (pCurrentItem)
            removeFromRemovalList(pCurrentItem);
        ++currentChild;
    }
}

void InstanceUpdateHandler::processChildRemove()
{
    if (!m_PendingItemsToRemove.size())
        return;

    RobloxTreeWidget* pTreeWidget = getTreeWidget();

    if (FFlag::StudioDE8774CrashFixEnabled)
    {
        QMutexLocker lock(pTreeWidget->treeWidgetMutex());

        while (m_PendingItemsToRemove.begin() != m_PendingItemsToRemove.end())
        {
            RobloxTreeWidgetItem* pTreeWidgetItem = *m_PendingItemsToRemove.begin();
            if (!pTreeWidgetItem)
                continue;

            pTreeWidgetItem->aboutToDelete(pTreeWidget);
            removeFromRemovalList(pTreeWidgetItem);

            delete pTreeWidgetItem;
        }
    }
    else
    {
        std::vector<RobloxTreeWidgetItem*> itemsToRemove;

        // Copy the items to a temp collection
        {
            QMutexLocker lock(pTreeWidget->treeWidgetMutex());
            for (TreeWidgetItemList::iterator iter = m_PendingItemsToRemove.begin(); iter != m_PendingItemsToRemove.end(); ++iter)
                itemsToRemove.push_back(*iter);
            m_PendingItemsToRemove.clear();
        }

        // Process them
        RobloxTreeWidgetItem* pTreeWidgetItem = NULL;
        for (std::vector<RobloxTreeWidgetItem*>::iterator iter = itemsToRemove.begin(); iter != itemsToRemove.end(); ++iter)
        {
            pTreeWidgetItem = *iter;
            if (!pTreeWidgetItem)
                continue;
            pTreeWidgetItem->aboutToDelete(pTreeWidget);
            // if (pTreeWidgetItem->parent())
            //	pTreeWidgetItem->parent()->removeChild(pTreeWidgetItem);
            delete pTreeWidgetItem;
        }
    }
}

RobloxTreeWidgetItem::RobloxTreeWidgetItem(int index, RobloxTreeWidget* pTreeWidget, boost::shared_ptr<Aya::Instance> pInstance)
    : InstanceUpdateHandler(pInstance)
    , m_ItemInfo(0)
    , m_queuedForDeletion(false)
{
    initData();

    m_treeWidgetDepth = 1;

    // add it to map in tree view for find
    pTreeWidget->addInstance(m_pInstance.get(), this);

    // add item to tree view
    pTreeWidget->insertTopLevelItem(index, this);
}

RobloxTreeWidgetItem::RobloxTreeWidgetItem(int index, RobloxTreeWidgetItem* pParentWidgetItem, boost::shared_ptr<Aya::Instance> pInstance)
    : InstanceUpdateHandler(pInstance)
    , m_ItemInfo(0)
    , m_queuedForDeletion(false)
{
    initData();

    m_treeWidgetDepth = pParentWidgetItem->getTreeWidgetDepth() + 1;

    // add it to map in tree view for find (cannot use current item's getTreeWidget)
    pParentWidgetItem->getTreeWidget()->addInstance(m_pInstance.get(), this);

    // add item as child
    pParentWidgetItem->insertChild(index, this);
}

RobloxTreeWidgetItem::~RobloxTreeWidgetItem()
{
    RobloxTreeWidget* pTreeWidget = getTreeWidget();

    if (!pTreeWidget)
    {
        // May not be in treeWidget
        RobloxExplorerWidget* robloxExplorer = static_cast<RobloxExplorerWidget*>(UpdateUIManager::Instance().getExplorerWidget());
        pTreeWidget = robloxExplorer->getTreeWidget();
    }

    if (pTreeWidget)
    {
        pTreeWidget->removeItemFromRemovalList(this);
        pTreeWidget->eraseInstance(getInstance().get());
    }
}

void RobloxTreeWidgetItem::setData(int column, int role, const QVariant& value)
{
    // only editing of column 0 i.e. name change is to be handled
    if (column != 0 || role != Qt::EditRole)
    {
        QTreeWidgetItem::setData(column, role, value);
        return;
    }

    try
    {
        QString label = value.toString();
        if (!label.isEmpty() && label != m_pInstance->getName().c_str())
        {
            Aya::DataModel::LegacyLock lock(getTreeWidget()->dataModel(), Aya::DataModelJob::Write);
            m_pInstance->setName(label.toStdString());

            // check to see if rename failed
            if (m_pInstance->getName() == label.toStdString())
            {
                // now update the tree widget data
                QTreeWidgetItem::setData(column, role, value);

                // set waypoint
                Aya::ChangeHistoryService::requestWaypoint("Rename", getTreeWidget()->dataModel().get());
                getTreeWidget()->dataModel()->setDirty(true);
            }
        }
    }

    catch (std::exception& exp)
    {
        Aya::StandardOut::singleton()->printf(Aya::MESSAGE_ERROR, "Label edit failed : %s", exp.what());
    }
}

void RobloxTreeWidgetItem::initData()
{
    setText(0, m_pInstance->getName().c_str());
    int imageIndex = getImageIndex(m_pInstance);
    setIcon(0, QIcon(QtUtilities::getPixmap(QString::fromStdString(GetAssetFolder()) + "/textures/ClassImages.PNG", imageIndex)));

    m_ItemType = getItemType(m_pInstance);

    setDirty(true);
    setExpandedOnce(false);

    setData(1, ExpandRole, QVariant(false));

    setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);

    if (hasChildren())
        setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
}
bool RobloxTreeWidgetItem::hasChildren() const
{
    if (!m_pInstance)
        return false;

    try
    {
        const Aya::copy_on_write_ptr<Aya::Instances>& childrenPtr = m_pInstance->getChildren();
        if (!childrenPtr)
            return false;

        const Aya::Instances& childInstances = *childrenPtr;

        for (const auto& child : childInstances)
        {
            if (child && isExplorerItem(child))
                return true;
        }
    }
    catch (const std::exception& e)
    {
        Aya::StandardOut::singleton()->printf(Aya::MESSAGE_ERROR, "Error in RobloxTreeWidgetItem::hasChildren: %s", e.what());
    }

    return false;
}

void RobloxTreeWidgetItem::setViewState()
{
    RobloxTreeWidget* pTreeWidget = getTreeWidget();

    if (pTreeWidget->isFilterEmpty())
    {
        setExpanded(data(1, ExpandRole).toBool());
        setForeground(0, QBrush(Qt::black));
        setHidden(false);
        setDisabled(false);
    }
    else if (pTreeWidget->isFoundItem(getInstance()))
    {
        setExpanded(true);
        setForeground(0, QBrush(Qt::black));
        setHidden(false);
        setDisabled(false);
    }
    else if (pTreeWidget->isAncestorOfFoundItem(getInstance()))
    {
        setExpanded(true);
        setForeground(0, QBrush(Qt::gray));
        setHidden(false);
        setDisabled(false);
    }
    else
    {
        setHidden(true);
        setDisabled(true);
    }
}
RobloxTreeWidget* RobloxTreeWidgetItem::getTreeWidget()
{
    return dynamic_cast<RobloxTreeWidget*>(treeWidget());
}

RobloxTreeWidgetItem* RobloxTreeWidgetItem::getItemParent()
{
    return this;
}

void RobloxTreeWidgetItem::updateFilterItems()
{
    m_FilterItemsToAdd.clear();

    RobloxTreeWidget* pTreeWidget = getTreeWidget();
    AYAASSERT(pTreeWidget);
    if (!pTreeWidget)
        return;

    if (!pTreeWidget->isFilterEmpty())
    {
        for (std::set<shared_ptr<Aya::Instance>>::const_iterator iter = m_PendingItemsToAdd.begin(); iter != m_PendingItemsToAdd.end(); ++iter)
            if (pTreeWidget->isAncestorOfFoundItem(*iter) || pTreeWidget->isFoundItem(*iter))
                m_FilterItemsToAdd.insert(*iter);
    }
}

void RobloxTreeWidgetItem::requestItemExpand()
{
    RobloxTreeWidget* pTreeWidget = getTreeWidget();

    AYAASSERT(pTreeWidget);
    if (!pTreeWidget)
        return;

    if (pTreeWidget->isFilterEmpty())
        setData(1, ExpandRole, QVariant(true));

    if (m_pInstance->getChildren())
    {
        Aya::Instances childInstances = *m_pInstance->getChildren();
        if (childInstances.empty())
            return;

        // add child in pending queue
        if (!isExpandedOnce())
        {
            QMutexLocker lock(pTreeWidget->treeWidgetMutex());
            for (Aya::Instances::const_iterator iter = childInstances.begin(); iter != childInstances.end(); ++iter)
                if (!pTreeWidget->findItemFromInstance(iter->get()) && RobloxTreeWidgetItem::isExplorerItem(*iter))
                    m_PendingItemsToAdd.insert(*iter);
        }
    }

    setExpandedOnce(true);

    updateFilterItems();

    pTreeWidget->addToUpdateList(this);
}

void RobloxTreeWidgetItem::takeAllChildren()
{
    while (childCount() > 0)
    {
        QTreeWidgetItem* firstItem = child(0);
        RobloxTreeWidgetItem* treeItem = dynamic_cast<RobloxTreeWidgetItem*>(firstItem);

        if (treeItem)
            treeItem->takeAllChildren();

        this->takeChild(0);
    }
}

bool RobloxTreeWidgetItem::removeItemFromTreeWidget()
{
    RobloxTreeWidget* pTreeWidget = getTreeWidget();
    if (!pTreeWidget)
        return false;

    bool currentlySelected = false;

    if (Aya::Selection* selection = Aya::ServiceProvider::find<Aya::Selection>(getInstance().get()))
    {
        for (Aya::Instances::const_iterator iter = selection->begin(); iter != selection->end(); ++iter)
        {
            if (*iter == getInstance())
            {
                currentlySelected = true;
                break;
            }
        }
    }

    if (currentlySelected)
        pTreeWidget->blockSignals(true);


    if (RobloxTreeWidgetItem* previousParent = dynamic_cast<RobloxTreeWidgetItem*>(static_cast<QTreeWidgetItem*>(this)->parent()))
    {
        int childIndex = previousParent->indexOfChild(this);
        takeAllChildren();
        previousParent->takeChild(childIndex);
    }
    else
    {
        int childIndex = pTreeWidget->indexOfTopLevelItem(this);
        takeAllChildren();
        pTreeWidget->takeTopLevelItem(childIndex);
    }

    if (currentlySelected)
        pTreeWidget->blockSignals(false);

    return currentlySelected;
}

void RobloxTreeWidgetItem::aboutToDelete(RobloxTreeWidget* pTreeWidget)
{
    m_queuedForDeletion = true;

    m_cChildAddedConnection.disconnect();
    m_cChildRemovedConnection.disconnect();
    m_cPropertyChangedConnection.disconnect();

    pTreeWidget->eraseInstance(getInstance().get());
    pTreeWidget->removeItemFromRemovalList(this);

    pTreeWidget->removeFromUpdateList(this);

    int numChild = childCount();
    if (!numChild)
    {
        removeItemFromTreeWidget();
        return;
    }

    int currentChild = 0;
    RobloxTreeWidgetItem* pCurrentItem = NULL;

    while (currentChild < numChild)
    {
        pCurrentItem = static_cast<RobloxTreeWidgetItem*>(child(currentChild));
        if (pCurrentItem)
        {
            pCurrentItem->aboutToDelete(pTreeWidget);
            pCurrentItem->deleteLater();
        }
        ++currentChild;
    }

    if (!FFlag::StudioDE8774CrashFixEnabled)
        pTreeWidget->removeFromUpdateList(this);

    removeItemFromTreeWidget();
}

bool RobloxTreeWidgetItem::onChildAdded(shared_ptr<Aya::Instance> pChild)
{
    if (FFlag::StudioTreeWidgetCheckDeletingFlagWhenDoingUpdates && m_queuedForDeletion)
        return false;

    FASTLOG1(FLog::Explorer, "RobloxTreeWidgetItem::onChildAdded, child: %p", pChild.get());

    RobloxTreeWidget* pTreeWidget = getTreeWidget();
    if (!pTreeWidget)
        return false;

    FASTLOGS(FLog::Explorer, "Child name: %s", pChild->getName());

    // if not expanded once then just set dirty flag
    if (!isExpandedOnce())
    {
        if (RobloxTreeWidgetItem::isExplorerItem(pChild))
        {
            setDirty(true);
            setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
            pTreeWidget->requestViewportUpdate();
        }
        return true;
    }

    if (!InstanceUpdateHandler::onChildAdded(pChild))
        return false;

    if (isExpandedOnce())
        pTreeWidget->addToUpdateList(this);

    return true;
}

bool RobloxTreeWidgetItem::onChildRemoved(shared_ptr<Aya::Instance> pChild)
{
    if (FFlag::StudioTreeWidgetCheckDeletingFlagWhenDoingUpdates && m_queuedForDeletion)
        return false;

    FASTLOG1(FLog::Explorer, "RobloxTreeWidgetItem::onChildRemoved, child: %p", pChild.get());
    // if child item has been added before then let it get removed!
    if (!InstanceUpdateHandler::onChildRemoved(pChild))
        return false;

    FASTLOGS(FLog::Explorer, "Child name: %s", pChild->getName());

    RobloxTreeWidget* treeWidget = getTreeWidget();

    if (!treeWidget)
        return false;

    if (!hasChildren())
    {
        setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
        getTreeWidget()->requestViewportUpdate();
    }

    // request for an update
    if (isExpandedOnce())
        getTreeWidget()->addToUpdateList(this);

    return true;
}

bool RobloxTreeWidgetItem::hasChildrenToProcess()
{
    if (RobloxTreeWidget* pWidget = getTreeWidget())
        if (!pWidget->isFilterEmpty())
            return !m_FilterItemsToAdd.empty();

    return !m_PendingItemsToAdd.empty();
}

void RobloxTreeWidgetItem::processPropertyChange()
{
    setText(0, m_pInstance->getName().c_str());
}

bool RobloxTreeWidgetItem::handleOpen()
{
    if (!m_pInstance)
        return false;

    // Open script document
    return RobloxDocManager::Instance().openDoc(LuaSourceBuffer::fromInstance(m_pInstance));
}

int RobloxTreeWidgetItem::getIndexToInsertAt(shared_ptr<Aya::Instance> pInstance)
{
    int numChild = childCount();
    if (!numChild)
        return -1;

    int itemTypeToAdd = RobloxTreeWidgetItem::getItemType(pInstance);
    int nearestIndex = nearestWidgetItemIndex(this, 0, childCount() - 1, itemTypeToAdd);

    RobloxTreeWidgetItem* pCurrentItem = static_cast<RobloxTreeWidgetItem*>(child(nearestIndex));
    if (!pCurrentItem || (itemTypeToAdd != pCurrentItem->itemType()))
        return nearestIndex;

    // if the type is same then we need to insert in the order of item's name
    if ((pInstance->getName() < pCurrentItem->getInstance()->getName()))
    {
        while (nearestIndex >= 0)
        {
            pCurrentItem = static_cast<RobloxTreeWidgetItem*>(child(nearestIndex));
            if (!pCurrentItem)
                break;

            if ((itemTypeToAdd != pCurrentItem->itemType()) || (pInstance->getName() > pCurrentItem->getInstance()->getName()))
            {
                nearestIndex++;
                break;
            }

            --nearestIndex;
        }
    }
    else
    {
        while (nearestIndex < childCount())
        {
            pCurrentItem = static_cast<RobloxTreeWidgetItem*>(child(nearestIndex));
            if (!pCurrentItem || (itemTypeToAdd != pCurrentItem->itemType()) || (pInstance->getName() < pCurrentItem->getInstance()->getName()))
                break;
            ++nearestIndex;
        }
    }

    return nearestIndex;
}

bool RobloxTreeWidgetItem::isExplorerItem(const shared_ptr<Aya::Instance>& pInstance)
{
    // For everything else, check the class metadata
    boost::shared_ptr<const Aya::Reflection::Metadata::Reflection> pReflection = Aya::Reflection::Metadata::Reflection::singleton();
    Aya::Reflection::Metadata::Class* pClassData = pReflection ? pReflection->get(pInstance->getDescriptor(), true) : NULL;
    if (pClassData && pClassData->isExplorerItem())
        return true;

    return false;
}

int RobloxTreeWidgetItem::getImageIndex(const shared_ptr<Aya::Instance>& pInstance)
{
    boost::shared_ptr<const Aya::Reflection::Metadata::Reflection> pReflection = Aya::Reflection::Metadata::Reflection::singleton();
    Aya::Reflection::Metadata::Class* pClassData = pReflection ? pReflection->get(pInstance->getDescriptor(), true) : NULL;
    if (pClassData)
        return pClassData->getExplorerImageIndex();

    return 0;
}

int RobloxTreeWidgetItem::getItemType(const shared_ptr<Aya::Instance>& pInstance)
{
    boost::shared_ptr<const Aya::Reflection::Metadata::Reflection> pReflection = Aya::Reflection::Metadata::Reflection::singleton();
    Aya::Reflection::Metadata::Class* pClassData = pReflection->get(pInstance->getDescriptor(), true);
    return pClassData ? pClassData->getExplorerOrder() : 0;
}

RobloxTreeWidget::RobloxTreeWidget(boost::shared_ptr<Aya::DataModel> pDataModel)
    : InstanceUpdateHandler(pDataModel)
    , m_pDataModel(pDataModel)
    , m_treeWidgetMutex()
    , m_pInstanceSelectionHandler(NULL)
    , m_pRubberBand(NULL)
    , m_bIgnoreInstanceSelectionChanged(false)
    , m_bIgnoreItemSelectionChanged(false)
    , m_bUpdateRequested(false)
    , m_bViewportUpdateRequested(false)
    , m_bDeletionRequested(false)
    , m_currentFilter("")
    , m_filterRunning(false)
    , m_lastSelectedItem(NULL)
    , m_isActive(false)
    , m_savedMarkerItem(NULL)
{
    setHeaderHidden(true);
    setUniformRowHeights(true);

    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(true);
    setAutoScroll(true);
    setAutoScrollMargin(32);

    setAttribute(Qt::WA_PaintOnScreen, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);

    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectItems);

    header()->setStretchLastSection(false);
    header()->setSectionResizeMode(0, QHeaderView::Stretch);

    // Set edit trigger only for SelectionClicked and EditKeyPressed
    setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);

    initTreeView(pDataModel);

    m_cDescendantAddedConnection =
        pDataModel->getWorkspace()->getOrCreateDescendantAddedSignal()->connect(boost::bind(&RobloxTreeWidget::onDescendantAdded, this, _1));
}

RobloxTreeWidget::~RobloxTreeWidget()
{
    deActivate();

    m_InstanceMap.clear();
    m_itemsToUpdate.clear();
    m_selectedItems.clear();
    m_unSelectedItems.clear();
}

void RobloxTreeWidget::addInstance(const Aya::Instance* pInstance, RobloxTreeWidgetItem* pTreeWidgetItem)
{
    m_InstanceMap[pInstance] = pTreeWidgetItem;
}

void RobloxTreeWidget::initTreeView(boost::shared_ptr<Aya::DataModel> pDataModel)
{
    if (!pDataModel->getChildren())
        return;

    Aya::Instances instances;
    instances = *pDataModel->getChildren();

    std::for_each(instances.begin(), instances.end(), boost::bind(&RobloxTreeWidget::createTreeRoot, this, _1));
}

void RobloxTreeWidget::focusInEvent(QFocusEvent* event)
{
    eraseLastSelectedItem();
    QTreeWidget::focusInEvent(event);
    Q_EMIT focusGained();

    UpdateUIManager::Instance().updateToolBars();
}

void RobloxTreeWidget::createTreeRoot(const boost::shared_ptr<Aya::Instance> pInstance)
{
    if (RobloxTreeWidgetItem::isExplorerItem(pInstance))
    {
        int index = getIndexToInsertAt(pInstance);
        // Aya::StandardOut::singleton()->printf(Aya::MESSAGE_INFO,"explorer: instance name: %s", pInstance.get()->getName().c_str());
        RobloxTreeWidgetItem* item = new RobloxTreeWidgetItem((index > 0) ? index : 0, this, pInstance);
    }
}

RobloxTreeWidget* RobloxTreeWidget::getTreeWidget()
{
    return this;
}

Aya::Selection* RobloxTreeWidget::getSelection()
{
    return Aya::ServiceProvider::create<Aya::Selection>(m_pDataModel.get());
}

boost::shared_ptr<Aya::DataModel> RobloxTreeWidget::dataModel()
{
    return m_pDataModel;
}

void RobloxTreeWidget::eraseInstance(Aya::Instance* pInstance)
{
    QMutexLocker lock(treeWidgetMutex());

    m_selectedItems.erase(pInstance);
    m_unSelectedItems.erase(pInstance);

    InstanceMap::iterator iter = m_InstanceMap.find(pInstance);
    if (iter != m_InstanceMap.end())
    {
        if (m_lastSelectedItem == iter->second)
            eraseLastSelectedItem();

        removeFromUpdateList(iter->second);
        m_InstanceMap.erase(iter);
    }
}

RobloxTreeWidgetItem* RobloxTreeWidget::findItemFromInstance(const Aya::Instance* pInstance)
{
    InstanceMap::iterator iter = m_InstanceMap.find(pInstance);
    if (iter != m_InstanceMap.end())
        return iter->second;
    return NULL;
}

void RobloxTreeWidget::scrollToInstance(Aya::Instance* pInstance)
{
    if (!pInstance)
        return;

    if (isValidSelection(pInstance))
    {
        RobloxTreeWidgetItem* item = findItemFromInstance(pInstance);
        if (item)
            scrollToItem(item);
        else if (RobloxTreeWidgetItem* parentItem = findItemFromInstance(pInstance->getParent()))
            parentItem->setExpanded(true);
    }
}

int RobloxTreeWidget::getIndexToInsertAt(shared_ptr<Aya::Instance> pInstance)
{
    int numChild = topLevelItemCount();
    if (!numChild)
        return -1;

    int itemTypeToAdd = RobloxTreeWidgetItem::getItemType(pInstance), currentChild = 0;
    RobloxTreeWidgetItem* pCurrentItem = NULL;

    while (currentChild < numChild)
    {
        pCurrentItem = static_cast<RobloxTreeWidgetItem*>(topLevelItem(currentChild));
        if (itemTypeToAdd < pCurrentItem->itemType())
            break;

        if (itemTypeToAdd == pCurrentItem->itemType())
        {
            if (pInstance->getName() < pCurrentItem->getInstance()->getName())
                break;
        }

        ++currentChild;
    }

    return currentChild;
}

void RobloxTreeWidget::activate()
{
    if (m_isActive)
        return;
    m_isActive = true;

    connect((QTreeWidget*)this, SIGNAL(itemSelectionChanged()), this, SLOT(onItemSelectionChanged()));
    connect((QTreeWidget*)this, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(onItemExpanded(QTreeWidgetItem*)));
    connect((QTreeWidget*)this, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this, SLOT(onItemCollapsed(QTreeWidgetItem*)));
    connect((QTreeWidget*)this, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(onItemDoubleClicked(QTreeWidgetItem*, int)));

    if (m_pDataModel)
    {
        Aya::DataModel::LegacyLock lock(m_pDataModel, Aya::DataModelJob::Write);
        Aya::Selection* pSelection = m_pDataModel->create<Aya::Selection>();
        if (pSelection)
            m_cInstanceSelectionChanged = pSelection->selectionChanged.connect(boost::bind(&RobloxTreeWidget::onInstanceSelectionChanged, this, _1));
    }
}

void RobloxTreeWidget::deActivate()
{
    if (!m_isActive)
        m_isActive = false;

    disconnect((QTreeWidget*)this, SIGNAL(itemSelectionChanged()), this, SLOT(onItemSelectionChanged()));
    m_cInstanceSelectionChanged.disconnect();

    disconnect((QTreeWidget*)this, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(onItemExpanded(QTreeWidgetItem*)));
    disconnect((QTreeWidget*)this, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this, SLOT(onItemCollapsed(QTreeWidgetItem*)));
    disconnect((QTreeWidget*)this, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(onItemDoubleClicked(QTreeWidgetItem*, int)));
}

bool RobloxTreeWidget::onChildAdded(shared_ptr<Aya::Instance> pChild)
{
    if (FFlag::StudioTreeWidgetCheckDeletingFlagWhenDoingUpdates && m_bDeletionRequested)
        return false;

    if (!InstanceUpdateHandler::onChildAdded(pChild))
        return false;
    requestUpdate();
    return true;
}

bool RobloxTreeWidget::onChildRemoved(shared_ptr<Aya::Instance> pChild)
{
    if (FFlag::StudioTreeWidgetCheckDeletingFlagWhenDoingUpdates && m_bDeletionRequested)
        return false;

    if (!InstanceUpdateHandler::onChildRemoved(pChild))
        return false;
    requestUpdate();
    return true;
}

void RobloxTreeWidget::onDescendantAdded(shared_ptr<Aya::Instance> pDescendant)
{
    if (!isFilterEmpty())
    {
        if (filterInstance(pDescendant) && filterAncestors(pDescendant))
            requestUpdate();
    }
}

void RobloxTreeWidget::contextMenuEvent(QContextMenuEvent* evt)
{
    QMenu menu;

    QList<QAction*> commonActions;
    UpdateUIManager::Instance().commonContextMenuActions(commonActions);

    QAction* pAction = NULL;
    for (int ii = 0; ii < commonActions.size(); ++ii)
    {
        pAction = commonActions.at(ii);
        if (pAction)
        {
            // hack to add sub menu for inserting basic objects
            if (pAction == UpdateUIManager::Instance().getMainWindow().insertIntoFileAction)
            {
                Aya::Instance* parent = NULL;
                if (currentItem())
                {
                    RobloxTreeWidgetItem* item = static_cast<RobloxTreeWidgetItem*>(currentItem());
                    parent = item->getInstance().get();
                }

                if (!parent)
                    parent = m_pDataModel->getWorkspace();

                menu.addAction(QIcon(QtUtilities::getPixmap(QString::fromStdString(GetAssetFolder()) + "/textures/ClassImages.PNG", 1)), // part icon
                    "Insert Part", this, SLOT(onInsertPart()));
                menu.addMenu(InsertObjectWidget::createMenu(parent, this, SLOT(onInsertObject())));
            }
            menu.addAction(pAction);
        }
        else
            menu.addSeparator();
    }

    // TODO: Add other tree view actions
    connect(&menu, SIGNAL(aboutToShow()), &UpdateUIManager::Instance(), SLOT(onMenuShow()));
    connect(&menu, SIGNAL(aboutToHide()), &UpdateUIManager::Instance(), SLOT(onMenuHide()));

    menu.exec(evt->globalPos());
}

/**
 * Callback for user clicking on insert part in context menu menu.
 */
void RobloxTreeWidget::onInsertPart()
{
    InsertObjectWidget::InsertObject("Part", m_pDataModel, InsertObjectWidget::InsertMode_TreeWidget);
}

/**
 * Callback for user clicking on insert basic object in context menu sub menu.
 */
void RobloxTreeWidget::onInsertObject()
{
    QAction* action = static_cast<QAction*>(sender());
    QString className = action->text();
    InsertObjectWidget::InsertObject(className, m_pDataModel, InsertObjectWidget::InsertMode_TreeWidget);
}

void RobloxTreeWidget::keyPressEvent(QKeyEvent* evt)
{
    if ((state() != EditingState) && ((evt->key() == Qt::Key_Enter) || (evt->key() == Qt::Key_Return)))
    {
        RobloxTreeWidgetItem* pTreeWidgetItem = dynamic_cast<RobloxTreeWidgetItem*>(currentItem());
        // handle appropriate document open
        if (pTreeWidgetItem && pTreeWidgetItem->handleOpen())
        {
            // accept event
            evt->accept();
            return;
        }
    }

    if (QKeySequence(evt->key() | evt->modifiers()) == UpdateUIManager::Instance().getMainWindow().zoomExtentsAction->shortcut())
    {
        UpdateUIManager::Instance().getMainWindow().zoomExtentsAction->activate(QAction::Trigger);
        evt->accept();
        return;
    }

    QTreeWidget::keyPressEvent(evt);
}

void RobloxTreeWidget::processChildrenRemoval()
{
    TreeWidgetItemList::const_iterator iter = m_itemsToRemove.begin();
    while (iter != m_itemsToRemove.end())
    {
        if (RobloxTreeWidgetItem* pTreeWidgetItem = *iter)
        {
            pTreeWidgetItem->aboutToDelete(this);
            pTreeWidgetItem->deleteLater();
        }
        else
        {
            m_itemsToRemove.erase(iter);
        }

        iter = m_itemsToRemove.begin();
    }
}

void RobloxTreeWidget::requestItemDelete(RobloxTreeWidgetItem* item)
{
    for (int i = 0; i < item->childCount(); ++i)
        if (RobloxTreeWidgetItem* childItem = dynamic_cast<RobloxTreeWidgetItem*>((item)->child(i)))
            requestItemDelete(childItem);

    m_itemsToRemove.insert(item);
    removeFromUpdateList(item);
}

void RobloxTreeWidget::onInstanceSelectionChanged(const Aya::SelectionChanged& evt)
{
    if (evt.addedItem != NULL)
        if (Aya::Instance* pInstance = evt.addedItem.get())
            if (Aya::GuiObject* guiObject = dynamic_cast<Aya::GuiObject*>(pInstance))
                guiObject->setSelectionBox(true);

    if (evt.removedItem != NULL)
        if (Aya::Instance* pInstance = evt.removedItem.get())
            if (Aya::GuiObject* guiObject = dynamic_cast<Aya::GuiObject*>(pInstance))
                guiObject->setSelectionBox(false);


    if (m_bIgnoreInstanceSelectionChanged)
        return;

    Aya::ScopedAssign<bool> ignoreSelectionEvent(m_bIgnoreItemSelectionChanged, true);

    QMutexLocker lock(treeWidgetMutex());
    bool isModificationRequired = false;

    if (evt.addedItem != NULL)
    {
        Aya::Instance* pInstance = evt.addedItem.get();
        if (isValidSelection(pInstance))
        {
            m_unSelectedItems.erase(pInstance);
            m_selectedItems.insert(pInstance);

            isModificationRequired = true;
        }
    }

    if (evt.removedItem != NULL)
    {
        Aya::Instance* pInstance = evt.removedItem.get();

        m_selectedItems.erase(pInstance);
        m_unSelectedItems.insert(pInstance);
        isModificationRequired = true;
    }

    if (isModificationRequired)
        requestUpdate();
}

bool RobloxTreeWidget::isSearchTimeUp(const Aya::Time& startTime, int duration)
{
    return (Aya::Time::now<Aya::Time::Fast>() - startTime).msec() > duration;
}

void RobloxTreeWidget::addFilterItemsToUpdateList()
{
    for (std::set<shared_ptr<Aya::Instance>>::const_iterator iter = m_ancestorsOfFoundItems.begin(); iter != m_ancestorsOfFoundItems.end(); ++iter)
        if (RobloxTreeWidgetItem* item = findItemFromInstance(iter->get()))
            addToUpdateList(item);

    for (std::set<shared_ptr<Aya::Instance>>::const_iterator iter = m_foundItems.begin(); iter != m_foundItems.end(); ++iter)
        if (RobloxTreeWidgetItem* item = findItemFromInstance(iter->get()))
            addToUpdateList(item);
}

void RobloxTreeWidget::onFilterWidgetUpdate()
{
    if (isHidden())
        return;

    Q_EMIT startedProcessing();

    FilterDeque_SPTR previousFilterStack = FilterDeque_SPTR(m_currentFilterStack.begin(), m_currentFilterStack.end());
    m_currentFilterStack.clear();

    {
        Aya::DataModel::LegacyLock lock(m_pDataModel, Aya::DataModelJob::Read);
        filterWidgetRecursive(dataModel(), previousFilterStack, Aya::Time::now<Aya::Time::Fast>());
    }

    if (!m_currentFilterStack.empty())
    {
        QTimer::singleShot(FInt::StudioTreeWidgetEventProcessingTime, this, SLOT(onFilterWidgetUpdate()));
    }
    else
    {
        m_filterRunning = false;
        addFilterItemsToUpdateList();
        requestUpdate();
        Q_EMIT filterSearchFinished();
    }
}

bool RobloxTreeWidget::filterAncestors(shared_ptr<Aya::Instance> instance)
{
    if (!instance->getParent())
        return true;

    if (filterAncestors(shared_from(instance->getParent())))
    {
        if (!RobloxTreeWidgetItem::isExplorerItem(instance))
            return false;

        if (filterInstance(instance))
            m_foundItems.insert(instance);
        else
            m_ancestorsOfFoundItems.insert(instance);

        if (RobloxTreeWidgetItem* item = findItemFromInstance(instance.get()))
            addToUpdateList(item);

        return true;
    }
    return false;
}

void RobloxTreeWidget::filterWidget(const QString& searchString)
{
    setWidgetFilter(searchString);
    filterWidget();
}

void RobloxTreeWidget::filterWidget()
{
    m_currentFilterStack.clear();
    m_foundItems.clear();
    m_ancestorsOfFoundItems.clear();
    m_itemsToUpdate.clear();

    for (int i = 0; i < invisibleRootItem()->childCount(); ++i)
    {
        invisibleRootItem()->child(i)->setDisabled(true);
        invisibleRootItem()->child(i)->setHidden(true);
    }

    if (!m_filterRunning)
    {
        m_filterRunning = true;
        QTimer::singleShot(0, this, SLOT(onFilterWidgetUpdate()));
    }
}

bool RobloxTreeWidget::filterInstance(shared_ptr<Aya::Instance> instance)
{
    for (ParamList::const_iterator iter = m_searchParamList.begin(); iter != m_searchParamList.end(); ++iter)
    {
        switch (iter->second)
        {
        case ParamAny:
            if (!boost::icontains(instance->getName(), iter->first) && !boost::icontains(instance->getClassNameStr(), iter->first))
                return false;
            break;
        case ParamName:
            if (!boost::icontains(instance->getName(), iter->first))
                return false;
            break;
        case ParamType:
            if (!boost::icontains(instance->getClassNameStr(), iter->first))
                return false;
            break;
        }
    }
    return true;
}

bool RobloxTreeWidget::filterWidgetRecursive(shared_ptr<Aya::Instance> instance, FilterDeque_SPTR& previousFilterStack, const Aya::Time& startTime)
{
    if (!RobloxTreeWidgetItem::isExplorerItem(instance))
        return false;

    bool parentOfShownItem = false;

    if (instance->getChildren())
    {
        shared_ptr<Aya::Instance> startingChild;
        bool stackFrontFound = true;

        // using previousFilterStack to start from last search location
        if (!previousFilterStack.empty())
        {
            stackFrontFound = false;

            startingChild = previousFilterStack.front();
            previousFilterStack.pop_front();

            // if stack is broken, we continue through all children
            if (startingChild->getParent() != instance.get())
            {
                previousFilterStack.clear();
                stackFrontFound = true;
            }
        }

        for (Aya::Instances::const_iterator iter = instance->getChildren()->begin(); iter != instance->getChildren()->end(); ++iter)
        {
            if (!stackFrontFound && *iter == startingChild)
                stackFrontFound = true;

            m_currentFilterStack.push_back(*iter);

            if (stackFrontFound && filterWidgetRecursive(*iter, previousFilterStack, startTime))
                parentOfShownItem = true;

            if (isSearchTimeUp(startTime, FInt::StudioTreeWidgetFilterTime))
                break;

            m_currentFilterStack.pop_back();
        }
    }
    else
    {
        previousFilterStack.clear();
    }

    // This is a check to see if filter is empty or instance is the datamodel
    if (isFilterEmpty() || !instance->getParent())
    {
        // Restore item to it's original state
        if (RobloxTreeWidgetItem* item = findItemFromInstance(instance.get()))
            item->setViewState();
        return false;
    }

    if (filterInstance(instance))
    {
        m_foundItems.insert(instance);
        return true;
    }
    else if (parentOfShownItem)
    {
        m_ancestorsOfFoundItems.insert(instance);
        return true;
    }
    else
    {
        // This is hiding the item if there is a filter
        if (RobloxTreeWidgetItem* item = findItemFromInstance(instance.get()))
            item->setViewState();
    }

    return false;
}

void RobloxTreeWidget::setWidgetFilter(const QString& filterString)
{
    m_currentFilter = filterString;

    QStringList filterList = filterString.split(" ", Qt::SkipEmptyParts);

    bool nextIsName = false;
    bool nextIsType = false;
    m_searchParamList.clear();

    for (QStringList::const_iterator iter = filterList.begin(); iter != filterList.end(); ++iter)
    {
        if (nextIsName)
        {
            nextIsName = false;
            m_searchParamList.push_back(StringParamPair(iter->toStdString(), ParamName));
        }
        else if (nextIsType)
        {
            nextIsType = false;
            m_searchParamList.push_back(StringParamPair(iter->toStdString(), ParamType));
        }
        else if (iter->startsWith(nameSearchString.c_str(), Qt::CaseInsensitive))
        {
            QString rightSide = iter->right(iter->size() - nameSearchString.size());

            if (rightSide.isEmpty())
                nextIsName = true;
            else
                m_searchParamList.push_back(StringParamPair(rightSide.toStdString(), ParamName));
        }
        else if (iter->startsWith(classSearchString.c_str(), Qt::CaseInsensitive))
        {
            QString rightSide = iter->right(iter->size() - classSearchString.size());

            if (rightSide.isEmpty())
                nextIsType = true;
            else
                m_searchParamList.push_back(StringParamPair(rightSide.toStdString(), ParamType));
        }
        else
        {
            m_searchParamList.push_back(StringParamPair(iter->toStdString(), ParamAny));
        }
    }
}

bool RobloxTreeWidget::isFoundItem(shared_ptr<Aya::Instance> instance)
{
    return m_foundItems.count(instance) > 0;
}

bool RobloxTreeWidget::isAncestorOfFoundItem(shared_ptr<Aya::Instance> instance)
{
    return m_ancestorsOfFoundItems.count(instance) > 0;
}

bool RobloxTreeWidget::isValidSelection(Aya::Instance* pInstance)
{
    if (!pInstance)
        return false;

    if (!pInstance->getParent())
        return true;

    if (isValidSelection(pInstance->getParent()))
    {
        if (!RobloxTreeWidgetItem::isExplorerItem(shared_from(pInstance)))
            return false;

        return true;
    }

    return false;
}

void RobloxTreeWidget::onItemSelectionChanged()
{
    if (m_bIgnoreItemSelectionChanged)
        return;

    Aya::Selection* pSelection(getSelection());
    if (!pSelection)
        return;

    if (m_pInstanceSelectionHandler)
    {
        boost::shared_ptr<Aya::Instance> selectedInstance;
        QList<QTreeWidgetItem*> selectedItems = this->selectedItems();
        if (selectedItems.size() > 0)
        {
            RobloxTreeWidgetItem* pTreeWidgetItem = dynamic_cast<RobloxTreeWidgetItem*>(selectedItems.at(0));
            if (pTreeWidgetItem)
                selectedInstance = pTreeWidgetItem->getInstance();
        }
        m_pInstanceSelectionHandler->onInstanceSelected(selectedInstance);
        return;
    }

    Aya::ScopedAssign<bool> ignoreSelectionEvent(m_bIgnoreInstanceSelectionChanged, true);
    Aya::DataModel::LegacyLock lock(m_pDataModel, Aya::DataModelJob::Write);

    QList<QTreeWidgetItem*> selectedItems = this->selectedItems();

    shared_ptr<Aya::Instances> instances(new Aya::Instances());
    for (QList<QTreeWidgetItem*>::const_iterator iter = selectedItems.begin(); iter != selectedItems.end(); ++iter)
    {
        if (RobloxTreeWidgetItem* pTreeWidgetItem = dynamic_cast<RobloxTreeWidgetItem*>(*iter))
        {
            if (shared_ptr<Aya::Instance> instance = pTreeWidgetItem->getInstance())
                instances->push_back(instance);
        }
    }

    pSelection->setSelection(instances);

    // HACK: to get toolbars updated for object selection
    UpdateUIManager::Instance().updateToolBars();
}

void RobloxTreeWidget::modifyItemsSelectionState(InstanceList_PTR& instancesList, bool select)
{
    QList<QModelIndex> modelIndexes;

    for (InstanceList_PTR::const_iterator iter = instancesList.begin(); iter != instancesList.end();)
    {
        if (RobloxTreeWidgetItem* pTreeWidgetItem = findItemFromInstance(*iter))
        {
            modelIndexes.push_back(indexFromItem(pTreeWidgetItem));

            if (select)
            {
                m_lastSelectedItem = pTreeWidgetItem;
                instancesList.erase(iter++);
            }
            else
            {
                ++iter;
            }
        }
        else
        {
            ++iter;
        }
    }

    std::stable_sort(modelIndexes.begin(), modelIndexes.end());
    selectionModel()->select(mergeModelIndexes(modelIndexes), select ? QItemSelectionModel::Select : QItemSelectionModel::Deselect);
}

void RobloxTreeWidget::onItemExpanded(QTreeWidgetItem* pItem)
{
    if (!pItem)
        return;

    RobloxTreeWidgetItem* robloxTreeWidgetItem = static_cast<RobloxTreeWidgetItem*>(pItem);
    robloxTreeWidgetItem->requestItemExpand();
}

void RobloxTreeWidget::onItemCollapsed(QTreeWidgetItem* pItem)
{
    if (!pItem)
        return;

    if (isFilterEmpty())
        pItem->setData(1, ExpandRole, QVariant(false));
}

void RobloxTreeWidget::onItemDoubleClicked(QTreeWidgetItem* pWidgetItem, int)
{
    if (!pWidgetItem)
        return;

    static_cast<RobloxTreeWidgetItem*>(pWidgetItem)->handleOpen();
}

void RobloxTreeWidget::addToUpdateList(RobloxTreeWidgetItem* pTreeWidgetItem)
{
    if (!pTreeWidgetItem)
        return;

    QMutexLocker lock(treeWidgetMutex());

    if (pTreeWidgetItem->isItemPendingDeletion())
        return;
    pTreeWidgetItem->setViewState();
    if (existsInRemovalList(pTreeWidgetItem) || !pTreeWidgetItem->isInTreeWidget())
        return;

    std::pair<TreeWidgetItemList::iterator, bool> retIter = m_itemsToUpdate.insert(pTreeWidgetItem);
    if (retIter.second)
        requestUpdate();
}

void RobloxTreeWidget::removeFromUpdateList(RobloxTreeWidgetItem* pTreeWidgetItem)
{
    QMutexLocker lock(treeWidgetMutex());
    m_itemsToUpdate.erase(pTreeWidgetItem);
}

void RobloxTreeWidget::requestUpdate()
{
    if (m_bUpdateRequested)
        return;
    // qt will take ownership of the event so no need to delete
    QApplication::postEvent(this, new RobloxCustomEvent(TREE_WIDGET_UPDATE));

    m_bUpdateRequested = true;
}

void RobloxTreeWidget::requestViewportUpdate()
{
    if (m_bViewportUpdateRequested || m_bUpdateRequested)
        return;
    // qt will take ownership of the event so no need to delete
    QApplication::postEvent(this, new RobloxCustomEvent(TREE_WIDGET_VIEWPORT_UPDATE));
    m_bViewportUpdateRequested = true;
}

void RobloxTreeWidget::updateSelectionState(Aya::Instance* pInstance)
{
    Aya::Selection* pSelection(getSelection());
    if (pSelection && pSelection->isSelected(pInstance))
    {
        QMutexLocker lock(treeWidgetMutex());
        m_unSelectedItems.erase(pInstance);
        m_selectedItems.insert(pInstance);
    }
}

bool RobloxTreeWidget::event(QEvent* evt)
{
    bool retVal = false;
    if (evt->type() == TREE_WIDGET_UPDATE)
    {
        onTreeWidgetUpdate();
        m_bUpdateRequested = false;
        retVal = true;
    }
    else if (evt->type() == TREE_WIDGET_FILTER_UPDATE)
    {
        onFilterWidgetUpdate();
    }
    else if (evt->type() == TREE_SCROLL_TO_INSTANCE)
    {
        RobloxCustomEventWithArg* pCustomEvent = dynamic_cast<RobloxCustomEventWithArg*>(evt);
        if (pCustomEvent)
        {
            boost::function<void()>* pFunctionObj = pCustomEvent->m_pEventArg;
            if (pFunctionObj)
                (*pFunctionObj)();
        }
        retVal = true;
    }
    else if (evt->type() == TREE_WIDGET_VIEWPORT_UPDATE)
    {
        m_bViewportUpdateRequested = false;
        viewport()->update();

        retVal = true;
    }
    else if (evt->type() == QEvent::ShortcutOverride)
    {
        retVal = QTreeWidget::event(evt);
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(evt);
        int key = keyEvent->key();
        QChar qkey = QChar((uint16_t)key);

        // capture single key alpha numerical presses - we want these to be considered
        //  as shortcuts in the tree to jumped to items starting with that key, not global shortcuts
        if (!(keyEvent->key() >= Qt::Key_F1 && keyEvent->key() <= Qt::Key_F35) && qkey.isLetterOrNumber() && keyEvent->modifiers() == Qt::NoModifier)
        {
            keyEvent->accept();
            retVal = true;
        }
    }
    else
        retVal = QTreeWidget::event(evt);

    return retVal;
}

void RobloxTreeWidget::onTreeWidgetUpdate()
{
    if (isHidden() || m_filterRunning)
        return;

    QCoreApplication::removePostedEvents(this, TREE_WIDGET_VIEWPORT_UPDATE);

    // prevents screen flicker
    setUpdatesEnabled(false);
    Aya::ScopedAssign<bool> ignoreSelectionEvent(m_bIgnoreItemSelectionChanged, true);

    QMutexLocker treeLock(treeWidgetMutex());

    if (!m_itemsToRemove.empty())
        processChildrenRemoval();

    int selectionSize = m_selectedItems.size();
    // modify selection first, items not removed from list and items that have yet been populated
    {
        blockSignals(true);
        QMutexLocker lock(treeWidgetMutex());

        if (!m_unSelectedItems.empty())
        {
            modifyItemsSelectionState(m_unSelectedItems, false);
            m_unSelectedItems.clear();
        }

        if (!m_selectedItems.empty())
            modifyItemsSelectionState(m_selectedItems, true);

        selectionSize = m_selectedItems.size();
        blockSignals(false);
    }

    bool itemsAdded = false;

    InstanceList_PTR itemsToSelect;

    Aya::Time startTime = Aya::Time::now<Aya::Time::Fast>();

    std::vector<RobloxTreeWidgetItem*> recentlyAddedItems;

    // Processing Selection
    while (!m_selectedItems.empty() && isFilterEmpty() && !isSearchTimeUp(startTime, FInt::StudioTreeWidgetProcessingTime))
    {
        Aya::Instance* pInstance = *m_selectedItems.begin();

        Aya::Instance* pChild = pInstance;
        while (pChild->getParent() && !findItemFromInstance(pChild->getParent()))
            pChild = pChild->getParent();

        RobloxTreeWidgetItem* newChildNode = NULL;

        if (RobloxTreeWidgetItem* pParentItem = findItemFromInstance(pChild->getParent()))
        {
            pParentItem->requestItemExpand();
            newChildNode = pParentItem->processChildAdd(shared_from(pChild));
        }
        else
        {
            m_selectedItems.erase(pInstance);
        }

        if (pChild == pInstance && newChildNode)
        {
            recentlyAddedItems.push_back(newChildNode);
            m_lastSelectedItem = newChildNode;
            m_selectedItems.erase(pInstance);
        }
    }

    // Processing items requesting update
    while (!m_itemsToUpdate.empty() && !isSearchTimeUp(startTime, FInt::StudioTreeWidgetProcessingTime))
    {
        RobloxTreeWidgetItem* itemNode = *m_itemsToUpdate.begin();

        if (itemNode)
        {
            itemNode->updateFilterItems();
            if (itemNode->processChildrenAdd())
                itemsAdded = true;

            if (!itemNode->hasChildrenToProcess())
            {
                removeFromUpdateList(itemNode);
                itemNode->setDirty(false);
            }
        }
    }

    // recently added items
    if (!recentlyAddedItems.empty())
    {
        QList<QModelIndex> modelIndexes;

        for (std::vector<RobloxTreeWidgetItem*>::const_iterator iter = recentlyAddedItems.begin(); iter != recentlyAddedItems.end(); ++iter)
            modelIndexes.push_back(indexFromItem(*iter));

        std::stable_sort(modelIndexes.begin(), modelIndexes.end());
        selectionModel()->select(mergeModelIndexes(modelIndexes), QItemSelectionModel::Select);
    }

    processChildrenAdd();

    setUpdatesEnabled(true);

    m_bUpdateRequested = false;

    // again request for an update
    if (!m_itemsToRemove.empty() || !m_itemsToUpdate.empty() || itemsAdded || (!m_selectedItems.empty() && isFilterEmpty()))
    {
        QTimer::singleShot(0, this, SLOT(requestUpdate()));
    }
    else
    {
        if (m_lastSelectedItem)
        {
            scrollToItem(m_lastSelectedItem);
            eraseLastSelectedItem();
        }

        if (m_savedMarkerItem)
            QMetaObject::invokeMethod(this, "scrollBackToLastMarkedLocation");

        Q_EMIT finishedProcessing();
    }
}

void RobloxTreeWidget::requestDelete()
{
    m_bDeletionRequested = true;

    deActivate();

    m_pInstance.reset();
    m_pDataModel.reset();

    deleteLater();
}

void RobloxTreeWidget::mousePressEvent(QMouseEvent* evt)
{
    QTreeWidget::mousePressEvent(evt);

    RobloxTreeWidgetItem* pTreeWidgetItem = dynamic_cast<RobloxTreeWidgetItem*>(itemAt(evt->pos()));

    // Check if ContextMenu was opened in the middle of dragging.
    if (m_pRubberBand)
    {
        m_pRubberBand->hide();
        m_pRubberBand->deleteLater();
        m_pRubberBand = NULL;
    }
    else
    {
        // although it will be set to NULL in mouseReleaseEvent but just to be sure setting it to NULL again
        m_pRubberBand = NULL;
    }


    // if we cannot drag select then continue with Qt event propagation
    if (!canDragSelect(evt))
    {
        if (FFlag::StudioMimeDataContainsInstancePath)
        {
            QDrag* drag = new QDrag(this);
            QMimeData* mimeData = new QMimeData;
            mimeData->setText(("Game." + pTreeWidgetItem->getInstance()->getFullName()).c_str());
            drag->setMimeData(mimeData);
            drag->exec();
        }

        return;
    }

    // begin drag select
    m_initialOffset = QPoint(horizontalOffset(), verticalOffset());
    m_rubberBandOrigin = evt->pos();

    m_pRubberBand = new QRubberBand(QRubberBand::Rectangle, this);
    m_pRubberBand->setGeometry(QRect(m_rubberBandOrigin, QSize()));
    m_pRubberBand->show();
}

void RobloxTreeWidget::mouseMoveEvent(QMouseEvent* evt)
{
    if (!m_pRubberBand)
    {
        QTreeWidget::mouseMoveEvent(evt);
        if (m_pInstanceSelectionHandler)
        {
            boost::shared_ptr<Aya::Instance> hoveredInstance;
            RobloxTreeWidgetItem* pTreeWidgetItem = dynamic_cast<RobloxTreeWidgetItem*>(itemAt(evt->pos()));
            if (pTreeWidgetItem)
                hoveredInstance = pTreeWidgetItem->getInstance();
            m_pInstanceSelectionHandler->onInstanceHovered(hoveredInstance);
        }

        return;
    }

    //--- drag selection is ON (do the required updates)
    if (verticalScrollBar())
    {
        if (evt->y() <= 1)
            verticalScrollBar()->setValue(verticalScrollBar()->value() + evt->y() - 1);
        else if (evt->y() >= (viewport()->height() - 1))
            verticalScrollBar()->setValue(verticalScrollBar()->value() + evt->y() - viewport()->height() - 1);
    }

    if (horizontalScrollBar())
    {
        if (evt->x() <= 1)
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() + evt->x() - 1);
        else if (evt->y() >= (viewport()->width() - 1))
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() + evt->x() - viewport()->width() - 1);
    }

    QPoint offset = QPoint(horizontalOffset() - m_initialOffset.x(), verticalOffset() - m_initialOffset.y());
    QRect rubberBandRect = QRect(m_rubberBandOrigin - offset, evt->pos()).normalized();

    m_pRubberBand->setGeometry(rubberBandRect);
    setSelection(QRect(rubberBandRect.bottomLeft(), rubberBandRect.topLeft()), QItemSelectionModel::ClearAndSelect);
}

void RobloxTreeWidget::mouseReleaseEvent(QMouseEvent* evt)
{
    if (!m_pRubberBand)
    {
        QTreeWidget::mouseReleaseEvent(evt);
        return;
    }

    // clean up rubberband
    m_pRubberBand->hide();
    m_pRubberBand->deleteLater();
    m_pRubberBand = NULL;
}

bool RobloxTreeWidget::canDragSelect(QMouseEvent* evt)
{
    if (evt->buttons() != Qt::LeftButton || evt->modifiers() != Qt::NoModifier || m_pInstanceSelectionHandler)
        return false;

    QPoint mousePos = evt->pos();
    QTreeWidgetItem* pTreeWidgetItem = itemAt(mousePos);
    if (pTreeWidgetItem)
    {
        // - make sure if the mouse is clicked outside the item label we must be able to drag select
        // - we must not activate drag selection if mouse click is on the left of the label (to ensure the expand keys work fine)
        // - item drag/drop will work when user clicks on the item label
        QRect itemRect = visualItemRect(pTreeWidgetItem);
        if (!itemRect.contains(mousePos) ||
            (itemRect.contains(mousePos) &&
                (mousePos.x() < (itemRect.bottomLeft().x() + 28 + fontMetrics().horizontalAdvance(pTreeWidgetItem->text(0))))))
            return false;
    }

    //-- we can do a drag select --

    // item under mouse is already selected and the only selected item, we don't proceed further
    if (pTreeWidgetItem && pTreeWidgetItem->isSelected() && (selectedItems().size() == 1))
        return true;

    // clear previously selected items
    clearSelection();
    // if we have an item then select it
    if (pTreeWidgetItem)
        pTreeWidgetItem->setSelected(true);

    return true;
}

void RobloxTreeWidget::dragEnterEvent(QDragEnterEvent* evt)
{
    const QMimeData* mime = evt->mimeData();
    QString data = mime->text();
    QTreeWidget::dragEnterEvent(evt);
}

void RobloxTreeWidget::dragMoveEvent(QDragMoveEvent* evt)
{
    // call base class to handle auto scrolling
    QTreeWidget::dragMoveEvent(evt);

    evt->ignore();

    if (RobloxTreeWidgetItem* pTreeWidgetItem = dynamic_cast<RobloxTreeWidgetItem*>(itemAt(evt->pos())))
    {
        if (shared_ptr<Aya::Instance> pInstance = pTreeWidgetItem->getInstance())
        {
            Aya::Selection* selection = getSelection();
            for (Aya::Instances::const_iterator iter = selection->begin(); iter != selection->end(); ++iter)
                if (pInstance == *iter || (*iter)->isAncestorOf2(pInstance) || (*iter)->getIsParentLocked())
                    return;

            evt->acceptProposedAction();
        }
    }
}

static void setChildren(Aya::Instance* pParent, shared_ptr<Aya::Instance> pChild);

void RobloxTreeWidget::dropEvent(QDropEvent* evt)
{
    try
    {
        RobloxTreeWidgetItem* pTreeWidgetItem = dynamic_cast<RobloxTreeWidgetItem*>(itemAt(evt->pos()));
        if (pTreeWidgetItem)
        {
            boost::shared_ptr<Aya::Instance> pInstance = pTreeWidgetItem->getInstance();
            if (pInstance)
            {
                Aya::DataModel::LegacyLock lock(m_pDataModel, Aya::DataModelJob::Write);
                pTreeWidgetItem->requestItemExpand();

                std::for_each(getSelection()->begin(), getSelection()->end(), boost::bind(&setChildren, pInstance.get(), _1));

                // add in history for undo/redo
                Aya::ChangeHistoryService::requestWaypoint("Drop", m_pDataModel.get());
                m_pDataModel->setDirty(true);
            }
        }
    }
    catch (std::exception& e)
    {
        QtUtilities::RBXMessageBox msgBox;
        msgBox.setText(e.what());
        msgBox.exec();
    }

    evt->setDropAction(Qt::IgnoreAction);
    setState(NoState);
}

static void setChildren(Aya::Instance* pParent, shared_ptr<Aya::Instance> pChild)
{
    // self drag case (ideally qt should have already taken care of this)
    if (!pChild || pChild->getParent() == pParent)
        return;

    pChild->setParent(pParent);
}

static int nearestWidgetItemIndex(RobloxTreeWidgetItem* pParentWidgetItem, int first, int last, int itemTypeToAdd)
{
    while (first <= last)
    {
        int mid = (first + last) / 2;
        RobloxTreeWidgetItem* pCurrentItem = static_cast<RobloxTreeWidgetItem*>(pParentWidgetItem->child(mid));
        AYAASSERT(pCurrentItem != NULL);

        if (itemTypeToAdd == pCurrentItem->itemType())
            return mid;
        else if (itemTypeToAdd > pCurrentItem->itemType())
            first = mid + 1;
        else
            last = mid - 1;
    }

    return first;
}

static QItemSelection mergeModelIndexes(const QList<QModelIndex>& indexes)
{
    QItemSelection colSpans;
    // merge columns
    int i = 0;
    while (i < indexes.count())
    {
        QModelIndex tl = indexes.at(i);
        QModelIndex br = tl;
        while (++i < indexes.count())
        {
            QModelIndex next = indexes.at(i);
            if ((next.parent() == br.parent()) && (next.row() == br.row()) && (next.column() == br.column() + 1))
                br = next;
            else
                break;
        }
        colSpans.append(QItemSelectionRange(tl, br));
    }
    // merge rows
    QItemSelection rowSpans;
    i = 0;
    while (i < colSpans.count())
    {
        QModelIndex tl = colSpans.at(i).topLeft();
        QModelIndex br = colSpans.at(i).bottomRight();
        QModelIndex prevTl = tl;
        while (++i < colSpans.count())
        {
            QModelIndex nextTl = colSpans.at(i).topLeft();
            QModelIndex nextBr = colSpans.at(i).bottomRight();

            if (nextTl.parent() != tl.parent())
                break;

            if ((nextTl.column() == prevTl.column()) && (nextBr.column() == br.column()) && (nextTl.row() == prevTl.row() + 1) &&
                (nextBr.row() == br.row() + 1))
            {
                br = nextBr;
                prevTl = nextTl;
            }
            else
            {
                break;
            }
        }
        rowSpans.append(QItemSelectionRange(tl, br));
    }
    return rowSpans;
}

void RobloxTreeWidget::setInstanceSelectionHandler(InstanceSelectionHandler* handler)
{
    setMouseTracking(handler);
    m_pInstanceSelectionHandler = handler;
}

void RobloxTreeWidget::markLocationToSuppressScrolling()
{
    if (m_savedMarkerItem == NULL)
    {
        m_savedMarkerItem = getMarkerItem();
    }
}

void RobloxTreeWidget::scrollBackToLastMarkedLocation()
{
    if (m_savedMarkerItem)
    {
        QTreeWidgetItem* currenMarkerItem = getMarkerItem();
        if (currenMarkerItem != m_savedMarkerItem)
        {
            int savedRow = indexFromItem(m_savedMarkerItem).row();
            int currentRow = indexFromItem(currenMarkerItem).row();

            verticalScrollBar()->setValue(verticalScrollBar()->value() + (savedRow - currentRow) * verticalScrollBar()->singleStep());
        }
        m_savedMarkerItem = NULL;
    }
}

QTreeWidgetItem* RobloxTreeWidget::getMarkerItem()
{
    QList<QTreeWidgetItem*> selection = selectedItems();
    // if we've a single selection and the selected item is not hidden, the use it as benchmark
    if (selection.count() == 1 && !selection.at(0)->isHidden())
        return selection.at(0);
    // or else use the top item as benchmark
    return itemAt(rect().topLeft());
}


// TODO: Merge this with eraseLastSelectedItem
void RobloxTreeWidget::itemDeletionRequested(RobloxTreeWidgetItem* pItem)
{
    if (pItem == m_savedMarkerItem)
        m_savedMarkerItem = NULL;
}

RobloxExplorerWidget::RobloxExplorerWidget(QWidget* pParent)
    : QWidget(pParent)
    , m_robloxTreeWidget(NULL)
{
    QLineEdit* lineEdit = new QLineEdit();
    lineEdit->setPlaceholderText(
        QString("Filter Workspace (%1)").arg(UpdateUIManager::Instance().getMainWindow().explorerFilterAction->shortcut().toString()));

    m_lineEdit = new QComboBox();
    m_lineEdit->setEditable(true);
    m_lineEdit->setLineEdit(lineEdit);
    m_lineEdit->lineEdit()->setClearButtonEnabled(true);
    m_lineEdit->setInsertPolicy(QComboBox::InsertAtTop);
    m_lineEdit->setMaxCount(10);
    m_lineEdit->setAutoFillBackground(true);

    if (DARKMODE)
    {
        QPalette palette = m_lineEdit->palette();
        palette.setColor(QPalette::PlaceholderText, Qt::white);
        palette.setColor(QPalette::Text, Qt::white);
        palette.setColor(QPalette::Base, QColor(46, 46, 46));
        palette.setColor(QPalette::Window, QColor(46, 46, 46));
        palette.setColor(QPalette::Button, QColor(46, 46, 46));
        palette.setColor(QPalette::ButtonText, Qt::white);
        m_lineEdit->setPalette(palette);
    }

    m_lineEdit->hide();

    m_loadingMovie = new QMovie(":/images/loading.gif");
    m_loadingLabel = new QLabel(lineEdit);
    m_loadingLabel->setMovie(m_loadingMovie);

    QHBoxLayout* lineEditLayout = new QHBoxLayout(lineEdit); // Create a new layout for the QLineEdit
    lineEditLayout->setContentsMargins(0, 0, 0, 0);
    lineEditLayout->addWidget(m_loadingLabel);
    lineEditLayout->addStretch();

    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_lineEdit);
    setLayout(mainLayout);

    setFocusProxy(m_lineEdit);
}

void RobloxExplorerWidget::setCurrentWidget(RobloxTreeWidget* treeWidget)
{
    if (m_robloxTreeWidget == treeWidget)
        return;

    QLayout* mainLayout = layout();

    if (m_robloxTreeWidget)
    {
        m_lineEdit->hide();

        onProcessingFinished();

        m_robloxTreeWidget->hide();
        mainLayout->removeWidget(m_robloxTreeWidget);

        disconnect(m_robloxTreeWidget, SIGNAL(finishedProcessing()), this, SLOT(onProcessingFinished()));
        disconnect(m_robloxTreeWidget, SIGNAL(startedProcessing()), this, SLOT(onProcessingStarted()));
        disconnect(m_robloxTreeWidget, SIGNAL(focusGained()), this, SIGNAL(focusGained()));
        disconnect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterTreeWidget(QString)));
    }

    m_robloxTreeWidget = treeWidget;

    if (m_robloxTreeWidget)
    {
        m_robloxTreeWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        mainLayout->addWidget(m_robloxTreeWidget);
        m_robloxTreeWidget->show();
        m_lineEdit->show();

        m_lineEdit->setEditText(m_robloxTreeWidget->currentFilter());

        connect(m_robloxTreeWidget, SIGNAL(finishedProcessing()), this, SLOT(onProcessingFinished()));
        connect(m_robloxTreeWidget, SIGNAL(startedProcessing()), this, SLOT(onProcessingStarted()));
        connect(m_robloxTreeWidget, SIGNAL(focusGained()), this, SIGNAL(focusGained()));
        connect(m_lineEdit, SIGNAL(editTextChanged(QString)), this, SLOT(filterTreeWidget(QString)));

        if (!m_robloxTreeWidget->isFilterEmpty())
        {
            onProcessingStarted();
            m_robloxTreeWidget->filterWidget();
        }
    }
}

void RobloxExplorerWidget::onProcessingStarted()
{
    m_loadingMovie->start();
    m_loadingLabel->show();
}

void RobloxExplorerWidget::onProcessingFinished()
{
    m_loadingLabel->hide();
    m_loadingMovie->stop();
}

void RobloxExplorerWidget::filterTreeWidget(const QString& text)
{
    if (m_robloxTreeWidget)
    {
        m_robloxTreeWidget->filterWidget(text);
    }
}