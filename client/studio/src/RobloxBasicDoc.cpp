


#include "RobloxBasicDoc.hpp"

// Qt Headers
#include <QString>

// Roblox Studio Headers
#include "IExternalHandler.hpp"
#include "UpdateUIManager.hpp"
#include "RobloxDocManager.hpp"

RobloxBasicDoc::RobloxBasicDoc()
    : m_bActive(false)
{
}

RobloxBasicDoc::~RobloxBasicDoc() {}

void RobloxBasicDoc::addExternalHandler(IExternalHandler* extHandler)
{
    if (extHandler == NULL)
        return;

    if (extHandler->handlerId().isEmpty())
        return;

    m_handlersMap[extHandler->handlerId()] = extHandler;
}

void RobloxBasicDoc::removeExternalHandler(const QString& handlerId)
{
    if (handlerId.isEmpty())
        return;

    m_handlersMap.erase(handlerId);
}

IExternalHandler* RobloxBasicDoc::getExternalHandler(const QString& handlerId)
{
    return m_handlersMap[handlerId];
}

bool RobloxBasicDoc::handleAction(const QString& actionID, bool isChecked /*= true*/)
{
    // Try to see if inherited class handles this action
    return doHandleAction(actionID, isChecked);
}

/** do not override **/
bool RobloxBasicDoc::close()
{
    if (doClose())
    {
        RobloxDocManager::Instance().updateWindowMenu();
        return true;
    }
    return false;
}
