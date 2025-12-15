

#pragma once

#include <QAction>

class PluginAction : public QAction
{
public:
    PluginAction(const QString& string, QObject* obj)
        : QAction(string, obj)
    {
    }
};