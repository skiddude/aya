#pragma once
#include <QLineEdit>
#include <QVBoxLayout>
#include <QKeyEvent>

class GrayChatBar : public QLineEdit
{
    Q_OBJECT

public:
    GrayChatBar(QWidget* parent = nullptr);
    void focus();

protected:
    void focusInEvent(QFocusEvent* e) override;
    void focusOutEvent(QFocusEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void handleEnteredText(const QString& text);
    void setVisibility(bool visible);

Q_SIGNALS:
    void returnPressed();
    void enteredText(const QString& text);
};
