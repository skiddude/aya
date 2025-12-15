


#include "ogrewidget.hpp"
#include "TaskScheduler.hpp"


// Qt Headers
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QApplication>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QUrl>
#include <QTimer>
#include <QMimeData>
#include <QWindow>

// Roblox Headers
#include "FastLog.hpp"
#include "Utility/Http.hpp"


// Roblox Studio Headers
#include "RobloxSettings.hpp"
#include "RobloxCustomWidgets.hpp"
#include "CommonInsertWidget.hpp"
#include "RobloxMainWindow.hpp"
#include "UpdateUIManager.hpp"
#include "InsertObjectListWidgetItem.hpp"
#include "RobloxDocManager.hpp"
#include "RobloxIDEDoc.hpp"
#include "DataModel/InputObject.hpp"
#include "DataModel/Lighting.hpp"
#include "DataModel/GameBasicSettings.hpp"



#include "Utility/PhysicalProperties.hpp"

#include "DataModel/Workspace.hpp"
#include "Render/VisualEngine.hpp"
#include "Render/TextureManager.hpp"
#include "Render/SceneUpdater.hpp"
#include "Render/MaterialGenerator.hpp"
#include "RobloxPluginHost.hpp"
#include "StudioUtilities.hpp"

LOGGROUP(TaskSchedulerTiming)
LOGGROUP(RenderRequest)
DYNAMIC_FASTFLAGVARIABLE(BackTabInputInStudio, false)
FASTFLAGVARIABLE(DontSwallowInputForStudioShortcuts, false)
DYNAMIC_FASTFLAG(MaterialPropertiesEnabled)

static Aya::KeyCode keyCodeTOUIKeyCode(int keyCode);
static Aya::ModCode modifiersToUIModCode(int modifier);

QOgreWidget::QOgreWidget(const QString& name, QWidget* parent)
    : QWidget(NULL)
    , m_pRobloxView(NULL)
    , m_bIgnoreEnterEvent(0)
    , m_bIgnoreLeaveEvent(false)
    , m_bUpdateInProgress(false)
    , m_bMouseCommandInvoked(false)
    , m_hasApplicationFocus(true)
    , m_bRobloxViewInitialized(false)
    , m_luaTextBoxHasFocus(false)
{
    setObjectName(name);

    // set default states

    setAttribute(Qt::WA_PaintOnScreen, true);
    setAttribute(Qt::WA_NativeWindow, true);
    setAttribute(Qt::WA_DontCreateNativeAncestors, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_NoSystemBackground, true);

    setFocusPolicy(Qt::StrongFocus);
    setAutoFillBackground(true);
    setAcceptDrops(true);
    setMinimumSize(100, 100);
    setMaximumSize(3800, 2100);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    setContextMenuPolicy(Qt::PreventContextMenu);
}

void QOgreWidget::setRobloxView(RobloxView* rbxView)
{
    if (RobloxIDEDoc* playDoc = RobloxDocManager::Instance().getPlayDoc())
    {
        playDoc->hideChatBar();
    }

    Aya::GameBasicSettings::singleton().setVirtualVersionInternal(Aya::GameBasicSettings::VERSION_2016);

    m_pRobloxView = rbxView;

    if (!m_pRobloxView)
        return;

    Aya::Lighting* lighting = m_pRobloxView->getViewBase().getVisualEngine()->getLighting();
    if (lighting)
    {
        m_pRobloxView->getViewBase().getVisualEngine()->flushTextures();
        lighting->lightingChangedSignal(true);
    }

    QPoint mousePos = mapFromGlobal(QCursor::pos());
    if (rect().contains(mousePos))
        enterEvent(NULL);
    else
        leaveEvent(NULL);
}

void QOgreWidget::activate()
{
    qApp->installEventFilter(this);

    // reset bounds to force a resize if parent or size changed
    // Roblox view can already be destroyed in resetting case
    if (m_pRobloxView)
    {
        // For rendering, we need physical pixel dimensions on high DPI displays
        qreal dpr = windowHandle() ? windowHandle()->devicePixelRatio() : 1.0;
        unsigned int physicalWidth = static_cast<unsigned int>(width() * dpr);
        unsigned int physicalHeight = static_cast<unsigned int>(height() * dpr);
        m_pRobloxView->setBounds(physicalWidth, physicalHeight);
    }
}

void QOgreWidget::deActivate()
{
    FASTLOG(FLog::RenderRequest, "QOgreWidget deactivated, OGRE_VIEW_UPDATE removed");
    if (Aya::TaskScheduler::singleton().isCyclicExecutive())
    {
        // NOTHING!
    }
    else
    {
        QCoreApplication::removePostedEvents(this, OGRE_VIEW_UPDATE);
    }

    qApp->removeEventFilter(this);
}

bool QOgreWidget::eventFilter(QObject* watched, QEvent* evt)
{
    QEvent::Type eventType = evt->type();

    if (eventType == QEvent::ApplicationActivate)
        m_hasApplicationFocus = true;
    else if (eventType == QEvent::ApplicationDeactivate)
        m_hasApplicationFocus = false;

    // whenever popup widgets are shown, QOgreWidget receives only enter and leave event but no mouse move event
    // so, do not handle enter/leave event in all such cases
    // e.g Color Picker Widget which comes up from the Properties Window
    if ((watched != this) && watched->isWidgetType() && (static_cast<QWidget*>(watched)->windowFlags() & Qt::Popup) == Qt::Popup)
    {
        // In case of Floating Dock Widget, do not show ArrowCursor when the mouse is over Ogre Window
        if (watched->inherits("QDockWidget"))
            return QWidget::eventFilter(watched, evt);

        if (eventType == QEvent::Show)
        {
            m_bIgnoreEnterEvent++;
        }
        else if (eventType == QEvent::Hide)
        {
            --m_bIgnoreEnterEvent;
            if (!m_bIgnoreEnterEvent)
            {
                QPoint mousePos = mapFromGlobal(QCursor::pos());
                if (rect().contains(mousePos))
                    enterEvent(NULL);
            }
        }
        else if (eventType == QEvent::MouseButtonRelease)
        {
            // on close of a popup, mouse events are replayed (button doesn't matter) on the widget under the mouse
            // make sure we disable it on the popup as well as it's parent to avoid any unnecessary event propagation
            QWidget* pWidget = static_cast<QWidget*>(watched);
            if (pWidget->rect().contains(pWidget->mapFromGlobal(QCursor::pos())))
            {
                pWidget->setAttribute(Qt::WA_NoMouseReplay);
                if (pWidget->parent() && pWidget->parent()->isWidgetType())
                    static_cast<QWidget*>(pWidget->parent())->setAttribute(Qt::WA_NoMouseReplay);
            }
        }
    }

    if (watched == this && hasFocus())
    {
        // Swallow slash key for retro chat bar
        if (eventType == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(evt);
            if (keyEvent->key() == Qt::Key_Slash)
            {
                if (RobloxIDEDoc* playDoc = RobloxDocManager::Instance().getPlayDoc())
                {
                    if (playDoc->chatBarShown() && !m_luaTextBoxHasFocus)
                    {
                        playDoc->focusChatBar();
                    }
                }

                evt->accept();
                return true;
            }
        }

        if (FFlag::DontSwallowInputForStudioShortcuts)
        {
            if (evt->type() == QEvent::ShortcutOverride)
            {
                bool processed = false;

                QKeyEvent* keyEvent = static_cast<QKeyEvent*>(evt);
                int keyCode = keyEvent->key() | StudioUtilities::translateKeyModifiers(keyEvent->modifiers(), keyEvent->text());

                if (UpdateUIManager::Instance().getMainWindow().isShortcut(QKeySequence(keyCode)))
                {
                    processed = true;
                }

                // ShorcutOverride is sent for every key press, not just shortcuts. We don't want to fire handleKeyEvent in KeyPressed because then
                // studio shortcuts get swallowed.
                handleKeyEvent((QKeyEvent*)evt, Aya::InputObject::TYPE_KEYBOARD, Aya::InputObject::INPUT_STATE_BEGIN, processed);
            }
            else if (evt->type() == QEvent::KeyRelease)
            {
                handleKeyEvent((QKeyEvent*)evt, Aya::InputObject::TYPE_KEYBOARD, Aya::InputObject::INPUT_STATE_END);
            }
        }
    }

    // This is needed for Modal Dialog Show/Hide on Mac
#ifdef Q_WS_MAC
    if (eventType == QEvent::ActivationChange || eventType == QEvent::FocusIn || eventType == QEvent::FocusOut)
    {
        if (rect().contains(mapFromGlobal(QCursor::pos())))
            isActiveWindow() ? enterEvent(NULL) : leaveEvent(NULL);
    }
    else if (eventType == QEvent::Hide && watched->inherits("QDialog"))
    {
        if (rect().contains(mapFromGlobal(QCursor::pos())) && isActiveWindow())
            m_bIgnoreLeaveEvent = true;
    }
#else
    if (eventType == QEvent::ActivationChange)
    {
        if (rect().contains(mapFromGlobal(QCursor::pos())))
            isActiveWindow() ? enterEvent(NULL) : leaveEvent(NULL);
    }
#endif

    return QWidget::eventFilter(watched, evt);
}

bool QOgreWidget::event(QEvent* evt)
{
    if (m_pRobloxView)
    {
        if (evt->type() == QEvent::FocusOut)
        {
            m_pRobloxView->handleFocus(false);
            setLuaTextBoxHasFocus(false);
        }
        else if (evt->type() == QEvent::FocusIn)
        {
            m_pRobloxView->handleFocus(true);
        }
    }

    // If a lua text box as focus then prevent all shortcut overrides
    // from executing.
    if (m_luaTextBoxHasFocus)
    {
        if (evt->type() == QEvent::ShortcutOverride)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(evt);
            keyEvent->accept();
            return true;
        }
    }

    if (evt->type() == QEvent::KeyPress)
    {
        if (static_cast<QKeyEvent*>(evt)->modifiers().testFlag(Qt::ControlModifier) && (static_cast<QKeyEvent*>(evt)->key() == Qt::Key_Tab))
        {
            evt->accept();
            QCoreApplication::sendEvent(&UpdateUIManager::Instance().getMainWindow(), evt);
            return true;
        }
    }

    FASTLOG(FLog::TaskSchedulerTiming, "QT Event fired through QOgreWidget");
    if ((evt->type() != OGRE_VIEW_UPDATE) || !m_pRobloxView)
    {
        FASTLOG1(FLog::RenderRequest, "OgreWidget Event returns before updateView. RBXView: %d", (size_t)m_pRobloxView);
        return QWidget::event(evt);
    }

    if (!m_bUpdateInProgress)
    {
        m_bUpdateInProgress = true;
        m_pRobloxView->updateView();
        m_bUpdateInProgress = false;
    }

    return true;
}

void QOgreWidget::closeEvent(QCloseEvent*)
{
    if (m_pRobloxView)
    {
        delete m_pRobloxView;
    }

    m_pRobloxView = nullptr;
}

void QOgreWidget::enterEvent(QEnterEvent*)
{
    if (!m_pRobloxView)
        return;

#ifdef Q_WS_MAC
    if (QApplication::activeWindow() != &UpdateUIManager::Instance().getMainWindow())
        return;
#endif

    // inform Aya view about the change
    m_pRobloxView->handleMouseInside(true);

    // enable mouse tracking
    setMouseTracking(true);
}

void QOgreWidget::leaveEvent(QEvent*)
{
#ifdef Q_WS_MAC
    if (m_bIgnoreLeaveEvent)
    {
        m_bIgnoreLeaveEvent = false;
        return;
    }
#endif

    if (!m_pRobloxView)
        return;

    // inform Aya view about the change
    m_pRobloxView->handleMouseInside(false);

    // disable mouse tracking
    setMouseTracking(false);

#ifdef Q_WS_MAC
    setCursor(Qt::ArrowCursor);
#endif
}

void QOgreWidget::focusOutEvent(QFocusEvent* focusEvent)
{
    QWidget::focusOutEvent(focusEvent);

    if (m_pRobloxView)
        m_pRobloxView->resetKeyState();
}

void QOgreWidget::resizeEvent(QResizeEvent* evt)
{
    if (!m_pRobloxView)
        return;

    // For rendering, we need physical pixel dimensions on high DPI displays
    qreal dpr = windowHandle() ? windowHandle()->devicePixelRatio() : 1.0;
    unsigned int physicalWidth = static_cast<unsigned int>(evt->size().width() * dpr);
    unsigned int physicalHeight = static_cast<unsigned int>(evt->size().height() * dpr);

    m_pRobloxView->setBounds(physicalWidth, physicalHeight);

    QPoint mousePos = mapFromGlobal(QCursor::pos());
    if (rect().contains(mousePos))
        enterEvent(NULL);
}

void QOgreWidget::mousePressEvent(QMouseEvent* evt)
{
    if (!m_pRobloxView)
        return;

    QPoint currentPos = evt->pos();

    // Scale to physical pixels to match the rendering coordinate system
    qreal dpr = windowHandle() ? windowHandle()->devicePixelRatio() : 1.0;
    int x = static_cast<int>(currentPos.x() * dpr);
    int y = static_cast<int>(currentPos.y() * dpr);

    Aya::InputObject::UserInputType mouseEventType = Aya::InputObject::TYPE_NONE;
    switch (evt->button())
    {
    case Qt::LeftButton:
        mouseEventType = Aya::InputObject::TYPE_MOUSEBUTTON1;
        break;
    case Qt::RightButton:
        mouseEventType = Aya::InputObject::TYPE_MOUSEBUTTON2;
        break;
    case Qt::MiddleButton:
        mouseEventType = Aya::InputObject::TYPE_MOUSEBUTTON3;
        break;
    default:
        break;
    }

    m_pRobloxView->handleMouse(mouseEventType, Aya::InputObject::INPUT_STATE_BEGIN, x, y, modifiersToUIModCode(evt->modifiers()));
    evt->accept();
}

void QOgreWidget::mouseReleaseEvent(QMouseEvent* evt)
{
    if (!m_pRobloxView)
        return;

    Aya::InputObject::UserInputType mouseEventType = Aya::InputObject::TYPE_NONE;
    switch (evt->button())
    {
    case Qt::LeftButton:
        mouseEventType = Aya::InputObject::TYPE_MOUSEBUTTON1;
        break;
    case Qt::RightButton:
        mouseEventType = Aya::InputObject::TYPE_MOUSEBUTTON2;
        break;
    case Qt::MiddleButton:
        mouseEventType = Aya::InputObject::TYPE_MOUSEBUTTON3;
        break;
    default:
        break;
    }

    evt->accept();

    QPoint currentPos = evt->pos();

    // Scale to physical pixels to match the rendering coordinate system
    qreal dpr = windowHandle() ? windowHandle()->devicePixelRatio() : 1.0;
    int x = static_cast<int>(currentPos.x() * dpr);
    int y = static_cast<int>(currentPos.y() * dpr);

    m_pRobloxView->handleMouse(mouseEventType, Aya::InputObject::INPUT_STATE_END, x, y, modifiersToUIModCode(evt->modifiers()));
}

void QOgreWidget::mouseMoveEvent(QMouseEvent* evt)
{
    FASTLOG(FLog::TaskSchedulerTiming, "QOgreWidget::mouseMoveEvent firing");
    if (m_pRobloxView)
    {
        lastMovePoint = evt->pos();
        lastMoveModCode = modifiersToUIModCode(evt->modifiers());

        // Scale to physical pixels to match the rendering coordinate system
        // Both position and deltas need to be in the same coordinate space as the viewport
        qreal dpr = windowHandle() ? windowHandle()->devicePixelRatio() : 1.0;
        int x = static_cast<int>(lastMovePoint.x() * dpr);
        int y = static_cast<int>(lastMovePoint.y() * dpr);

        m_pRobloxView->handleMouse(Aya::InputObject::TYPE_MOUSEMOVEMENT, Aya::InputObject::INPUT_STATE_CHANGE, x, y, lastMoveModCode);
    }
    evt->accept();
}

void QOgreWidget::handleKeyEvent(
    QKeyEvent* evt, Aya::InputObject::UserInputType eventType, Aya::InputObject::UserInputState eventState, bool processed)
{
    if (eventType == Aya::InputObject::TYPE_KEYBOARD && (!m_pRobloxView || evt->isAutoRepeat()))
    {
        if (!FFlag::DontSwallowInputForStudioShortcuts)
        {
            if (eventState == Aya::InputObject::INPUT_STATE_BEGIN)
                Super::keyPressEvent(evt);
            else if (eventState == Aya::InputObject::INPUT_STATE_END)
                Super::keyReleaseEvent(evt);
        }
        return;
    }

    std::string textString = evt->text().toStdString();

    m_pRobloxView->handleKey(eventType, eventState, keyCodeTOUIKeyCode(evt->key()), modifiersToUIModCode(evt->modifiers()),
        (textString.length() > 0) ? textString.c_str()[0] : NULL, processed);

    if (!FFlag::DontSwallowInputForStudioShortcuts)
    {
        evt->accept();
    }
}

void QOgreWidget::keyPressEvent(QKeyEvent* evt)
{
    if (FFlag::DontSwallowInputForStudioShortcuts)
    {
        if (!m_pRobloxView || evt->isAutoRepeat())
        {
            Super::keyPressEvent(evt);
            return;
        }
        evt->accept();
    }
    else
    {
        handleKeyEvent(evt, Aya::InputObject::TYPE_KEYBOARD, Aya::InputObject::INPUT_STATE_BEGIN);
    }
}

bool QOgreWidget::focusNextPrevChild(bool next)
{
    return false;
}

void QOgreWidget::keyReleaseEvent(QKeyEvent* evt)
{
    if (FFlag::DontSwallowInputForStudioShortcuts)
    {
        if (!m_pRobloxView || evt->isAutoRepeat())
        {
            Super::keyReleaseEvent(evt);
            return;
        }
        evt->accept();
    }
    else
    {
        handleKeyEvent(evt, Aya::InputObject::TYPE_KEYBOARD, Aya::InputObject::INPUT_STATE_END);
    }
}

void QOgreWidget::wheelEvent(QWheelEvent* evt)
{
    if (!m_pRobloxView)
        return;

    QPointF currentPos = evt->position();
    QPoint angleDelta = evt->angleDelta();
    m_pRobloxView->handleScrollWheel(angleDelta.y(), currentPos.x(), currentPos.y());
    evt->accept();
}

void QOgreWidget::paintEvent(QPaintEvent* evt)
{
    if (!m_bRobloxViewInitialized)
    {
        if (RobloxIDEDoc* playDoc = RobloxDocManager::Instance().getPlayDoc())
        {
            m_bRobloxViewInitialized = true;
            playDoc->initializeRobloxView();
        }
    }


    if (!m_pRobloxView)
        return QWidget::paintEvent(evt);

    m_pRobloxView->requestUpdateView();

    evt->accept();
}

void QOgreWidget::dragEnterEvent(QDragEnterEvent* evt)
{
    if (!isValidDrag(evt))
        return QWidget::dragEnterEvent(evt);

    // Scale to physical pixels to match the rendering coordinate system
    qreal dpr = windowHandle() ? windowHandle()->devicePixelRatio() : 1.0;
    int x = static_cast<int>(evt->pos().x() * dpr);
    int y = static_cast<int>(evt->pos().y() * dpr);

    const QMimeData* pMimeData = evt->mimeData();
    if (pMimeData && pMimeData->hasUrls())
    {
        QList<QUrl> urlList = pMimeData->urls();
        shared_ptr<const Aya::Instances> insertedInstances =
            m_pRobloxView->handleDropOperation(urlList.at(0).toString(), x, y, m_bMouseCommandInvoked);

        if (RobloxIDEDoc* playDoc = RobloxDocManager::Instance().getPlayDoc())
            if (Aya::DataModel* dataModel = playDoc->getDataModel())
                UpdateUIManager::Instance().getMainWindow().getPluginHost()->handleDragEnterEvent(
                    dataModel, shared_ptr<const Aya::Instances>(insertedInstances), Aya::Vector2(x, y));
    }
    else
    {
        QListWidget* pListWidget = static_cast<QListWidget*>(evt->source());
        QList<QListWidgetItem*> itemList = pListWidget->selectedItems();
        if (itemList.size() <= 0)
            return QWidget::dragEnterEvent(evt);

        InsertObjectListWidgetItem* pListWidgetItem = dynamic_cast<InsertObjectListWidgetItem*>(itemList.at(0));
        if (pListWidgetItem)
        {
            m_pRobloxView->handleDropOperation(pListWidgetItem->getInstance(), x, y, m_bMouseCommandInvoked);
        }
    }

#ifdef Q_WS_MAC
    if (m_bMouseCommandInvoked)
    {
        QApplication::setOverrideCursor(Qt::ClosedHandCursor);
        enterEvent(NULL);
    }
    else
    {
        QApplication::setOverrideCursor(Qt::DragCopyCursor);
        leaveEvent(NULL);
    }
#endif

    evt->acceptProposedAction();
}

void QOgreWidget::dragMoveEvent(QDragMoveEvent* evt)
{
    if (!m_pRobloxView)
        return QWidget::dragMoveEvent(evt);

    if (m_bMouseCommandInvoked)
    {
        lastMovePoint = evt->pos();

        // Scale to physical pixels to match the rendering coordinate system
        qreal dpr = windowHandle() ? windowHandle()->devicePixelRatio() : 1.0;
        int x = static_cast<int>(lastMovePoint.x() * dpr);
        int y = static_cast<int>(lastMovePoint.y() * dpr);

        m_pRobloxView->handleMouse(Aya::InputObject::TYPE_MOUSEMOVEMENT, Aya::InputObject::INPUT_STATE_CHANGE, x, y, lastMoveModCode);
    }

    evt->acceptProposedAction();
}

void QOgreWidget::dropEvent(QDropEvent* evt)
{
    const QMimeData* pMimeData = evt->mimeData();
    if (!m_pRobloxView || !pMimeData)
        return QWidget::dropEvent(evt);

#ifdef Q_WS_MAC
    QApplication::restoreOverrideCursor();
#endif

    if (m_bMouseCommandInvoked)
    {
        QPoint currentPos = evt->pos();

        // Scale to physical pixels to match the rendering coordinate system
        qreal dpr = windowHandle() ? windowHandle()->devicePixelRatio() : 1.0;
        int x = static_cast<int>(currentPos.x() * dpr);
        int y = static_cast<int>(currentPos.y() * dpr);

        m_pRobloxView->handleMouse(Aya::InputObject::TYPE_MOUSEBUTTON1, Aya::InputObject::INPUT_STATE_END, x, y, (Aya::ModCode)0);
    }

    evt->acceptProposedAction();
    setFocus();
}

void QOgreWidget::dragLeaveEvent(QDragLeaveEvent* evt)
{
    if (!m_pRobloxView)
        return QWidget::dragLeaveEvent(evt);

    m_pRobloxView->cancelDropOperation(m_bMouseCommandInvoked);

#ifdef Q_WS_MAC
    QApplication::restoreOverrideCursor();
    leaveEvent(NULL);
#endif

    evt->accept();
}

#ifdef Q_WS_WIN
bool QOgreWidget::winEvent(MSG* msg, long* result)
{

    return QWidget::winEvent(msg, result);
}
#endif

bool QOgreWidget::isValidDrag(QDragEnterEvent* evt)
{
    bool result = false;

    const QMimeData* pMimeData = evt->mimeData();
    if (m_pRobloxView && pMimeData)
    {
        // All drags from HTML should be called via window.external.Insert in JS
        if (pMimeData->hasFormat("text/html"))
            return false;

        if (pMimeData->hasUrls())
        {
            QList<QUrl> urlList = pMimeData->urls();
            QString urlQStr = urlList.at(0).toString();
            QString fileStr = urlList.at(0).toLocalFile();
            if (!urlQStr.isEmpty() && fileStr.isEmpty())
                result = true;
        }
        else
        {
            QObject* pSourceObject = evt->source();
            QWidget* pSourceWidget = dynamic_cast<QWidget*>(pSourceObject);
            if (pSourceWidget && (pSourceWidget->objectName() == "InsertObjectListWidget"))
                result = true;
        }
    }

    return result;
}

static Aya::KeyCode keyCodeTOUIKeyCode(int keyCode)
{
    /// Handle all albhabets
    if (keyCode >= 65 && keyCode <= 90)
        return Aya::KeyCode(keyCode - Qt::Key_A + 'a');

    // Handle Numbers
    if (keyCode >= Qt::Key_0 && keyCode <= Qt::Key_9)
        return Aya::KeyCode(keyCode);

    // Handle F1 to F15
    if (keyCode >= Qt::Key_F1 && keyCode <= Qt::Key_F15)
        return Aya::KeyCode(keyCode - 0xFFFF16);


    Aya::KeyCode rbxKey = Aya::AYA_SDLK_UNKNOWN;

    // Handle Special Unordered Keys Here
    switch (keyCode)
    {
    case Qt::Key_CapsLock:
        rbxKey = Aya::AYA_SDLK_CAPSLOCK;
        break;
    case Qt::Key_Backspace:
        rbxKey = Aya::AYA_SDLK_BACKSPACE;
        break;
    case Qt::Key_Up:
        rbxKey = Aya::AYA_SDLK_UP;
        break;
    case Qt::Key_Down:
        rbxKey = Aya::AYA_SDLK_DOWN;
        break;
    case Qt::Key_Left:
        rbxKey = Aya::AYA_SDLK_LEFT;
        break;
    case Qt::Key_Right:
        rbxKey = Aya::AYA_SDLK_RIGHT;
        break;
    case Qt::Key_Insert:
        rbxKey = Aya::AYA_SDLK_INSERT;
        break;
    case Qt::Key_Delete:
        rbxKey = Aya::AYA_SDLK_DELETE;
        break;
    case Qt::Key_Home:
        rbxKey = Aya::AYA_SDLK_HOME;
        break;
    case Qt::Key_End:
        rbxKey = Aya::AYA_SDLK_END;
        break;
    case Qt::Key_PageUp:
        rbxKey = Aya::AYA_SDLK_PAGEUP;
        break;
    case Qt::Key_PageDown:
        rbxKey = Aya::AYA_SDLK_PAGEDOWN;
        break;
    case Qt::Key_Space:
        rbxKey = Aya::AYA_SDLK_SPACE;
        break;
    case Qt::Key_Control:
        rbxKey = Aya::AYA_SDLK_LCTRL;
        break;
    case Qt::Key_Alt:
        rbxKey = Aya::AYA_SDLK_LALT;
        break;
    case Qt::Key_Shift:
        rbxKey = Aya::AYA_SDLK_LSHIFT;
        break;
    case Qt::Key_Escape:
        rbxKey = Aya::AYA_SDLK_ESCAPE;
        break;
    case Qt::Key_Minus:
        rbxKey = Aya::AYA_SDLK_MINUS;
        break;
    case Qt::Key_Equal:
        rbxKey = Aya::AYA_SDLK_EQUALS;
        break;
    case Qt::Key_Tab:
        rbxKey = Aya::AYA_SDLK_TAB;
        break;
    case Qt::Key_Backtab:
    {
        if (DFFlag::BackTabInputInStudio)
        {
            rbxKey = Aya::AYA_SDLK_TAB;
            break;
        }
    }
    case Qt::Key_BracketLeft:
        rbxKey = Aya::AYA_SDLK_LEFTBRACKET;
        break;
    case Qt::Key_BracketRight:
        rbxKey = Aya::AYA_SDLK_RIGHTBRACKET;
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        rbxKey = Aya::AYA_SDLK_RETURN;
        break;
    case Qt::Key_Semicolon:
        rbxKey = Aya::AYA_SDLK_SEMICOLON;
        break;
    case Qt::Key_QuoteLeft:
        rbxKey = Aya::AYA_SDLK_BACKQUOTE;
        break;
    case Qt::Key_Apostrophe:
        rbxKey = Aya::AYA_SDLK_QUOTE;
        break;
    case Qt::Key_QuoteDbl:
        rbxKey = Aya::AYA_SDLK_QUOTEDBL;
        break;
    case Qt::Key_Backslash:
        rbxKey = Aya::AYA_SDLK_BACKSLASH;
        break;
    case Qt::Key_Comma:
        rbxKey = Aya::AYA_SDLK_COMMA;
        break;
    case Qt::Key_Period:
        rbxKey = Aya::AYA_SDLK_PERIOD;
        break;
    case Qt::Key_Slash:
        rbxKey = Aya::AYA_SDLK_SLASH;
        break;
    case Qt::Key_multiply:
        rbxKey = Aya::AYA_SDLK_KP_MULTIPLY;
        break;
    case Qt::Key_NumLock:
        rbxKey = Aya::AYA_SDLK_NUMLOCK;
        break;
    case Qt::Key_ScrollLock:
        rbxKey = Aya::AYA_SDLK_SCROLLOCK;
        break;
    case Qt::Key_Asterisk:
        rbxKey = Aya::AYA_SDLK_ASTERISK;
        break;
    case Qt::Key_Plus:
        rbxKey = Aya::AYA_SDLK_PLUS;
        break;
    }

    return rbxKey;
}

static Aya::ModCode modifiersToUIModCode(int modifier)
{
    unsigned int modCode = 0;

    if (modifier & Qt::ShiftModifier)
    {
        modCode = modCode | Aya::AYA_KMOD_LSHIFT;
    }
    if (modifier & Qt::ControlModifier)
    {
        modCode = modCode | Aya::AYA_KMOD_LCTRL;
    }
    if (modifier & Qt::AltModifier)
    {
        modCode = modCode | Aya::AYA_KMOD_LALT;
    }

    return (Aya::ModCode)modCode;
}
