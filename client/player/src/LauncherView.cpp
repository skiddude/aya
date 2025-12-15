#include "LauncherView.hpp"
#include "Utility/BrickColor.hpp"
#include "AvatarViewService.hpp"
#include "Tree/Service.hpp"
#include "Application.hpp"
#include "Window.hpp"
#include "DataModel/ContentProvider.hpp"
#include "Utility/Statistics.hpp"
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QFileDialog>
#include <QInputDialog>
#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QProgressDialog>
#include <QFrame>
#include <QWebEngineSettings>
#include <QSettings>
#include <QDialogButtonBox>
#include <QTabWidget>
#include <QLabel>
#include <QMessageBox>
#include <QApplication>
#include <QNetworkRequest>
#include <QCheckBox>
#include <qnetworkrequest.h>
#include <QString>
#include <qsettings.h>
#include "winrc.h"
#include <rapidjson/document.h>

LauncherView::LauncherView(QWidget* parent)
    : QWebEngineView(parent)
{
    channel = new QWebChannel(page());
    page()->setWebChannel(channel);
    setContextMenuPolicy(Qt::NoContextMenu);
    jsHelpers = new JsHelpers();
    channel->registerObject(QStringLiteral("transport"), jsHelpers);

    QFile apiFile(":/qtwebchannel/qwebchannel.js"); // load the API from the resources
    if (!apiFile.open(QIODevice::ReadOnly))
        qDebug() << "Couldn't load Qt's QWebChannel API!";
    QString apiScript = QString::fromLatin1(apiFile.readAll());
    apiFile.close();

    QWebEngineScript script;
    script.setSourceCode(apiScript);
    script.setName("qtwebchannel.js");
    script.setWorldId(QWebEngineScript::MainWorld);
    script.setInjectionPoint(QWebEngineScript::DocumentCreation);
    script.setRunsOnSubFrames(false);
    page()->scripts().insert(script);

    std::string assetFolderPath = Aya::ContentProvider::assetFolder();
    QString qAssetFolderPath = QString::fromStdString(assetFolderPath);
#ifdef AYA_TEST_BUILD
    QUrl url = QUrl("http://localhost:5173");
#else
    QUrl url = QUrl::fromLocalFile(qAssetFolderPath + "/app/index.html");
#endif

    QWebEngineSettings* settings = page()->settings();

    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    settings->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
    settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);

#ifdef AYA_TEST_BUILD
    // Enable DevTools
    settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);

#endif

    page()->load(url);
    page()->setBackgroundColor(Qt::transparent); // to see avatar when customizing
}

JsHelpers::JsHelpers(QWidget* parentWidget)
{
    reloadSettings();

    netManager = new QNetworkAccessManager();
    this->parentWidget = parentWidget;

    QSettings settings;
    selectedFilePath = settings.value("JsHelpers/selectedFilePath").toString().toStdString();
}

void JsHelpers::reloadSettings()
{
    QSettings settings;
    masterServerAuthorization = settings.value("JsHelpers/masterServerAuthorization", "").toString().toStdString();
    masterServerUrl = settings.value("JsHelpers/masterServerUrl").toString().toStdString();
    robloSecurityCookie = settings.value("JsHelpers/robloSecurityCookie").toString().toStdString();
    currentServerAuthorization = "";
}

extern QApplication* qtAppPtr;

int JsHelpers::getCompileTime()
{
    return AYA_BUILD_TIMESTAMP;
}

QString JsHelpers::getPlatformName()
{
    return AYA_PLATFORM_NAME;
}

QString JsHelpers::getCompilerName()
{
    return AYA_COMPILER_ID;
}

QString JsHelpers::getVersion()
{
    return VERSION_FULL_STR;
}

bool JsHelpers::isUsingInstance()
{
    return IsUsingInstance();
}

QString JsHelpers::getInstanceURL()
{
    return QString::fromStdString(GetBaseURL());
}

QString JsHelpers::getInstanceName()
{
    return QString::fromStdString(GetInstanceName());
}

QString JsHelpers::getInstanceMotd()
{
    return QString::fromStdString(GetInstanceMotd());
}


QString JsHelpers::getMasterServerURL()
{
    if (masterServerUrl == "")
    {
        QMessageBox::critical(parentWidget, "Request Error", "There is no master server set. Use Advanced Settings to select one.");
        return QString();
    }

    return QString::fromStdString(masterServerUrl);
}

void JsHelpers::setSetting(QString setting, QString value)
{
    QSettings settings;
    settings.beginGroup("JsHelpers");
    settings.setValue(setting, value);
    settings.endGroup();
}

void JsHelpers::setMasterServerURL(QString url)
{
    QSettings settings;
    settings.setValue("JsHelpers/masterServerUrl", url);
}

void JsHelpers::setRobloSecurityCookie(QString cookie)
{
    QSettings settings;
    settings.setValue("JsHelpers/robloSecurityCookie", cookie);
}

QString JsHelpers::getRobloSecurityCookie()
{
    if (robloSecurityCookie == "")
    {
        return QString();
    }

    return QString::fromStdString(robloSecurityCookie);
}

QString JsHelpers::grabServers()
{
    if (masterServerUrl == "")
    {
        QMessageBox::critical(parentWidget, "Request Error", "There is no master server set. Use Advanced Settings to select one.");
        return QString();
    }

    QUrl url = QUrl::fromUserInput(QString::fromStdString(masterServerUrl) + "/ping");
    printf("%s\n", url.toString().toStdString().c_str());

    if (!url.isValid())
    {
        QMessageBox::critical(parentWidget, "Request Error", "The URL for the master server is invalid.");
        return QString();
    }

    QNetworkRequest req = QNetworkRequest();
    if (masterServerAuthorization != "")
    {
        req.setRawHeader("Authorization", QByteArray(masterServerAuthorization.c_str(), masterServerAuthorization.length()));
    }
    req.setUrl(url);

    QNetworkReply* reply = netManager->get(req);

    QNetworkReply::NetworkError error; // = reply->error();

    QProgressDialog progress("Grabbing servers...", "Cancel", 0, 1, parentWidget);
    progress.setWindowModality(Qt::WindowModal);

    while (!reply->isFinished())
    {
        progress.setValue(0);
        progress.update();

        qtAppPtr->processEvents();
    }

    progress.close();

    error = reply->error();
    if (error)
    {
        printf("Error %i '%s'\n", error, reply->errorString().toStdString().c_str());
        QMessageBox::critical(parentWidget, "Request Error", "Aya could not reach the selected master server.");
    }
    QByteArray responseB = reply->readAll();
    QString response(responseB);
    printf("%s\n", response.toStdString().c_str());

    return response;
}

QString JsHelpers::userName()
{
    QSettings settings;
    QString username = settings.value("JsHelpers/Username").toString();
    if (username.isEmpty())
    {
        QString sysUsername = qEnvironmentVariable("USER");
        if (sysUsername.isEmpty())
            sysUsername = qEnvironmentVariable("USERNAME");
        settings.setValue("JsHelpers/Username", sysUsername);
        return sysUsername;
    }
    return username;
}

QString JsHelpers::characterAppearanceJSON()
{
    QSettings settings;
    QString json = settings.value("JsHelpers/CharApp").toString();
    if (json.isEmpty())
    {
        settings.setValue("JsHelpers/CharApp", "[]");
        return QString("[]");
    }
    return json;
}

void JsHelpers::setBodyColorJson(QString json)
{
    QSettings settings;
    settings.setValue("JsHelpers/BodyColors", json);
}

QString JsHelpers::getBodyColorJson()
{
    QSettings settings;
    return settings.value("JsHelpers/BodyColors").toString();
}

void JsHelpers::setCharacterAppearanceJSON(QString json)
{
    QSettings settings;
    settings.setValue("JsHelpers/CharApp", json);
}

void JsHelpers::setUserName(QString username)
{
    QSettings settings;
    settings.setValue("JsHelpers/Username", username);
}

extern OgreWindow* qtOgreWidget;

void JsHelpers::enableChatBarWidget()
{
    OgreWindow* _ogreWindow = dynamic_cast<OgreWindow*>(qtOgreWidget);
    if (!_ogreWindow)
        return;

    GrayChatBar* textInput = dynamic_cast<OgreWindow*>(qtOgreWidget)->getTextInput();
    textInput->setVisible(true);
}

void JsHelpers::launchStudio()
{
    OgreWindow* _ogreWindow = dynamic_cast<OgreWindow*>(qtOgreWidget);
    if (!_ogreWindow)
        return;

    QString studioPath = "Aya.Studio";
#ifdef _WIN32
    studioPath += ".exe";
#endif
    if (QFile::exists(studioPath))
    {
        QProcess::startDetached(studioPath);
    }
    else
    {
        QMessageBox::critical(parentWidget, "Cannot launch Studio", "Aya Studio was not found in the current directory");
    }
}

void JsHelpers::openMapPickerDialog()
{
    QString filename = QFileDialog::getOpenFileName(nullptr, tr("Open Place File"), QString(), tr("Level (*.rbxl)"));
    selectedFilePath = filename.toStdString();
    QSettings settings;
    settings.setValue("JsHelpers/selectedFilePath", filename);
    Q_EMIT onMapPicked(filename);
}

class MotdDialog : public QDialog
{
    Q_OBJECT

    QWebEngineView* webView;
    QPushButton* okButton;

    void okButtonPressed()
    {
        close();
    }

public:
    MotdDialog(QString motd, QWidget* parent = NULL)
        : QDialog(parent)
    {
        QVBoxLayout* layout = new QVBoxLayout(this);
        setMinimumSize(0, 0);

        QSettings qsettings;
        restoreGeometry(qsettings.value("Player/MotdGeometry").toByteArray());

        setAttribute(Qt::WA_DeleteOnClose);

        webView = new QWebEngineView();
        webView->setHtml(motd);
        webView->setMinimumSize(300, 200);
        QWebEngineSettings* settings = webView->page()->settings();

        settings->setAttribute(QWebEngineSettings::JavascriptEnabled, false);
        settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, false);
        settings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, false);
        settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, false);
        settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
        settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, false);
        settings->setAttribute(QWebEngineSettings::WebGLEnabled, false);
        settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, false);
        settings->setAttribute(QWebEngineSettings::AutoLoadIconsForPage, false);
        settings->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, false);
        settings->setAttribute(QWebEngineSettings::JavascriptCanPaste, false);
        settings->setAttribute(QWebEngineSettings::AllowWindowActivationFromJavaScript, false);
        layout->addWidget(webView);

        okButton = new QPushButton();
        okButton->setText("OK");
        okButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        layout->addWidget(okButton);

        setWindowTitle("Message of the day");

        connect(okButton, &QPushButton::pressed, this, &MotdDialog::okButtonPressed);
    }

    ~MotdDialog()
    {
        QSettings settings;
        settings.setValue("Player/MotdGeometry", saveGeometry());
    }
};

void JsHelpers::presentMotd(QString motd)
{
    MotdDialog* d = new MotdDialog(motd, parentWidget);
    d->show();
}

bool JsHelpers::readPassword(const QString& type, bool read)
{
    const QString MASTER_SERVER = "masterServer";
    const QString CURRENT_SERVER = "serverPassword";

    if (read)
    {
        bool ok;
        QString key = QInputDialog::getText(parentWidget, tr("Enter key"), tr("Enter key"), QLineEdit::Password, QString(), &ok);
        if (ok)
        {
            if (type == MASTER_SERVER)
            {
                QSettings settings;
                masterServerAuthorization = key.toStdString();
                settings.setValue("JsHelpers/masterServerAuthorization", key);
            }
            else if (type == CURRENT_SERVER)
            {
                currentServerAuthorization = key.toStdString();
            }
        }
        else
        {
            if (type == CURRENT_SERVER)
            {
                currentServerAuthorization = "";
            }
        }
    }

    if (type == MASTER_SERVER)
    {
        return !masterServerAuthorization.empty();
    }
    else if (type == CURRENT_SERVER)
    {
        return !masterServerAuthorization.empty();
    }

    return false;
}

bool JsHelpers::hasServer()
{
    QString fname;
#ifdef _WIN32
    fname = "Aya.Server.exe";
#else
    fname = "Aya.Server";
#endif
    return (QFile::exists(fname));
}

extern Aya::Application* appPtr;

void JsHelpers::wearAsset(int64_t assetId, QString category)
{
    // TODO: at a later date
    /*QUrl url = QUrl::fromUserInput(QString::fromStdString(masterServerUrl) + "/wear");
      printf("%s\n", url.toString().toStdString().c_str());

      if (!url.isValid())
      {
          QMessageBox::critical(parentWidget, "Request Error", "The URL for the master server is invalid.");
          return;
      }

      QUrlQuery postData;
      postData.addQueryItem("assetId", assetId);
      QNetworkRequest req = QNetworkRequest();
      if (masterServerAuthorization != "")
      {
          req.setRawHeader("Authorization", QByteArray(masterServerAuthorization.c_str(), masterServerAuthorization.length()));
      }
      req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
      req.setUrl(url);

      QNetworkReply* reply = netManager->post(req, postData.toString(QUrl::FullyEncoded).toUtf8());
      connect(reply, &QNetworkReply::finished, this, [=](){
        if(reply->error()) {
          QMessageBox::critical(parentWidget, "Request Error", reply->errorString());
        }
        }); */


    Aya::AvatarViewService* avatarViewService = Aya::ServiceProvider::create<Aya::AvatarViewService>(appPtr->getDM().get());
    avatarViewService->assetWornSignal(assetId, category.toStdString());
}

void JsHelpers::setBodyColor(QString bodyPart, int bodyColor)
{
    Aya::AvatarViewService* avatarViewService = Aya::ServiceProvider::create<Aya::AvatarViewService>(appPtr->getDM().get());
    avatarViewService->setBodyColorSignal(bodyPart.toStdString(), Aya::BrickColor(bodyColor));
}

class SettingsDialog : public QDialog
{
    struct SettingsEntry
    {
        QString name;     // visual
        QString property; //
        QString type;
        int tab;
        QWidget* widget;

        QString filePathValue; // path value, used only in `file' mode
    };

    std::vector<SettingsEntry> entries;

    void addEntry(QString name, QString property, QString type, int tab = 0)
    {
        SettingsEntry e;
        e.name = name;
        e.property = property;
        e.type = type;
        e.tab = tab;
        entries.push_back(e);
    };

    QTabWidget* tabWidget;
    QDialogButtonBox* buttonBox;

    void accept()
    {
        QSettings settings;
        for (auto& e : entries)
        {
            if (e.type == "password" || e.type == "text")
            {
                settings.setValue(e.property, ((QLineEdit*)e.widget)->text());
            }
            else if (e.type == "bool")
            {
                Qt::CheckState cs = ((QCheckBox*)e.widget)->checkState();
                settings.setValue(e.property, cs == Qt::Checked);
            }
            else if (e.type == "file")
            {
                settings.setValue(e.property, e.filePathValue);
            }
        }

        QDialog::accept();
    }

    void openButton() {}

public:
    SettingsDialog(QWidget* parent = NULL)
    {
        setAttribute(Qt::WA_DeleteOnClose);

        tabWidget = new QTabWidget;

        QWidget* generalTab = new QWidget();
        QWidget* serverTab = new QWidget();
        QWidget* creditsTab = new QWidget();
        tabWidget->addTab(generalTab, tr("General"));
        tabWidget->addTab(serverTab, tr("Server"));
        tabWidget->addTab(creditsTab, tr("Credits"));

        buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

        connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        QVBoxLayout* mainLayout = new QVBoxLayout;
        mainLayout->addWidget(tabWidget);
        mainLayout->addWidget(buttonBox);
        setLayout(mainLayout);

        QVBoxLayout* generalLayout = new QVBoxLayout;
        QVBoxLayout* serverLayout = new QVBoxLayout;

        std::vector<QVBoxLayout*> tgtLayouts;
        tgtLayouts.push_back(generalLayout);
        tgtLayouts.push_back(serverLayout);

        generalLayout->addStretch(1);
        generalLayout->setAlignment(Qt::AlignTop);
        generalTab->setLayout(generalLayout);
        serverLayout->addStretch(1);
        serverLayout->setAlignment(Qt::AlignTop);
        serverTab->setLayout(serverLayout);

        // HOW TO ADD NEW TABS:
        // create a QWidget for your tab
        // create a box layout for the QWidget
        // add it to tgtLayouts, then use the tab parameter of addEntry to set which settings get put where

        addEntry("Master Server URL", "JsHelpers/masterServerUrl", "text");
        addEntry("Master Server Key", "JsHelpers/masterServerAuthorization", "password");
        addEntry("Host Server Password", "JsHelpers/serverHostPassword", "password");

        addEntry("Server MOTD Preview Use Place File", "JsHelpers/serverMotdPreviewUsePlaceFile", "bool", 1);
        addEntry("Server MOTD Preview", "JsHelpers/serverMotdPreview", "text", 1);
        addEntry("Server MOTD File (*.html, *.txt)", "JsHelpers/serverMotdFile", "file", 1);

        QSettings settings;
        for (auto& e : entries)
        {
            QLabel* label = new QLabel(e.name);
            QVariant val = settings.value(e.property);
            tgtLayouts[e.tab]->addWidget(label);
            e.widget = NULL;
            if (e.type == "password" || e.type == "text")
            {
                QLineEdit* ed = new QLineEdit(val.toString());
                if (e.type == "password")
                    ed->setEchoMode(QLineEdit::Password);
                e.widget = ed;
            }
            else if (e.type == "bool")
            {
                QCheckBox* cb = new QCheckBox();
                cb->setCheckState(val.toBool() ? Qt::Checked : Qt::Unchecked);
                e.widget = cb;
            }
            else if (e.type == "file")
            {
                QPushButton* pb = new QPushButton();
                pb->setText("Select File");
                QObject::connect(pb, &QPushButton::clicked,
                    [this, &e](bool checked)
                    {
                        e.filePathValue = QFileDialog::getOpenFileName(this, tr("Select File"), QString(), e.name);
                    });
                e.widget = pb;
            } // TODO: file picker
            if (e.widget)
                tgtLayouts[e.tab]->addWidget(e.widget);
        }

        QFile creditsFile(":/credits.txt"); // load the credits from the resources
        if (!creditsFile.open(QIODevice::ReadOnly))
            qDebug() << "Couldn't load Credits!";
        QString credits = QString::fromLatin1(creditsFile.readAll());
        creditsFile.close();

        QLabel* tbdLabel = new QLabel(credits);
        tbdLabel->setWordWrap(true);
        QVBoxLayout* creditsLayout = new QVBoxLayout;
        creditsLayout->setAlignment(Qt::AlignTop);
        creditsLayout->addWidget(tbdLabel);
        creditsTab->setLayout(creditsLayout);
    }
};

void JsHelpers::openSettings()
{
    SettingsDialog* d = new SettingsDialog();
    d->exec();
    reloadSettings();
}

#include "LauncherView.moc"