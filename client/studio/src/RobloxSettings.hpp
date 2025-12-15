

#pragma once
#include "AppSettings.hpp"
#include "DataModel/ContentProvider.hpp"
#include "Utility/Statistics.hpp"
// Qt Headers
#include <QSettings>

class StudioAppSettings
{
public:
    static StudioAppSettings& instance();

    QString contentFolder()
    {
        return QString::fromStdString(GetAssetFolder());
    }
    QString builtInPluginsFolder()
    {
        return m_builtInPluginsFolder;
    }
    QString baseURL()
    {
        return QString::fromStdString(GetBaseURL());
    }
    QString tempLocation()
    {
        return m_tempLocation;
    }
    bool isScriptAssetUploadEnabled()
    {
        return settings->get("Studio", "ScriptAssetUploadEnabled").value_or("false") == "true";
    }
    bool isAnimationAssetUploadEnabled()
    {
        return settings->get("Studio", "AnimationAssetUploadEnabled").value_or("false") == "true";
    }
    bool isImageModelAssetUploadEnabled()
    {
        return settings->get("Studio", "ImageModelAssetUploadEnabled").value_or("false") == "true";
    }
    bool showCrashMenu()
    {
        return settings->get("Studio", "ShowStudioCrashMenu").value_or("false") == "true";
    }

private:
    StudioAppSettings();

    AppSettings* settings;

    QString m_contentFolder;
    QString m_builtInPluginsFolder;
    QString m_baseURL;
    QString m_tempLocation;
    bool m_bIsScriptAssetUploadEnabled;
    bool m_bIsAnimationAssetUploadEnabled;
    bool m_bIsImageModelAssetUploadEnabled;
    bool m_bShowStudioCrashMenu;
};

class RobloxSettings : public QSettings
{
    Q_OBJECT

public:
    RobloxSettings();
    static void saveAssets();
    static void recoverAssets();
    static QString getAssetFolder();
    static QString getBuiltInPluginsFolder();
    static QString getBaseURL();
    static QString getApiBaseURL();
    static QString getVersionString();
    static QString getTempLocation();
    static void initWorkspaceSettings();
    static bool showCrashMenu();

private:
    RobloxSettings(const RobloxSettings&);            // Prevent copy-construction
    RobloxSettings& operator=(const RobloxSettings&); // Prevent assignment
};
