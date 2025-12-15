#pragma once
#include <QSettings>
#include <string>
#include <optional>

class AppSettings
{
public:
    AppSettings(const std::string& appDir);
    virtual ~AppSettings();

    bool load();
    std::optional<std::string> get(const std::string& group, const std::string& key) const;
    bool hasGroup(const std::string& group);
    bool has(const std::string& group, const std::string& key);
    void set(const std::string& group, const std::string& key, const std::string& value);

protected:
    QSettings* m_pSettings;
    std::string appDir;
};