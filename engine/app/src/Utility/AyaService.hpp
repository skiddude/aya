#pragma once

#include "DataModel/GameBasicSettings.hpp"
#include "Tree/Service.hpp"
#include "DataModel/DataModel.hpp"
#include "DataModel/Stats.hpp"
#include "Reflection/Event.hpp"
#include "Utility/IHasLocation.hpp"
#include "Statistics.hpp"
namespace Aya
{
extern const char* const sAyaService;

class AyaService
    : public DescribedCreatable<AyaService, Instance, sAyaService, Reflection::ClassDescriptor::PERSISTENT_HIDDEN>
    , public Service
{
    typedef DescribedCreatable<AyaService, Instance, sAyaService, Reflection::ClassDescriptor::PERSISTENT_HIDDEN> Super;

    std::string serverMotdPreview;
    std::string serverMotdContent;
    std::string serverName;
    std::string host;
    int hostPort;

protected:
    class PingMasterServerJob;
    shared_ptr<PingMasterServerJob> pingMasterServerJob;

public:
    AyaService();
    ~AyaService();

    static std::string masterServerAuthorization;
    static bool passwordProtected;

    bool shouldShowCorescripts = true;

    static Reflection::PropDescriptor<AyaService, std::string> prop_masterServerUrl;
    static std::string masterServerUrl;
    std::string getMasterServerUrl() const
    {
        return masterServerUrl;
    };

    static Reflection::PropDescriptor<AyaService, bool> prop_localServer;
    static bool localServer;
    bool getLocalServer() const
    {
        return localServer;
    }

    static Reflection::PropDescriptor<AyaService, std::string> prop_serverMotdPreview;
    std::string getServerMotdPreview() const
    {
        return serverMotdPreview;
    }
    void setServerMotdPreview(std::string content)
    {
        serverMotdPreview = content;
    }

    static Reflection::PropDescriptor<AyaService, std::string> prop_serverMotdContent;
    std::string getServerMotdContent() const
    {
        return serverMotdContent;
    }
    void setServerMotdContent(std::string content)
    {
        serverMotdContent = content;
    }

    static Reflection::PropDescriptor<AyaService, std::string> prop_serverName;
    std::string getServerName() const
    {
        return serverName;
    }
    void setServerName(std::string content)
    {
        serverName = content;
    }

    bool isUsingInstance()
    {
        return IsUsingInstance();
    }

    static Reflection::PropDescriptor<AyaService, std::string> prop_host;
    std::string getHost() const
    {
        return host;
    }
    void setHost(std::string content)
    {
        host = content;
    }

    GameBasicSettings::VirtualVersion initialVersion = GameBasicSettings::VirtualVersion::VERSION_2016;
    void setInitialVersion(GameBasicSettings::VirtualVersion version)
    {
        initialVersion = version;
    };
    GameBasicSettings::VirtualVersion getInitialVersion()
    {
        return initialVersion;
    }

    void setHostPort(int port)
    {
        hostPort = port;
    } // because we can't access Server from here...
    int getHostPort()
    {
        return hostPort;
    }

    void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

    void announceMasterServer();
};
} // namespace Aya
