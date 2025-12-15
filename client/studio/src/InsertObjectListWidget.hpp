

#pragma once

#include <QListWidget>

#include "signal.hpp"

#include "Reflection/Property.hpp"

class QListWidgetItem;

class InsertObjectListWidget : public QListWidget
{
    Q_OBJECT
public:
    InsertObjectListWidget(QWidget* pParent);
    virtual ~InsertObjectListWidget();
    void InsertObject(QListWidgetItem* item);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void sortItems(const QHash<QString, QVariant>& itemWeights = (QHash<QString, QVariant>()), QString filter = QString(),
        Qt::SortOrder order = Qt::AscendingOrder);

protected:
    virtual bool event(QEvent* e);

Q_SIGNALS:
    void enterKeyPressed(QListWidgetItem* pItem);
    void itemInserted();

public Q_SLOTS:
    void onItemInsertRequested(QListWidgetItem* item = NULL);

private:
    virtual void mouseMoveEvent(QMouseEvent* event);

    void onPropertyChanged(const Aya::Reflection::PropertyDescriptor* pDescriptor);

    QPoint m_dragStartPosition;
    Aya::signals::scoped_connection m_PropertyChangedConnection;
};
