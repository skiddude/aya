


#include "InsertObjectListWidget.hpp"

// Qt Headers
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QApplication>

#include "DataModel/DataModel.hpp"

#include <algorithm>

// Aya Headers
#include "AuthoringSettings.hpp"
#include "CommonInsertWidget.hpp"
#include "InsertObjectListWidgetItem.hpp"
#include "RobloxIDEDoc.hpp"
#include "RobloxDocManager.hpp"

FASTFLAG(StudioNewWiki)

InsertObjectListWidget::InsertObjectListWidget(QWidget* pParent)
    : QListWidget(pParent)
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // listen for changes to the editor settings
    m_PropertyChangedConnection =
        AuthoringSettings::singleton().propertyChangedSignal.connect(boost::bind(&InsertObjectListWidget::onPropertyChanged, this, _1));
    onPropertyChanged(NULL);

    connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(onItemInsertRequested(QListWidgetItem*)));
    connect(this, SIGNAL(enterKeyPressed(QListWidgetItem*)), this, SLOT(onItemInsertRequested(QListWidgetItem*)));
}

InsertObjectListWidget::~InsertObjectListWidget()
{
    m_PropertyChangedConnection.disconnect();
}

void InsertObjectListWidget::mouseMoveEvent(QMouseEvent* evt)
{
    if (!(evt->buttons() & Qt::LeftButton) || ((evt->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance()))
        return QListWidget::mouseMoveEvent(evt);

    QDrag* pDrag = new QDrag(this);
    pDrag->setMimeData(new QMimeData);

    QPixmap pixMap(1, 1);
    pDrag->setPixmap(pixMap);

#ifdef Q_WS_MAC
    pDrag->exec();
#else
    pDrag->exec(Qt::CopyAction);
#endif

    pDrag->deleteLater();
}

bool InsertObjectListWidget::event(QEvent* e)
{
    QListWidget::event(e);

    if (isVisible() && currentRow() == -1 && count() > 0)
        setCurrentRow(0);

    return true;
}

void InsertObjectListWidget::keyPressEvent(QKeyEvent* evt)
{
    if ((evt->key() == Qt::Key_Enter) || (evt->key() == Qt::Key_Return))
    {
        if (currentItem())
        {
            Q_EMIT enterKeyPressed(currentItem());
            evt->accept();
            return;
        }
    }

    QListWidget::keyPressEvent(evt);
}

int weightSorter(const InsertObjectListWidgetItem* a, const InsertObjectListWidgetItem* b)
{
    if (a->data(Qt::UserRole + 1).toInt() == b->data(Qt::UserRole + 1).toInt())
        return a->text().compare(b->text()) > 0;

    return a->data(Qt::UserRole + 1).toInt() < b->data(Qt::UserRole + 1).toInt();
}

void InsertObjectListWidget::sortItems(const QHash<QString, QVariant>& itemWeights, QString filter, Qt::SortOrder order)
{
    QListWidget::sortItems(order);
    if (filter.isEmpty())
        return;

    QList<InsertObjectListWidgetItem*> objectWeightList;

    for (int i = this->count() - 1; i >= 0; --i)
    {
        InsertObjectListWidgetItem* item = dynamic_cast<InsertObjectListWidgetItem*>(takeItem(i));
        if (item->text().toLower().startsWith(filter.toLower()))
        {
            item->setData(Qt::UserRole + 1, itemWeights.value(item->text()).toInt());
            objectWeightList.append(item);
        }
        else
        {
            insertItem(i, item);
        }
    }

    std::sort(objectWeightList.begin(), objectWeightList.end(), weightSorter);

    for (QList<InsertObjectListWidgetItem*>::iterator iter = objectWeightList.begin(); iter != objectWeightList.end(); ++iter)
        insertItem(0, *iter);
}

void InsertObjectListWidget::InsertObject(QListWidgetItem* item)
{
    onItemInsertRequested(item);
}

void InsertObjectListWidget::onItemInsertRequested(QListWidgetItem* pListWidgetItem)
{
    if (!pListWidgetItem)
        pListWidgetItem = currentItem();

    if (!pListWidgetItem)
        return;

    InsertObjectListWidgetItem* pInsertListWidgetItem = dynamic_cast<InsertObjectListWidgetItem*>(pListWidgetItem);
    if (!pInsertListWidgetItem || !pInsertListWidgetItem->getInstance())
        return;

    try
    {
        shared_ptr<Aya::Instance> pObjectToInsert = pInsertListWidgetItem->getInstance()->clone(Aya::EngineCreator);
        if (!pObjectToInsert)
            throw std::runtime_error(Aya::format("InsertObject: Cannot find the object selected."));

        RobloxIDEDoc* pCurrentDoc = RobloxDocManager::Instance().getPlayDoc();
        if (pCurrentDoc && pCurrentDoc->getDataModel())
            InsertObjectWidget::InsertObject(pObjectToInsert, shared_from(pCurrentDoc->getDataModel()));
        Q_EMIT itemInserted();
    }
    catch (std::bad_alloc&)
    {
        throw;
    }
    catch (std::exception& e)
    {
        Aya::StandardOut::singleton()->print(Aya::MESSAGE_ERROR, e.what());
    }
}

void InsertObjectListWidget::onPropertyChanged(const Aya::Reflection::PropertyDescriptor* pDescriptor)
{
    if (AuthoringSettings::singleton().basicObjectsDisplayMode == AuthoringSettings::Vertical)
        setWrapping(false);
    else
        setWrapping(true);
}
