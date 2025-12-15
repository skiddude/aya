#pragma once

#include <QWebEngineView>
#include <QtWebChannel>
#include <QObject>
#include <qwebengineview.h>
#include <thread>
#include <QNetworkAccessManager>

class JsHelpers : public QObject
{
    Q_OBJECT

    QWidget* parentWidget;
    QNetworkAccessManager* netManager;

    std::string masterServerAuthorization;
    std::string currentServerAuthorization;
    std::string masterServerUrl;
    std::string selectedFilePath;
    std::string robloSecurityCookie;

    void reloadSettings();

public:
    std::string getCurrentServerAuthorization()
    {
        return currentServerAuthorization;
    };
    std::string getFilePath()
    {
        return selectedFilePath;
    }

    JsHelpers(QWidget* parentWidget = NULL);
public Q_SLOTS:
    // send game configuration to aya; can use lua script
    void launchGame(const QString& gameConfiguration, const int& virtualVersion)
    {
        Q_EMIT onLaunchingGame(gameConfiguration, virtualVersion);
    };
    void startAvatarView()
    {
        Q_EMIT onStartAvatarView();
    };
    void stopAvatarView()
    {
        Q_EMIT onStopAvatarView();
    };

    // open settings panel
    void openSettings();
    // return JSON string from server endpoint (should be set after using openSettings)
    QString grabServers();
    QString getMasterServerURL();
    QString getRobloSecurityCookie();
    // get current username or logged in username
    QString userName();
    QString characterAppearanceJSON();
    void setSetting(QString setting, QString value);
    void setMasterServerURL(QString url);
    void setRobloSecurityCookie(QString cookie);
    void setUserName(QString string);
    void enableChatBarWidget();
    void setCharacterAppearanceJSON(QString json);
    void launchStudio();
    int getCompileTime();
    QString getPlatformName();
    QString getCompilerName();
    QString getVersion();
    bool isUsingInstance();
    QString getInstanceName();
    QString getInstanceMotd();
    QString getInstanceURL();
    // get current map
    QString currentMap()
    {
        return QString::fromStdString(selectedFilePath);
    }

    // selects map
    void openMapPickerDialog();
    // present motd for server
    void presentMotd(QString motd);
    // start rccservice in another thread and connect using lua script
    void startRcc(const QString& serverConfiguration, int port, int virtualVersion, bool forceVirtualVersion)
    {
        Q_EMIT onStartRcc(serverConfiguration, port, virtualVersion, forceVirtualVersion);
    };
    // reads to password type if read is true
    // returns true if type is set, returns false if type is blank
    // types: "masterServer", "serverPassword"
    bool readPassword(const QString& type, bool read);
    bool hasServer();

    void wearAsset(int64_t assetId, QString category);
    void setBodyColorJson(QString json);
    QString getBodyColorJson();
    void setBodyColor(QString bodyPart, int bodyColor);
Q_SIGNALS:
    void onStartAvatarView();
    void onStopAvatarView();
    void onLaunchingGame(const QString& gameConfiguration, const int& virtualVersion);
    void onMapPicked(QString& filePicked);
    void onStartRcc(const QString& serverConfiguration, int port, int virtualVersion, bool forceVirtualVersion);
};

class LauncherView : public QWebEngineView
{
    Q_OBJECT

    QWebChannel* channel;
    JsHelpers* jsHelpers;


public:
    LauncherView(QWidget* parent);

    JsHelpers* getJsHelpers()
    {
        return jsHelpers;
    }
};