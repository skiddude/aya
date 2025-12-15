


#include "InsertObjectListWidgetItem.hpp"

// Qt Headers
#include <QIcon>

// Aya Headers
#include "QtUtilities.hpp"
#include "RobloxTreeWidget.hpp"


InsertObjectListWidgetItem::InsertObjectListWidgetItem(
    const QString& name, const QString& description, boost::shared_ptr<Aya::Instance> instance, std::string preferredParentName)
    : m_instance(instance)
    , m_preferredParent(preferredParentName)
{
    setIcon(QIcon(QtUtilities::getPixmap(QString::fromStdString(GetAssetFolder()) + "/textures/ClassImages.PNG", RobloxTreeWidgetItem::getImageIndex(m_instance))));
    setText(name);

    QString desc(description);
    setToolTip(QtUtilities::wrapText(desc.replace(QRegularExpression("<a href.*(\\/a>|\\/>)"), ""), 80));
}

bool InsertObjectListWidgetItem::checkFilter(QString& filterString, Aya::Instance* pParent)
{
    return pParent && m_instance && m_instance->canSetParent(pParent) &&
           (filterString.isEmpty() || text().contains(filterString, Qt::CaseInsensitive));
}