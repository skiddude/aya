


#include "WebDialog.hpp"

// Qt Headers
#include <QtWebEngineWidgets/QtWebEngineWidgets>

// Roblox Headers
#include "Utility/Statistics.hpp"


// Roblox Studio Headers
#include "RbxWorkspace.hpp"
#include "RobloxBrowser.hpp"
#include "RobloxSettings.hpp"
#include "RobloxWebPage.hpp"
#include "QtUtilities.hpp"
#include "AuthenticationHelper.hpp"

FASTFLAG(WebkitLocalStorageEnabled);
FASTFLAG(WebkitDeveloperToolsEnabled);

WebDialog::WebDialog(QWidget* parent, const QString& initialUrl, Aya::DataModel* dm, int widthInPixels, int heightInPixels)
    : RobloxSavingStateDialog<QDialog>(parent, "WebDialog/Geometry")
    , m_initialUrl(initialUrl)
    , m_pWebView(NULL)
    , m_pDataModel(dm)
    , m_iWidthInPixels(widthInPixels)
    , m_iHeightInPixels(heightInPixels)
{
    // Webview
    m_pWebView = new RobloxBrowser(this);
    m_pWebPage = new RobloxWebPage(m_pWebView);
    m_pWebView->setPage(m_pWebPage);

    load(m_initialUrl);

    QWebEngineProfile* profile = QWebEngineProfile::defaultProfile();
    profile->settings()->setAttribute(QWebEngineSettings::AutoLoadImages, true);
    profile->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    profile->settings()->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
    profile->settings()->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);

    if (FFlag::WebkitLocalStorageEnabled)
    {
        profile->settings()->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    }

    /*
    m_pWorkspace.reset(new RbxWorkspace(this, m_pDataModel));
    connect(m_pWorkspace.get(), SIGNAL(closeEvent()), this, SLOT(close()));
    connect(m_pWorkspace.get(), SIGNAL(hideEvent()), this, SLOT(hide()));
    connect(m_pWorkspace.get(), SIGNAL(showEvent()), this, SLOT(show()));

    connect(m_pWebView->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(initJavascript()));
    connect(m_pWebPage, SIGNAL(windowCloseRequested()), this, SLOT(close()));
    // d9mz - KILL EVERYONE
    */

    // Do not show the help button
    Qt::WindowFlags flags = windowFlags();
    Qt::WindowFlags helpFlag = Qt::WindowContextHelpButtonHint;
    flags = flags & (~helpFlag);
    setWindowFlags(flags);

    // Allow expanding and shrinking in both x and y
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    QtUtilities::updateWindowTitle(this);
}

void WebDialog::load(const QString& url)
{
    m_pWebView->load(url);
}

void WebDialog::initJavascript()
{
    // d9mz - I don't give a fuck
    // m_pWebView->page()->mainFrame()->addToJavaScriptWindowObject("external", m_pWorkspace.get() );
    // hack to get window.close() working with webkit
    // m_pWebView->page()->mainFrame()->evaluateJavaScript("window.open('', '_self')");
}

void WebDialog::resizeEvent(QResizeEvent* evt)
{
    QDialog::resizeEvent(evt);

    m_pWebView->resize(width(), height());
}

QSize WebDialog::sizeHint() const
{
    return QSize(m_iWidthInPixels, m_iHeightInPixels);
}

UploadDialog::UploadDialog(QWidget* parent, Aya::DataModel* dataModel)
    : WebDialog(parent, QString("%1/Game/Upload.aspx").arg(RobloxSettings::getBaseURL()), dataModel)
{
    setWindowModality(Qt::WindowModal);
}

void UploadDialog::reject()
{
    QDialog::reject();
    setResult(CANCEL);
}

void UploadDialog::closeEvent(QCloseEvent* evt)
{
    setResult(CANCEL);

    /*
    // d9mz - No
    QWebElement dialogResult = m_pWebView->page()->mainFrame()->findFirstElement("#DialogResult");
    if ( !dialogResult.isNull() )
    {
            QString value = dialogResult.attribute("value");
            if ( value == "1" )
                    setResult(OK);
            else if ( value == "2" )
                    setResult(CANCEL);
            else if ( value == "3" )
                    setResult(LOCAL_SAVE);
    }*/

    evt->accept();
}
