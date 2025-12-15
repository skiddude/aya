

#pragma once

// Qt Headers
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMutex>

// Roblox Headers
#include "signal.hpp"


namespace Aya
{
class Instance;
class DataModel;
namespace Stats
{
class Item;
class StatsService;
} // namespace Stats
} // namespace Aya

class RobloxDiagnosticsView : public QTreeWidget
{
    Q_OBJECT
public:
    RobloxDiagnosticsView(bool createdFromIDEDoc = false);
    virtual ~RobloxDiagnosticsView();

    void setDataModel(boost::shared_ptr<Aya::DataModel> pDataModel);
    boost::shared_ptr<Aya::DataModel> dataModel();

public Q_SLOTS:
    void setVisible(bool visible);

private Q_SLOTS:
    void updateValues();

private:
    void onChildAdded(boost::shared_ptr<Aya::Instance> child);
    void onChildRemoved(boost::shared_ptr<Aya::Instance> child);

    boost::shared_ptr<Aya::DataModel> m_pDataModel;
    boost::shared_ptr<Aya::Stats::StatsService> m_pStats;
    Aya::signals::scoped_connection m_childAddedConnection;
    Aya::signals::scoped_connection m_childRemovedConnection;
    QTimer* m_pTimer;

    bool m_bPreviousProfiling;
    bool m_bIsCreatedFromIDEDoc;
};

class RobloxDiagnosticsViewItem : public QTreeWidgetItem
{
public:
    RobloxDiagnosticsViewItem(boost::shared_ptr<Aya::Stats::Item> pItem);

    RobloxDiagnosticsView* getTreeWidget();
    RobloxDiagnosticsViewItem* getItemParent();

    boost::shared_ptr<Aya::Stats::Item> getItem()
    {
        return m_pItem;
    }

    void updateValues();

private:
    void onChildAdded(boost::shared_ptr<Aya::Instance> child);
    void onChildRemoved(boost::shared_ptr<Aya::Instance> child);

    boost::shared_ptr<Aya::Stats::Item> m_pItem;
    Aya::signals::scoped_connection m_childAddedConnection;
    Aya::signals::scoped_connection m_childRemovedConnection;
};
