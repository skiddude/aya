

#pragma once

// Qt Headers
#include <QTabWidget>

class RobloxTabWidget : public QTabWidget
{
public:
    RobloxTabWidget(QWidget* parent)
        : QTabWidget(parent)
    {
    }

    QTabBar& getTabBar()
    {
        return *tabBar();
    }
};
