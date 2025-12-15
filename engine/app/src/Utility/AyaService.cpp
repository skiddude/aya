#include "AyaService.hpp"
#include "DataModel/GameBasicSettings.hpp"
#include "Players.hpp"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace Aya
{
std::string AyaService::masterServerAuthorization = "";
std::string AyaService::masterServerUrl = "";
bool AyaService::localServer = false;
bool AyaService::passwordProtected = false;

const char* const sAyaService = "AyaService";

Reflection::PropDescriptor<AyaService, std::string> AyaService::prop_masterServerUrl(
    "MasterServerUrl", category_Data, &AyaService::getMasterServerUrl, NULL, Reflection::PropertyDescriptor::STANDARD_NO_REPLICATE);
Reflection::PropDescriptor<AyaService, bool> AyaService::prop_localServer(
    "LocalServer", category_Data, &AyaService::getLocalServer, NULL, Reflection::PropertyDescriptor::STANDARD_NO_REPLICATE);
Reflection::PropDescriptor<AyaService, std::string> AyaService::prop_serverMotdPreview("ServerMotdPreview", category_Data,
    &AyaService::getServerMotdPreview, &AyaService::setServerMotdPreview, Reflection::PropertyDescriptor::STANDARD_NO_REPLICATE);
Reflection::PropDescriptor<AyaService, std::string> AyaService::prop_serverMotdContent("ServerMotdContent", category_Data,
    &AyaService::getServerMotdContent, &AyaService::setServerMotdContent, Reflection::PropertyDescriptor::STANDARD_NO_REPLICATE);
Reflection::PropDescriptor<AyaService, std::string> AyaService::prop_serverName(
    "ServerName", category_Data, &AyaService::getServerName, &AyaService::setServerName, Reflection::PropertyDescriptor::STANDARD_NO_REPLICATE);
Reflection::PropDescriptor<AyaService, std::string> AyaService::prop_host(
    "Host", category_Data, &AyaService::getHost, &AyaService::setHost, Reflection::PropertyDescriptor::STANDARD_NO_REPLICATE);
static Reflection::BoundFuncDesc<AyaService, void()> func_announce(&AyaService::announceMasterServer, "AnnounceMasterServer", Security::Roblox);
static Reflection::BoundFuncDesc<AyaService, bool(void)> func_isUsingInstance(&AyaService::isUsingInstance, "IsUsingInstance", Security::None);

AyaService::AyaService()
{
    serverName = "Aya Server";
    serverMotdPreview = "MOTD not set";
    serverMotdContent = "Please tell the server administrator to set a Message of the Day.";
    host = "";
    hostPort = -1;

    setName(sAyaService);
}

AyaService::~AyaService() {}

void AyaService::announceMasterServer()
{
    Network::Players* players = ServiceProvider::create<Network::Players>(this);
    if (players->backendProcessing(this))
    {
        std::string jsonRequest;
        rapidjson::Document request;
        request.SetObject();

        DataModel* dm = DataModel::get(this);
        Network::Players* players = ServiceProvider::create<Network::Players>(dm);
        rapidjson::Document::AllocatorType& allocator = request.GetAllocator();
        request.AddMember("PlayerCount", players->getNumPlayers(), allocator);
        request.AddMember("PlayerLimit", players->getMaxPlayers(), allocator);
        std::string motdPreview = getServerMotdPreview();
        std::string motdContent = getServerMotdContent();
        std::string serverName = getServerName();
        std::string host = getHost();
        rapidjson::Value mp_value(motdPreview.c_str(), motdPreview.size(), allocator);
        request.AddMember("MotdPreview", mp_value, allocator);
        rapidjson::Value mc_value(motdContent.c_str(), motdContent.size(), allocator);
        request.AddMember("MotdContent", mc_value, allocator);
        rapidjson::Value au_value(masterServerAuthorization.c_str(), masterServerAuthorization.size(), allocator);
        request.AddMember("Authorization", au_value, allocator);
        rapidjson::Value sn_value(serverName.c_str(), serverName.size(), allocator);
        request.AddMember("ServerName", sn_value, allocator);
        rapidjson::Value hs_value(host.c_str(), host.size(), allocator);
        request.AddMember("Host", hs_value, allocator);
        request.AddMember("CustomPassword", passwordProtected, allocator);
        request.AddMember("ServerPort", getHostPort(), allocator);
        request.AddMember("VirtualVersion", getInitialVersion(), allocator);

        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        request.Accept(writer);
        jsonRequest = sb.GetString();

        if (masterServerUrl != "")
        {
            Security::Impersonator impersonate(Aya::Security::COM);

            dm->httpPost(masterServerUrl + "/announce", jsonRequest.c_str(), false, "application/json");

            StandardOut::singleton()->printf(MESSAGE_INFO, "sending json:  %s", jsonRequest.c_str());
            StandardOut::singleton()->printf(MESSAGE_INFO, "Sent announce message to %s", masterServerUrl.c_str());
        }
        else
        {
            StandardOut::singleton()->printf(MESSAGE_ERROR, "No master server url is set. Will not be pinging");
        }
    }
}

class AyaService::PingMasterServerJob : public DataModelJob
{
    AyaService* ayaService;
    double desiredHz;

public:
    PingMasterServerJob(AyaService* owner)
        : DataModelJob("PingMasterServerJob", DataModelJob::Write, false, shared_from(DataModel::get(owner)), Time::Interval(30.0))
    {
        desiredHz = 1.0 / 30.0;
        ayaService = owner;
    };

    /*override*/ Time::Interval sleepTime(const Stats& stats)
    {
        return computeStandardSleepTime(stats, desiredHz);
    }

    /*override*/ Job::Error error(const Stats& stats)
    {
        return computeStandardError(stats, desiredHz);
    }

    TaskScheduler::StepResult stepDataModelJob(const Stats& stats)
    {
        if (ayaService->getLocalServer())
        {
            ayaService->announceMasterServer();
        }
        return TaskScheduler::Stepped;
    }
};

void AyaService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
    if (oldProvider)
    {
        TaskScheduler::singleton().remove(pingMasterServerJob);
        pingMasterServerJob.reset();
    }

    Super::onServiceProvider(oldProvider, newProvider);

    if (newProvider)
    {
        pingMasterServerJob = shared_ptr<PingMasterServerJob>(new PingMasterServerJob(this));
        TaskScheduler::singleton().add(pingMasterServerJob);
    }
}
} // namespace Aya
