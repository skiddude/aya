

#pragma once

// Qt Headers
#include <QPlainTextEdit>
#include <QMenu>

// Roblox Headers
#include "signal.hpp"

#include "Utility/StandardOut.hpp"

#include "Reflection/Property.hpp"

class QMenu;
class QAction;

// Override the plain text edit so that other features can be added to it.
class RobloxTextOutputWidget : public QPlainTextEdit
{
    Q_OBJECT

public:
    RobloxTextOutputWidget(QWidget* parent);
    virtual ~RobloxTextOutputWidget();

    virtual QSize sizeHint() const;

protected:
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual bool event(QEvent* evt);

    virtual void resizeEvent(QResizeEvent* e);

    bool isScrollOnBottom();

protected Q_SLOTS:

    void appendOutputText(const QString message, Aya::MessageType type);

private:
    void onPropertyChanged(const Aya::Reflection::PropertyDescriptor* pDescriptor);

    Aya::signals::scoped_connection m_PropertyChangedConnection;
    QTextCharFormat m_TextFormats[Aya::MESSAGE_TYPE_MAX];
};
