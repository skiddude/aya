

#pragma once

// Standard C/C++ Headers
#include <sstream>

// Qt Headers
#include <QObject>

// Required for Publish Selection to Roblox
class RbxContent : public QObject
{
    Q_OBJECT

public:
    RbxContent(QObject* parent);
    std::stringstream data;

public Q_SLOTS:
    QString Upload(const QString& url);
};
