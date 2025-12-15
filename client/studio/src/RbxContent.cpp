


#include "RbxContent.hpp"

// Roblox Headers
#include "Utility/Http.hpp"

#include "Utility/StandardOut.hpp"


#include "RobloxGameExplorer.hpp"
#include "UpdateUIManager.hpp"

FASTFLAG(StudioEnableGameAnimationsTab)

RbxContent::RbxContent(QObject* parent)
    : QObject(parent)
{
}

// Publish Selection To Roblox
QString RbxContent::Upload(const QString& urlQStr)
{
    QByteArray ba = urlQStr.toLocal8Bit();
    const char* url = ba.data();

    bool success = Aya::Http::trustCheck(url);
    std::string response;
    if (success)
    {
        try
        {
            Aya::Http http(url);
            http.post(data, Aya::Http::kContentTypeApplicationXml, true, response);
        }
        catch (std::exception& e)
        {
            Aya::StandardOut::singleton()->print(Aya::MESSAGE_ERROR, e.what());
            return QString("");
        }
    }

    // if we are creating 'new' animation game asset then we will need to reload game explorer
    if (FFlag::StudioEnableGameAnimationsTab && !response.empty() && urlQStr.contains("uploadnewanimation?assetTypeName=Animation") &&
        urlQStr.contains("isGamesAsset=True"))
    {
        RobloxGameExplorer& gameExplorer = UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER);
        QMetaObject::invokeMethod(&gameExplorer, "reloadDataFromWeb", Qt::QueuedConnection);
    }

    return QString::fromStdString(response);
}
