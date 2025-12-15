

#pragma once

// Roblox Headers
#include "TaskScheduler.hpp"


// Roblox Studio Headers
#include "RobloxReportView.hpp"

class TSJobItem;

class ArbiterItem : public RobloxCategoryItem
{
public:
    ArbiterItem() {}
    TSJobItem* getOrCreateTSJobItem(boost::shared_ptr<const Aya::TaskScheduler::Job> pJob);
};

class TSJobItem : public QTreeWidgetItem
{
public:
    TSJobItem(ArbiterItem* pParentItem, boost::shared_ptr<const Aya::TaskScheduler::Job> pJob);

    boost::shared_ptr<const Aya::TaskScheduler::Job> getJob()
    {
        return m_pJob;
    }
    void updateValues();

private:
    boost::shared_ptr<const Aya::TaskScheduler::Job> m_pJob;
};

class RobloxTaskScheduler : public RobloxReportView
{
    Q_OBJECT
public:
    RobloxTaskScheduler();
    virtual ~RobloxTaskScheduler();

public Q_SLOTS:
    void setVisible(bool visible);

private Q_SLOTS:
    void updateValues();

private:
    typedef std::vector<boost::shared_ptr<const Aya::TaskScheduler::Job>> Jobs;
    typedef std::map<boost::shared_ptr<Aya::TaskScheduler::Arbiter>, ArbiterItem*> ArbiterItemsMap;
    typedef std::set<TSJobItem*> TSJobItemCollection;

    ArbiterItem* getOrCreateArbiterItem(boost::shared_ptr<Aya::TaskScheduler::Arbiter> pArbiter);
    void syncTSJobItems(const TSJobItemCollection& currentTaskSchedulerItems);

    ArbiterItemsMap m_arbiterMap;
    TSJobItemCollection m_TSJobItems;

    QTimer* m_pTimer;
};
