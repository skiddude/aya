


#include "RobloxWebPage.hpp"

// Qt Headers
#include <QDesktopServices>
#include <QNetworkReply>
#include <QWebEnginePage>
#include <QKeyEvent>
#include <QCoreApplication>

// Roblox Headers
#include "AuthenticationHelper.hpp"
#include "FastLog.hpp"
#include "RobloxBrowser.hpp"
#include "RobloxNetworkAccessManager.hpp"
#include "RobloxSettings.hpp"
#include "QtUtilities.hpp"

RobloxWebPage::RobloxWebPage(QWidget* parent)
    : QWebEnginePage(parent)
{
    connect(&RobloxNetworkAccessManager::Instance(), &RobloxNetworkAccessManager::finished, this, &RobloxWebPage::handleFinished);
}

void RobloxWebPage::handleFinished(QNetworkReply* reply)
{
    QString status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString();
    if (status.toInt() != 200 || !reply->hasRawHeader("refresh"))
        return;

    QString acceptHeader = reply->request().rawHeader("accept").data();
    if (!acceptHeader.startsWith("application"))
        return;

    QString url = QString(reply->rawHeader("refresh").data());
    // d9mz - Uhhh kill everybody
    // this->mainFrame()->load(url.mid(url.indexOf("=")+1));
}

bool RobloxWebPage::acceptNavigationRequest(const QUrl& url, NavigationType type, bool isMainFrame)
{
    /*
if (!isMainFrame) {
    // QDesktopServices::openUrl(url);
    return false;
} else {*/
    return true;
    // }
}

QString RobloxWebPage::getDefaultUserAgent() const
{
    // d9mz - i hate webkit and webengine and everything and im going to kill someone
    return QString("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/122.0.0.0 Safari/537.36");
}

// If we have an override for the file upload element on this page, use that instead
// This allows us to inject files into file input tags <input type="file" /> on the page
// d9mz - check header for method
/* QString RobloxWebPage::chooseFile( QWebEnginePage *originatingFrame, const QString& oldFile )
{
        if (m_overideUploadFile.isEmpty())
                return QWebEnginePage::chooseFile(originatingFrame, oldFile);
        else
                return m_overideUploadFile;
} */

void RobloxWebPage::setUploadFile(QString selector, QString fileName)
{
    // Store for uploading
    m_overideUploadFile = fileName;

    // Invoke an "file selection" behind the scenes.  This will automatically put the file into
    // the pages file input tag.
    // d9mz - we CANNOT do this shit in qtwebengine. We resort to js
    /*
    QWebElement button = this->mainFrame()->findFirstElement(selector);
    button.setFocus();
    QKeyEvent *event = new QKeyEvent ( QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
    QCoreApplication::postEvent (this->view(), event);
    */
}
bool RobloxWebPage::event(QEvent* evt)
{
    if (evt->type() == QEvent::ContextMenu)
        m_contextPos = static_cast<QContextMenuEvent*>(evt)->pos();

    return QWebEnginePage::event(evt); // Call event directly
}

void RobloxWebPage::triggerAction(QWebEnginePage::WebAction action, bool checked)
{
    QWebEnginePage::triggerAction(action, checked); // Call triggerAction directly
}