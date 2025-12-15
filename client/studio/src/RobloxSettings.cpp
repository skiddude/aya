


#include "RobloxSettings.hpp"

// Qt Headers
#include <QDomDocument>
#include <QFile>
#include <QDir>
#include <QApplication>
#include <QDesktopServices>
#include <QMessageBox>

// Roblox Headers
#include "DataModel/ContentProvider.hpp"

#include "DataModel/DebugSettings.hpp"

#include "Script/script.hpp"
#include "Utility/Statistics.hpp"

#include <QStandardPaths>

// Roblox Studio Headers
#include "RbxWorkspace.hpp"

#include "winrc.h"

StudioAppSettings& StudioAppSettings::instance()
{
    static StudioAppSettings singleton;
    return singleton;
}

StudioAppSettings::StudioAppSettings()
    : m_bIsScriptAssetUploadEnabled(false)
    , m_bIsAnimationAssetUploadEnabled(false)
    , m_bIsImageModelAssetUploadEnabled(false)
{
    m_tempLocation = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
}

RobloxSettings::RobloxSettings()
    : QSettings()
{
}

void RobloxSettings::saveAssets()
{
    // RobloxSettings settings;
    // settings.setValue("BaseUrl", QString(::GetBaseURL().c_str()));
    // settings.setValue("ContentFolder", QString(Aya::ContentProvider::assetFolder().c_str()));
}

void RobloxSettings::recoverAssets()
{
    QString baseUrl = StudioAppSettings::instance().baseURL();

    QString assetFolder = StudioAppSettings::instance().contentFolder();
    QByteArray assetFolderUtf8 = assetFolder.toUtf8();

    Aya::ContentProvider::setAssetFolder(std::string(assetFolderUtf8.data(), assetFolderUtf8.size()).c_str());
}

QString RobloxSettings::getAssetFolder()
{
    return QString::fromStdString(GetAssetFolder());
}

QString RobloxSettings::getBuiltInPluginsFolder()
{
    return QString::fromStdString(Aya::ContentProvider::getPluginsFolder());
}

QString RobloxSettings::getBaseURL()
{
    return QString::fromStdString(GetBaseURL());
}

QString RobloxSettings::getApiBaseURL()
{
    return QString::fromStdString(Aya::ContentProvider::getUnsecureApiBaseUrl(getBaseURL().toStdString()));
}

QString RobloxSettings::getVersionString()
{
    return QString(VERSION_FULL_STR);
}


void RobloxSettings::initWorkspaceSettings()
{
    RbxWorkspace::isScriptAssetUploadEnabled = true;
    RbxWorkspace::isAnimationAssetUploadEnabled = true;
    RbxWorkspace::isImageModelAssetUploadEnabled = true;
}

bool RobloxSettings::showCrashMenu()
{
    return StudioAppSettings::instance().showCrashMenu();
}
