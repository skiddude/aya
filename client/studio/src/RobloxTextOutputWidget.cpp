


#include "RobloxTextOutputWidget.hpp"

// Qt Headers
#include <QApplication>
#include <QDateTime>
#include <QThread>
#include <QScrollBar>

// Roblox Studio Headers
#include "AuthoringSettings.hpp"
#include "Roblox.hpp"
#include "RobloxSettings.hpp"

RobloxTextOutputWidget::RobloxTextOutputWidget(QWidget* parent)
    : QPlainTextEdit(parent)
{
    setReadOnly(true);
    setTextInteractionFlags(Qt::TextSelectableByMouse);

    // set up colors and font styles
    QString assetFolder = StudioAppSettings::instance().contentFolder();
    int id = QFontDatabase::addApplicationFont(assetFolder + "/fonts/cascadiacode.ttf");
    QString family = QFontDatabase::applicationFontFamilies(id).at(0);
    QFont consolas(family);
    consolas.setPointSize(10);

    m_TextFormats[Aya::MESSAGE_OUTPUT].setFont(consolas);
    m_TextFormats[Aya::MESSAGE_INFO].setFont(consolas);
    m_TextFormats[Aya::MESSAGE_WARNING].setFont(consolas);
    m_TextFormats[Aya::MESSAGE_ERROR].setFont(consolas);
    m_TextFormats[Aya::MESSAGE_SENSITIVE].setFont(consolas);

    m_TextFormats[Aya::MESSAGE_OUTPUT].setFontWeight(QFont::Normal);

    m_TextFormats[Aya::MESSAGE_INFO].setFontWeight(QFont::Normal);
    m_TextFormats[Aya::MESSAGE_INFO].setForeground(Qt::blue);

    m_TextFormats[Aya::MESSAGE_WARNING].setFontWeight(QFont::Bold);
    m_TextFormats[Aya::MESSAGE_WARNING].setForeground(QColor(255, 128, 0));

    m_TextFormats[Aya::MESSAGE_ERROR].setFontWeight(QFont::Bold);
    m_TextFormats[Aya::MESSAGE_ERROR].setForeground(Qt::red);

    m_TextFormats[Aya::MESSAGE_SENSITIVE].setFontWeight(QFont::Bold);
    m_TextFormats[Aya::MESSAGE_SENSITIVE].setForeground(Qt::darkMagenta);

    // listen for changes to the editor settings
    m_PropertyChangedConnection =
        AuthoringSettings::singleton().propertyChangedSignal.connect(boost::bind(&RobloxTextOutputWidget::onPropertyChanged, this, _1));
    onPropertyChanged(NULL);

    connect(&Roblox::Instance(), SIGNAL(newOutputMessage(const QString, Aya::MessageType)), this,
        SLOT(appendOutputText(const QString&, Aya::MessageType)));
}

RobloxTextOutputWidget::~RobloxTextOutputWidget()
{
    m_PropertyChangedConnection.disconnect();
}

QSize RobloxTextOutputWidget::sizeHint() const
{
    return QSize(width(), 104);
}

void RobloxTextOutputWidget::contextMenuEvent(QContextMenuEvent* evt)
{
    QMenu* menu = new QMenu(this);
    QAction* a;

    a = menu->addAction(tr("&Copy\t") + QKeySequence(QKeySequence::Copy).toString(), this, SLOT(copy()));
    a->setEnabled(textCursor().hasSelection());

    menu->addSeparator();
    a = menu->addAction(tr("Select All\t") + QKeySequence(QKeySequence::SelectAll).toString(), this, SLOT(selectAll()));
    a->setEnabled(!document()->isEmpty());

    a = menu->addAction(tr("Clear Output"), this, SLOT(clear()));
    a->setEnabled(!document()->isEmpty());

    menu->exec(evt->globalPos());
    delete menu;
}

bool RobloxTextOutputWidget::event(QEvent* evt)
{
    bool handled = false;

    if (evt->type() == QEvent::ShortcutOverride)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(evt);
        if (keyEvent->matches(QKeySequence::Copy))
        {
            copy();
            keyEvent->accept();
            handled = true;
        }
        else if (keyEvent->matches(QKeySequence::SelectAll))
        {
            selectAll();
            keyEvent->accept();
            handled = true;
        }
    }
    else
    {
        handled = QPlainTextEdit::event(evt);
    }

    return handled;
}

void RobloxTextOutputWidget::onPropertyChanged(const Aya::Reflection::PropertyDescriptor* pDescriptor)
{
    if (!pDescriptor || pDescriptor->name.str == "Maximum Output Lines")
    {
        setMaximumBlockCount(AuthoringSettings::singleton().maximumOutputLines);
    }
}


bool RobloxTextOutputWidget::isScrollOnBottom()
{
    int verticalBarValue = verticalScrollBar()->value(), maximumValue = verticalScrollBar()->maximum();
    return verticalBarValue == maximumValue;
}

void RobloxTextOutputWidget::resizeEvent(QResizeEvent* e)
{
    bool isOnBottom = isScrollOnBottom();

    QPlainTextEdit::resizeEvent(e);

    if (isOnBottom)
    {
        int maximumValue = verticalScrollBar()->maximum();
        verticalScrollBar()->setValue(maximumValue);
    }
}

/**
 * Callback for handling a log entry coming from the engine.
 *
 * @param   message     note that this is a copy, not a reference to handle multi-threading
 * @param   type        type of log message
 */
void RobloxTextOutputWidget::appendOutputText(const QString message, Aya::MessageType type)
{
    AYAASSERT(QThread::currentThread() == qApp->thread());

    QTextCharFormat dateTimeFormat;
    dateTimeFormat.setForeground(Qt::gray);
    QFont dateTimeFont = m_TextFormats[type].font();
    dateTimeFont.setBold(false);
    dateTimeFormat.setFont(dateTimeFont);

    QString dateTime = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    dateTime.append("  ");

    bool isOnBottom = isScrollOnBottom();
    int oldScrollValue = verticalScrollBar()->value();

    QTextCursor oldTextCursor = textCursor();

    // This will stop highlighting.  If a selection is highlighted from right to left the QT overrides
    //  setCurrentCharFormat with whatever text color was at the cursor selection location. This terminates
    //  highlighting before the format is set so it will be the set format, not the cursor location format.
    moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);

    // Set the format for the datetime
    setCurrentCharFormat(dateTimeFormat);
    insertPlainText(dateTime);

    setCurrentCharFormat(m_TextFormats[type]);
    insertPlainText(message);

    insertPlainText("\n");

    if (oldTextCursor.hasSelection())
        setTextCursor(oldTextCursor);

    if (!isOnBottom)
        verticalScrollBar()->setValue(oldScrollValue);
}