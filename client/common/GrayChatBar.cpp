#include "GrayChatBar.hpp"

#define PLACEHOLDER_TEXT "To chat click here or press the \"/\" key, dumbass."

GrayChatBar::GrayChatBar(QWidget* parent) : QLineEdit(parent)
{
    setText(PLACEHOLDER_TEXT);

    // note: this uses segoe ui, may not work on linux?

    // styles:
    // padding-left: 1px
    // border-bottom: 5px solid #404040
    // background-color: #404040 (#e6e6fa on active)
    // color: #ffffc8 (white on active)
    // font-weight: bold
    // font-family: 'Segoe UI'
    // fixed height: 21px

    setStyleSheet("QLineEdit { background-color: #404040; color: #ffffc8; font-weight: bold; border-radius: 0 !important; padding: 0 !important; border: none; border-bottom: 5px solid #404040; margin: 0 !important; font-family: 'Segoe UI'; font-size: 12px; padding-left: 1px; }");
    setFixedHeight(21);
    setMinimumSize(QSize(0, 21));
    setVisible(false);

    QFont font = this->font();
    font.setHintingPreference(QFont::PreferFullHinting); // for crappy aa
    setFont(font);
}

void GrayChatBar::focus()
{
    setFocus();
}

void GrayChatBar::focusInEvent(QFocusEvent* e)
{
    QLineEdit::focusInEvent(e);
    setStyleSheet("QLineEdit { background-color: #e6e6fa; color: black; font-weight: bold; border-radius: 0 !important; padding: 0 !important; border: none; border-bottom: 5px solid #404040; margin: 0 !important; font-family: 'Segoe UI'; font-size: 12px; padding-left: 1px; }");

    if (text() == PLACEHOLDER_TEXT)
    {
        clear();
    }
}

void GrayChatBar::focusOutEvent(QFocusEvent* e)
{
    QLineEdit::focusOutEvent(e);
    setStyleSheet("QLineEdit { background-color: #404040; color: #ffffc8; font-weight: bold; border-radius: 0 !important; padding: 0 !important; border: none; border-bottom: 5px solid #404040; margin: 0 !important; font-family: 'Segoe UI'; font-size: 12px; padding-left: 1px; }");

    if (text() == "")
    {
        setText(PLACEHOLDER_TEXT);
    }
}

void GrayChatBar::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)
    {
        Q_EMIT returnPressed();
        handleEnteredText(text());
        setText(PLACEHOLDER_TEXT);
        clearFocus();
    }
    else
    {
        QLineEdit::keyPressEvent(e);
    }
}

void GrayChatBar::handleEnteredText(const QString& txt) {
    Q_EMIT enteredText(txt);
    if (text() == "")
    {
        setText(PLACEHOLDER_TEXT);
    }
}

void GrayChatBar::setVisibility(bool visible)
{
    setVisible(visible);
}

void GrayChatBar::mousePressEvent(QMouseEvent* e) {
  focus();
}
