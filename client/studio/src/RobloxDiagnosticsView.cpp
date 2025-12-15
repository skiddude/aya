


#include "RobloxDiagnosticsView.hpp"

// Qt Headers
#include <QTimer>
#include <QHeaderView>

// Roblox Headers
#include "DataModel/DataModel.hpp"

#include "DataModel/Stats.hpp"

#include "Utility/Profiling.hpp"


static const int UpdateInterval = 1000 / 5;

RobloxDiagnosticsViewItem::RobloxDiagnosticsViewItem(boost::shared_ptr<Aya::Stats::Item> pItem)
    : m_pItem(pItem)
{
    // Aya::DataModel::LegacyLock lock(getTreeWidget()->dataModel(), Aya::DataModelJob::Read);
    setText(0, m_pItem->getName().c_str());
    setText(1, m_pItem->getStringValue().c_str());

    m_pItem->visitChildren(boost::bind(&RobloxDiagnosticsViewItem::onChildAdded, this, _1));
    m_childAddedConnection = m_pItem->getOrCreateChildAddedSignal()->connect(boost::bind(&RobloxDiagnosticsViewItem::onChildAdded, this, _1));
    m_childRemovedConnection = m_pItem->getOrCreateChildRemovedSignal()->connect(boost::bind(&RobloxDiagnosticsViewItem::onChildRemoved, this, _1));
}

RobloxDiagnosticsView* RobloxDiagnosticsViewItem::getTreeWidget()
{
    return dynamic_cast<RobloxDiagnosticsView*>(treeWidget());
}

RobloxDiagnosticsViewItem* RobloxDiagnosticsViewItem::getItemParent()
{
    return dynamic_cast<RobloxDiagnosticsViewItem*>(parent());
}

void RobloxDiagnosticsViewItem::onChildAdded(boost::shared_ptr<Aya::Instance> child)
{
    if (!child)
        return;

    boost::shared_ptr<Aya::Stats::Item> item = Aya::shared_dynamic_cast<Aya::Stats::Item>(child);
    if (!item)
        return;

    RobloxDiagnosticsViewItem* pViewItem = new RobloxDiagnosticsViewItem(item);
    if (pViewItem)
    {
        addChild(pViewItem);
    }
}

void RobloxDiagnosticsViewItem::onChildRemoved(boost::shared_ptr<Aya::Instance> inst)
{
    if (!inst)
        return;

    boost::shared_ptr<Aya::Stats::Item> item = Aya::shared_dynamic_cast<Aya::Stats::Item>(inst);
    if (!item)
        return;

    RobloxDiagnosticsViewItem* pItemToDelete = NULL;
    for (int ii = 0; ii < childCount(); ii++)
    {
        RobloxDiagnosticsViewItem* pViewItem = dynamic_cast<RobloxDiagnosticsViewItem*>(child(ii));
        if (pViewItem->getItem() == item)
        {
            pItemToDelete = dynamic_cast<RobloxDiagnosticsViewItem*>(takeChild(ii));
            break;
        }
    }

    if (pItemToDelete)
    {
        delete pItemToDelete;
        pItemToDelete = NULL;
    }

    if (!isExpanded() && childCount() > 0)
        setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
}

void RobloxDiagnosticsViewItem::updateValues()
{
    setText(1, m_pItem->getStringValue().c_str());

    for (int ii = 0; ii < childCount(); ii++)
    {
        RobloxDiagnosticsViewItem* pViewItem = dynamic_cast<RobloxDiagnosticsViewItem*>(child(ii));
        if (pViewItem)
            pViewItem->updateValues();
    }
}

RobloxDiagnosticsView::RobloxDiagnosticsView(bool createdFromIDEDoc)
    : m_pTimer(new QTimer(this))
    , m_bPreviousProfiling(false)
    , m_bIsCreatedFromIDEDoc(createdFromIDEDoc)
{
    setUniformRowHeights(true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setAlternatingRowColors(true);

    setColumnCount(2);
    QStringList headerLabels;
    headerLabels << "Name" << "Value";
    setHeaderLabels(headerLabels);

    header()->setDefaultSectionSize(125);

    m_pTimer->setInterval(UpdateInterval);
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(updateValues()));
}

RobloxDiagnosticsView::~RobloxDiagnosticsView()
{
    // turn off profiling
    if (!m_bIsCreatedFromIDEDoc)
        Aya::Profiling::setEnabled(false);

    //	setDataModel(shared_ptr<Aya::DataModel>());
}

void RobloxDiagnosticsView::setDataModel(boost::shared_ptr<Aya::DataModel> pDataModel)
{
    if (m_pDataModel == pDataModel)
        return;

    m_pTimer->stop();

    if (m_pDataModel)
    {
        Aya::DataModel::LegacyLock lock(m_pDataModel, Aya::DataModelJob::Write);
        m_childAddedConnection.disconnect();
        m_childRemovedConnection.disconnect();
        clear();
    }

    m_pStats.reset();
    m_pDataModel = pDataModel;

    if (m_pDataModel)
    {
        Aya::DataModel::LegacyLock lock(m_pDataModel, Aya::DataModelJob::Write);
        m_pStats = shared_from(Aya::ServiceProvider::create<Aya::Stats::StatsService>(m_pDataModel.get()));
        m_pTimer->start();

        m_pStats->visitChildren(boost::bind(&RobloxDiagnosticsView::onChildAdded, this, _1));
        m_childAddedConnection = m_pStats->getOrCreateChildAddedSignal()->connect(boost::bind(&RobloxDiagnosticsView::onChildAdded, this, _1));
        m_childRemovedConnection = m_pStats->getOrCreateChildRemovedSignal()->connect(boost::bind(&RobloxDiagnosticsView::onChildRemoved, this, _1));
    }
}

boost::shared_ptr<Aya::DataModel> RobloxDiagnosticsView::dataModel()
{
    return m_pDataModel;
}

void RobloxDiagnosticsView::updateValues()
{
    // if the window is hidden, stop the update timer
    if (!isVisible())
    {
        m_pTimer->stop();
        return;
    }

    for (int ii = 0; ii < topLevelItemCount(); ii++)
    {
        RobloxDiagnosticsViewItem* pViewItem = dynamic_cast<RobloxDiagnosticsViewItem*>(topLevelItem(ii));
        if (pViewItem)
            pViewItem->updateValues();
    }
}

void RobloxDiagnosticsView::setVisible(bool visible)
{
    if (visible)
    {
        m_bPreviousProfiling = Aya::Profiling::isEnabled();
        if (!m_bPreviousProfiling)
            Aya::Profiling::setEnabled(true);

        m_pTimer->start();
    }
    else
    {
        Aya::Profiling::setEnabled(m_bPreviousProfiling);
        m_pTimer->stop();
    }
    QTreeWidget::setVisible(visible);
}

void RobloxDiagnosticsView::onChildAdded(boost::shared_ptr<Aya::Instance> child)
{
    if (!child)
        return;

    boost::shared_ptr<Aya::Stats::Item> item = Aya::shared_dynamic_cast<Aya::Stats::Item>(child);
    if (!item)
        return;

    RobloxDiagnosticsViewItem* pViewItem = new RobloxDiagnosticsViewItem(item);
    if (pViewItem)
    {
        addTopLevelItem(pViewItem);
    }
}

void RobloxDiagnosticsView::onChildRemoved(boost::shared_ptr<Aya::Instance> child)
{
    if (!child)
        return;

    boost::shared_ptr<Aya::Stats::Item> item = Aya::shared_dynamic_cast<Aya::Stats::Item>(child);
    if (!item)
        return;

    RobloxDiagnosticsViewItem* pItemToDelete = NULL;
    for (int ii = 0; ii < topLevelItemCount(); ii++)
    {
        RobloxDiagnosticsViewItem* pViewItem = dynamic_cast<RobloxDiagnosticsViewItem*>(topLevelItem(ii));
        if (pViewItem->getItem() == item)
        {
            pItemToDelete = dynamic_cast<RobloxDiagnosticsViewItem*>(takeTopLevelItem(ii));
            break;
        }
    }

    if (pItemToDelete)
    {
        delete pItemToDelete;
        pItemToDelete = NULL;
    }
}
