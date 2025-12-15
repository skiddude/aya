

#pragma once

// Qt Headers
#include <QString>
#include <QWidget>

// Roblox Headers
#include "Utility/KeyCode.hpp"

#include "RobloxView.hpp"

class RobloxView;

class QOgreWidget : public QWidget
{
    Q_OBJECT
public:
    QOgreWidget(const QString& name, QWidget* parent = NULL);

    void setRobloxView(RobloxView* rbxView);

    void activate();
    void deActivate();

    bool hasApplicationFocus()
    {
        return m_hasApplicationFocus;
    }

    bool luaTextBoxHasFocus() const
    {
        return m_luaTextBoxHasFocus;
    }
    void setLuaTextBoxHasFocus(bool hasFocus)
    {
        m_luaTextBoxHasFocus = hasFocus;
    }

protected:
    /*override*/ virtual bool eventFilter(QObject* watched, QEvent* evt) override;
    /*override*/ virtual bool event(QEvent* evt) override;
    /*override*/ virtual void closeEvent(QCloseEvent*) override;

    /*override*/ virtual void enterEvent(QEnterEvent*) override;
    /*override*/ virtual void leaveEvent(QEvent*);

    /*override*/ virtual void focusOutEvent(QFocusEvent* focusEvent) override;

    /*override*/ virtual void resizeEvent(QResizeEvent* evt) override;

    /*override*/ virtual void mousePressEvent(QMouseEvent* evt) override;
    /*override*/ virtual void mouseReleaseEvent(QMouseEvent* evt) override;
    /*override*/ virtual void mouseMoveEvent(QMouseEvent* evt) override;

    /*override*/ virtual void keyPressEvent(QKeyEvent* evt) override;
    /*override*/ virtual void keyReleaseEvent(QKeyEvent* evt) override;

    /*override*/ virtual bool focusNextPrevChild(bool next) override;

    /*override*/ virtual void wheelEvent(QWheelEvent* evt) override;

    /*override*/ virtual void paintEvent(QPaintEvent* event) override;

    /*override*/ QPaintEngine* paintEngine() const override
    {
        return NULL;
    }

    // drag-drop related override
    /*override*/ virtual void dragEnterEvent(QDragEnterEvent* evt) override;
    /*override*/ virtual void dragMoveEvent(QDragMoveEvent* evt) override;
    /*override*/ virtual void dropEvent(QDropEvent* evt) override;
    /*override*/ virtual void dragLeaveEvent(QDragLeaveEvent* evt) override;

#ifdef Q_WS_WIN
    virtual bool winEvent(MSG* msg, long* result);
#endif

private:
    typedef QWidget Super;

    bool isValidDrag(QDragEnterEvent* evt);

    void handleKeyEvent(
        QKeyEvent* evt, Aya::InputObject::UserInputType eventType, Aya::InputObject::UserInputState eventState, bool processed = false);

    RobloxView* m_pRobloxView;
    int m_bIgnoreEnterEvent;
    bool m_bIgnoreLeaveEvent;
    bool m_bUpdateInProgress;
    bool m_bMouseCommandInvoked;
    bool m_hasApplicationFocus;
    bool m_bRobloxViewInitialized;
    bool m_luaTextBoxHasFocus;

    QPoint lastMovePoint;
    Aya::ModCode lastMoveModCode;
};
