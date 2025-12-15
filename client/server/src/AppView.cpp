#include "AppView.hpp"

#include <QWebEngineSettings>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QNetworkRequest>
#include <QMessageBox>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QProgressDialog>
#include <QFrame>
#include <QVBoxLayout>
#include <QString>
#include <QSettings>

AppView::AppView(QWidget* parent, std::string mode)
    : QWebEngineView(parent)
{
    setContextMenuPolicy(Qt::NoContextMenu);

    QWebEngineSettings* settings = page()->settings();
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    settings->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
    settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);

    // pre-load the QWebChannel API
    QFile apiFile(":/qtwebchannel/qwebchannel.js");
    if (!apiFile.open(QIODevice::ReadOnly))
        qDebug() << "Couldn't load Qt's QWebChannel API!";
    QString apiScript = QString::fromLatin1(apiFile.readAll());
    apiFile.close();

    QWebEngineScript script;
    script.setSourceCode(apiScript);
    script.setName("QWebChannel");
    script.setWorldId(QWebEngineScript::MainWorld);
    script.setInjectionPoint(QWebEngineScript::DocumentCreation);
    script.setRunsOnSubFrames(false);
    page()->scripts().insert(script);

    // setup the transport interface
    transport = new AppTransport();
    channel = new QWebChannel(page());
    channel->registerObject(QStringLiteral("transport"), transport);
    page()->setWebChannel(channel);

    load(QUrl::fromLocalFile(QString::fromStdString(GetAssetFolder() + "/app/index.html?mode=" + mode)));
}

// #include "AppView.moc"