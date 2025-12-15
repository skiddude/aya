


#include "ExternalHandlers.hpp"

// Qt Headers
#include <QUrl>

// Roblox Studio Headers
#include "FastLog.hpp"
#include "RobloxSettings.hpp"
#include "StudioUtilities.hpp"

TeleportHandler::TeleportHandler(const QString& handlerId, const QString& url, const QString& ticket, const QString& teleportScript)
    : m_handlerId(handlerId)
    , m_url(url)
    , m_ticket(ticket)
    , m_teleportScript(teleportScript)
{
}

bool TeleportHandler::handle()
{
    return true;
}
