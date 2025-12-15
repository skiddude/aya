

#pragma once

#include <QListWidgetItem>

#include "Tree/Instance.hpp"


class InsertObjectListWidgetItem : public QListWidgetItem
{
public:
    InsertObjectListWidgetItem(
        const QString& name, const QString& description, boost::shared_ptr<Aya::Instance> instance, std::string preferredParentName);

    boost::shared_ptr<Aya::Instance> getInstance()
    {
        return m_instance;
    }
    std::string getPreferredParent()
    {
        return m_preferredParent;
    }
    bool checkFilter(QString& filterString, Aya::Instance* pParent);

private:
    boost::shared_ptr<Aya::Instance> m_instance;
    std::string m_preferredParent;
};
