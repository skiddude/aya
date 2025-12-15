


#include "RobloxNetworkAccessManager.hpp"

#include <QWebEngineProfile>
#include <QWebEngineCookieStore>

// Roblox Headers
#include "QtUtilities.hpp"
#include "RobloxCookieJar.hpp"
#include "RobloxWebPage.hpp"
#include "RobloxSettings.hpp"
#include "RobloxNetworkReply.hpp"

RobloxNetworkAccessManager::RobloxNetworkAccessManager()
    : QNetworkAccessManager()
{
    // When we enable the Cookie Persistence, logging into mail.roblox.com stops working, but works fine for www.roblox.com with persistence
    setCookieJar(new RobloxCookieJar());
}
void RobloxNetworkAccessManager::initUserAgent()
{
    // RobloxWebPage dummy;
    // userAgent = dummy.getDefaultUserAgent();
    // dummy.profile()->setHttpUserAgent(userAgent);
}
QString RobloxNetworkAccessManager::getUserAgent()
{
    // if (userAgent.isEmpty())
    // 	initUserAgent();
    return QString("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/122.0.0.0 Safari/537.36");

    // return userAgent;
}

RobloxNetworkReply* RobloxNetworkAccessManager::get(const QNetworkRequest& request, bool followRedirection /*= true*/)
{
    QNetworkReply* reply = QNetworkAccessManager::get(request);
    RobloxNetworkReply* robloxreply = new RobloxNetworkReply(reply, followRedirection);
    return robloxreply;
}

RobloxCookieJar* RobloxNetworkAccessManager::cookieJar() const
{
    return dynamic_cast<RobloxCookieJar*>(QNetworkAccessManager::cookieJar());
}
