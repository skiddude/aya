#pragma once

#include <QWidget>
#include <QEvent>

#include "AppView.hpp"

class MainWindow : public QWidget
{
    Q_OBJECT

    AppView* app;

public:
    MainWindow(QWidget* parent = NULL);
//  ~MainWindow();
//
//  bool event(QEvent* event);
};