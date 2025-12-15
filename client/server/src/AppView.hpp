#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QtWebChannel>
#include <QWebEngineView>

class AppTransport : public QObject
{
    Q_OBJECT

    QWidget* parent;
    QNetworkAccessManager* manager;
};

class AppView : public QWebEngineView
{
    Q_OBJECT

    QWebChannel* channel;
    AppTransport* transport;

public:
    AppView(QWidget* parent, std::string mode);

    AppTransport* getTransport()
    {
        return transport;
    };
};