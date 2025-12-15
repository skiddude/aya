#include "MainWindow.hpp"

#include <QIcon>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent)
{
    QWidget* widget = new QWidget(this);
    app = new AppView(widget, "server");

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(widget);

    setWindowIcon(QIcon(":/icon.ico"));
    setMinimumSize(800, 600);
    setMaximumSize(800, 600);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
}