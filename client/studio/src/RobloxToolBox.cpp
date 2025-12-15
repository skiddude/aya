


#include "RobloxToolBox.hpp"

// Qt Headers
#include <QGridLayout>
#include <QWebChannel>
#include <QWebEngineSettings>

// Roblox Headers
#include "DataModel/DataModel.hpp"

#include "DataModel/InsertService.hpp"

#include "DataModel/ContentProvider.hpp"

#include "Utility/StandardOut.hpp"

#include "Players.hpp"


// Roblox Studio Headers
#include "AuthenticationHelper.hpp"
#include "RbxWorkspace.hpp"
#include "RobloxCookieJar.hpp"
#include "RobloxNetworkAccessManager.hpp"
#include "RobloxSettings.hpp"

FASTFLAG(WebkitLocalStorageEnabled);
FASTFLAG(WebkitDeveloperToolsEnabled);

RobloxToolBox::RobloxToolBox()
    : m_pWorkspace()
    , m_pWebView(NULL)
    , reloadView(false)
{
    QGridLayout* layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    setMaximumWidth(250);

    setupWebView(this);
    layout->addWidget(m_pWebView, 0, 0);
    setLayout(dynamic_cast<QLayout*>(layout));

    setMaximumWidth(QWIDGETSIZE_MAX);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    setMinimumWidth(285);
}

void RobloxToolBox::setupWebView(QWidget* wrapperWidget)
{
    m_pWebView = new RobloxBrowser(wrapperWidget);
    m_pWebPage = new RobloxWebPage(wrapperWidget);

    m_pWebView->setPage(m_pWebPage);

    // Use QWebEngineSettings associated with the page instead of global QWebSettings
    QWebEngineSettings* pageSetting = m_pWebPage->settings();

    pageSetting->setAttribute(QWebEngineSettings::AutoLoadImages, true);
    pageSetting->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    pageSetting->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
    pageSetting->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);

    pageSetting->setAttribute(QWebEngineSettings::PluginsEnabled, false);

    /// Keep all this for now, later on we should remove it depending on bare minimum required.
    pageSetting->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    pageSetting->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);

    if (FFlag::WebkitLocalStorageEnabled)
        pageSetting->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);

        /*
    if(FFlag::WebkitDeveloperToolsEnabled)
        pageSetting->setAttribute(QWebEngineSettings::DeveloperExtrasEnabled, true);
        */

        // The connection to javaScriptWindowObjectCleared might need to be replaced or removed,
        // as it pertains to QtWebKit. QtWebEngine handles JavaScript integration differently.

#ifdef AYA_TEST_BUILD
    m_urlString = QUrl("http://localhost:5173/?mode=studio&page=toolbox").toString();
#else
    m_urlString = QUrl::fromLocalFile(Aya::ContentProvider::getAssetFile("app/index.html") + "?mode=studio&page=toolbox").toString();
#endif
    connect(&AuthenticationHelper::Instance(), SIGNAL(authenticationChanged(bool)), this, SLOT(onAuthenticationChanged(bool)));
}


void RobloxToolBox::setDataModel(boost::shared_ptr<Aya::DataModel> pDataModel)
{
    if (m_pDataModel == pDataModel)
        return;

    bool firstTime = false;
    m_pDataModel = pDataModel;

    if (!m_pWorkspace)
    {
        m_pWorkspace.reset(new RbxWorkspace(this, m_pDataModel ? m_pDataModel.get() : NULL));
        firstTime = true;
    }
    else
    {
        m_pWorkspace->setDataModel(pDataModel.get());
    }

    if (!m_pDataModel)
    {
        setEnabled(false);
        return;
    }

    setEnabled(true);

    if (firstTime)
    {
        m_pWebView->load(m_urlString);
    }
    else if (reloadView)
    {
        m_pWebPage->triggerAction(QWebEnginePage::Reload);
    }
    reloadView = false;

    update();
}

void RobloxToolBox::initJavascript()
{
    if (m_pWorkspace && m_pWebView->page())
    {
        QWebChannel* channel = new QWebChannel(m_pWebView->page());
        channel->registerObject(QStringLiteral("external"), m_pWorkspace.get());

        m_pWebView->page()->setWebChannel(channel);

        m_pWebView->page()->runJavaScript(
            QStringLiteral("new QWebChannel(qt.webChannelTransport, function(channel) { window.external = channel.objects.external; });"));
    }
}

QString RobloxToolBox::getTitleFromUrl(const QString& urlString)
{
    // d9mz - too lazy to fix this im gonna kill everybody
    /*
    if(m_pWebView && m_pWebView->page() && m_pWebView->page()->mainFrame())
    {
            int pos = urlString.indexOf("id=");
            if (pos > 0 && pos+3 < urlString.size())
            {
                    QWebElementCollection toolboxItemElements = m_pWebView->page()->mainFrame()->findAllElements(QString("span#span_setitem_%1
    a").arg(urlString.mid(pos+3))); Q_FOREACH(QWebElement toolboxItemElement, toolboxItemElements)
                    {
                            QStringList attributesList = toolboxItemElement.attributeNames();
                            Q_FOREACH(QString attributeName, attributesList)
                            {
                                    if (attributeName == "title")
                                            return toolboxItemElement.attribute(attributeName);
                            }
            }
            }
        });
                    }
        });
            }
    }
    */
    return QString("");
}

void RobloxToolBox::onAuthenticationChanged(bool)
{
    if (m_pDataModel)
        m_pWebPage->triggerAction(QWebEnginePage::Reload);
    else
        reloadView = true;
}

void RobloxToolBox::loadUrl(const QString url)
{
    setEnabled(true);
    m_urlString = url;
    m_pWebView->load(url);
}