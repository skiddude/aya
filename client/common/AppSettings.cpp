#include "AppSettings.hpp"
#include <QString>

#ifndef SKIP_APP_SETTINGS_LOADING
#include "Utility/StandardOut.hpp"
#include "Utility/Statistics.hpp"
#endif // SKIP_APP_SETTINGS_LOADING

#include <boost/algorithm/string.hpp>
#include <filesystem>

static const char* kAppSettingsFileName = "AppSettings.ini";

namespace fs = std::filesystem;

AppSettings::AppSettings(const std::string& appDir)
    : m_pSettings(nullptr)
{
    this->appDir = appDir;
}

AppSettings::~AppSettings()
{
    delete m_pSettings;
}

bool AppSettings::load()
{
    if (appDir.empty())
        return false;

    delete m_pSettings;

    std::string path = appDir + "/" + kAppSettingsFileName;

#ifndef SKIP_APP_SETTINGS_LOADING
    Aya::StandardOut::singleton()->printf(Aya::MESSAGE_SYSTEM, "Loading AppSettings from %s", path.c_str());
#endif

    m_pSettings = new QSettings(QString::fromStdString(appDir + "/" + kAppSettingsFileName), QSettings::IniFormat);

    bool success = m_pSettings->status() == QSettings::NoError;

    if (success)
    {
        m_pSettings->sync();
        m_pSettings->setFallbacksEnabled(true);

        // hack for bootstrapper who can't deal with allat
#ifndef SKIP_APP_SETTINGS_LOADING
        std::string assetFolder = get("Aya", "ContentFolder").value_or(appDir + "/content");
        SetAssetFolder(assetFolder);
        SetTrustCheckURL(get("Aya", "TrustCheckUrl").value_or(""));

        if (GetTrustCheckURL().empty())
            SetUsingTrustCheck(false);

        if (has("Aya", "InsecureMode"))
        {
            std::string insecureMode = get("Aya", "InsecureMode").value_or("false");
            boost::algorithm::to_lower(insecureMode);
            SetInsecureMode(insecureMode == "true" || insecureMode == "1" || insecureMode == "yes");
        }

        if (has("Aya", "VerboseLogging"))
        {
            std::string verboseLogging = get("Aya", "VerboseLogging").value_or("false");
            boost::algorithm::to_lower(verboseLogging);
            SetVerboseLogging(verboseLogging == "true" || verboseLogging == "1" || verboseLogging == "yes");
        }

        if (hasGroup("Instance"))
        {
            SetBaseURL(get("Instance", "BaseUrl").value_or(""));
            SetInstanceAccessKey(get("Instance", "AccessKey").value_or(""));
        }

        if (GetBaseURL().empty())
        {
            SetUsingInstance(false);
            SetFetchLocalClientSettings(true);
        }
        else
        {
            SetUsingInstance(true);
            SetFetchLocalClientSettings(false);
        }

        if (hasGroup("MasterServer"))
        {
            SetMasterServerURL(get("MasterServer", "BaseUrl").value_or(""));
            SetMasterServerKey(get("MasterServer", "AccessKey").value_or(""));
        }

        SetUsingMasterServer(!GetMasterServerURL().empty());
#endif // SKIP_APP_SETTINGS_LOADING
    }

    return success;
}

std::optional<std::string> AppSettings::get(const std::string& group, const std::string& key) const
{
    m_pSettings->beginGroup(QString::fromStdString(group));
    QVariant value = m_pSettings->value(QString::fromStdString(key));
    m_pSettings->endGroup();

    if (value.isValid() && value.type() == QVariant::String)
        return value.toString().toStdString();

    return std::nullopt;
}

bool AppSettings::hasGroup(const std::string& group)
{
    return m_pSettings->childGroups().contains(QString::fromStdString(group));
}

bool AppSettings::has(const std::string& group, const std::string& key)
{
    if (!this->hasGroup(group))
        return false;

    m_pSettings->beginGroup(QString::fromStdString(group));
    QVariant value = m_pSettings->value(QString::fromStdString(key));
    m_pSettings->endGroup();

    return value.isValid();
}

void AppSettings::set(const std::string& group, const std::string& key, const std::string& value)
{
    m_pSettings->beginGroup(QString::fromStdString(group));
    m_pSettings->setValue(QString::fromStdString(key), QString::fromStdString(value));
    m_pSettings->endGroup();
}