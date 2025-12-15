// d9mz - This file makes me really sad

#if defined(_WIN32) || defined(_WIN64)
#include <atlsync.h>
#endif
#include "rapidjson/document.h"
#include "generated/soapRCCServiceSoapService.h"
#include "ProcessPerfCounter.hpp"

#include "Debug.hpp"

#include "Utility/ProtectedString.hpp"

#include "Utility/StandardOut.hpp"

#include "boost/noncopyable.hpp"
#include "boost/thread/xtime.hpp"
#include "boost/foreach.hpp"

#include "DataModel/Workspace.hpp"

#include "DataModel/ContentProvider.hpp"

#include "DataModel/DataModel.hpp"

#include "DataModel/DebugSettings.hpp"

#include "DataModel/PhysicsSettings.hpp"

#include "DataModel/FastLogSettings.hpp"

#include "DataModel/GameBasicSettings.hpp"

#include "Script/LuaSettings.hpp"
#include "DataModel/GameSettings.hpp"

#include "DataModel/MarketplaceService.hpp"

#include "DataModel/AdService.hpp"

#include "DataModel/CSGDictionaryService.hpp"

#include "DataModel/NonReplicatedCSGDictionaryService.hpp"

#include "DataModel/Message.hpp"

#include "Utility/RobloxServicesTools.hpp"
#include "Script/ScriptContext.hpp"
#include "ThumbnailGenerator.hpp"
#include <string>
#include <sstream>
#include "DataModel/factoryregistration.hpp"

#include "Utility/Profiling.hpp"

#include "Utility/SoundService.hpp"

#include "Utility/Guid.hpp"

#include "Utility/Http.hpp"

#include "Utility/Statistics.hpp"

#include "Utility/rbxrandom.hpp"

#include "API.hpp"
#include "Utility/AyaService.hpp"

#include <queue>
#include "Base/ViewBase.hpp"
#include "Utility/FileSystem.hpp"

#include "Players.hpp"

#include "Xml/XmlSerializer.hpp"
#include "Xml/WebParser.hpp"
#include "Utility/RobloxServicesTools.hpp"
#include "Utility/Utilities.hpp"

#include "ChatFilter.hpp"

#include "WebChatFilter.hpp"

#include "DataModel/ContentProvider.hpp"

#include "CountersClient.hpp"

#include "SimpleJSON.hpp"
#include "AyaFormat.hpp"
#if !defined(__linux) && !defined(__APPLE__)
#include "VersionInfo.hpp"
#include "DumpErrorUploader.hpp"
#endif

#include "Utility/Analytics.hpp"

#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/algorithm/string.hpp>

#if !defined(__linux) && !defined(__APPLE__)
#include <strsafe.h>
#endif

long diagCount = 0;
long batchJobCount = 0;
long openJobCount = 0;
long closeJobCount = 0;
long helloWorldCount = 0;
long getVersionCount = 0;
long renewLeaseCount = 0;
long executeCount = 0;
long getExpirationCount = 0;
long getStatusCount = 0;
long getAllJobsCount = 0;
long closeExpiredJobsCount = 0;
long closeAllJobsCount = 0;

// #define DIAGNOSTICS

#ifdef DIAGNOSTICS
#define BEGIN_PRINT(func, msg) \
    static int func = 0; \
    int counter = func++; \
    Aya::StandardOut::singleton()->printf(Aya::MESSAGE_INFO, "Begin-%s%d\r\n", msg, counter)
#define END_PRINT(func, msg) Aya::StandardOut::singleton()->printf(Aya::MESSAGE_INFO, "End---%s%d\r\n", msg, counter)
#else
#define BEGIN_PRINT(func, msg)
#define END_PRINT(func, msg)
#endif

LOGVARIABLE(RCCServiceInit, 1);
LOGVARIABLE(RCCServiceJobs, 1);
LOGVARIABLE(RCCDataModelInit, 1);
// See TaskScheduler::ThreadPoolConfig enum for valid values
DYNAMIC_FASTINTVARIABLE(TaskSchedulerThreadCountEnum, 1)

DYNAMIC_FASTFLAGVARIABLE(DebugCrashOnFailToLoadClientSettings, false)

DYNAMIC_FASTFLAGVARIABLE(UseNewSecurityKeyApi, false);
DYNAMIC_FASTFLAGVARIABLE(UseNewMemHashApi, false);
DYNAMIC_FASTSTRINGVARIABLE(MemHashConfig, "");
FASTINTVARIABLE(RCCServiceThreadCount, Aya::TaskScheduler::Threads1)
DYNAMIC_FASTFLAGVARIABLE(US30476, false);
FASTFLAGVARIABLE(UseDataDomain, true);

FASTFLAGVARIABLE(Dep, true)

namespace Aya
{
class Explosion;
class Cofm;
class NormalBreakConnector;
class ContactConnector;
class RevoluteLink;
class SimBody;
class BallBallContact;
class BallBlockContact;
class BlockBlockContact;

} // namespace Aya

class CrashAfterTimeout
{
    shared_ptr<boost::condition_variable_any> event;
    shared_ptr<boost::mutex> mutex;

    static void run(shared_ptr<boost::condition_variable_any> event, int timeount, shared_ptr<boost::mutex> mutex)
    {
        boost::xtime xt;
        boost::xtime_get(&xt, boost::TIME_UTC_);
        xt.sec += static_cast<boost::xtime::xtime_sec_t>(timeount);
        boost::mutex::scoped_lock lock(*mutex);
        bool set = event->timed_wait(lock, xt);
        if (!set)
            AYACRASH();
    }

public:
    CrashAfterTimeout(int seconds)
        : event(new boost::condition_variable_any())
        , mutex(new boost::mutex())
    {
        boost::thread thread(boost::bind(&CrashAfterTimeout::run, event, seconds, mutex));
    }
    ~CrashAfterTimeout()
    {
        boost::mutex::scoped_lock lock(*mutex);
        event->notify_all();
    }
};

class RCCServiceSettings : public Aya::FastLogJSON
{
public:
    START_DATA_MAP(RCCServiceSettings);
    DECLARE_DATA_STRING(WindowsMD5);
    DECLARE_DATA_STRING(WindowsPlayerBetaMD5);
    DECLARE_DATA_STRING(MacMD5);
    DECLARE_DATA_INT(SecurityDataTimer);
    DECLARE_DATA_INT(ClientSettingsTimer);

    END_DATA_MAP();
};

class SecurityDataUpdater
{
    std::string apiUrl;

protected:
    std::string data;

    virtual void processDataArray(shared_ptr<const Aya::Reflection::ValueArray> dataArray) = 0;
    bool fetchData()
    {
        std::string newData;
        try
        {
            Aya::Http request(apiUrl);

            request.get(newData);

            // no need to continue if data did not change
            if (newData == data)
                return false;
        }
        catch (std::exception& ex)
        {
            Aya::StandardOut::singleton()->printf(
                Aya::MESSAGE_WARNING, "SecurityDataUpdater failed to fetch data from %s, %s", apiUrl.c_str(), ex.what());
            return false;
        }

        data = newData;
        return true;
    }

public:
    SecurityDataUpdater(const std::string& url)
        : apiUrl(url)
    {
    }
    virtual ~SecurityDataUpdater() {}

    void run()
    {
        if (!fetchData())
            return;

        shared_ptr<const Aya::Reflection::ValueTable> jsonResult(Aya::make_shared<const Aya::Reflection::ValueTable>());
        if (Aya::WebParser::parseJSONTable(data, jsonResult))
        {
            Aya::Reflection::ValueTable::const_iterator iter = jsonResult->find("data");
            if (iter != jsonResult->end())
            {
                shared_ptr<const Aya::Reflection::ValueArray> dataArray = iter->second.cast<shared_ptr<const Aya::Reflection::ValueArray>>();
                processDataArray(dataArray);
            }
        }
    }
};

class MD5Updater : public SecurityDataUpdater
{
protected:
    void processDataArray(shared_ptr<const Aya::Reflection::ValueArray> dataArray)
    {
        std::set<std::string> hashes;
        for (Aya::Reflection::ValueArray::const_iterator it = dataArray->begin(); it != dataArray->end(); ++it)
        {
            std::string value = it->get<std::string>();
            hashes.insert(value);
        }

        // always add hash for ios
        hashes.insert("ios,ios");

        Aya::Network::Players::setGoldenHashes2(hashes);
    }

public:
    MD5Updater(const std::string& url)
        : SecurityDataUpdater(url)
    {
    }
    ~MD5Updater() {}
};

class SecurityKeyUpdater : public SecurityDataUpdater
{
protected:
    void processDataArray(shared_ptr<const Aya::Reflection::ValueArray> dataArray)
    {
        std::vector<std::string> versions;
        for (Aya::Reflection::ValueArray::const_iterator it = dataArray->begin(); it != dataArray->end(); ++it)
        {
            // version = data + salt
            std::string value = it->get<std::string>();
            std::string version = Aya::sha1(value + "askljfLUZF");
            versions.push_back(version);
        }

        Aya::Network::setSecurityVersions(versions);
    }

public:
    SecurityKeyUpdater(const std::string& url)
        : SecurityDataUpdater(url)
    {
    }
    ~SecurityKeyUpdater() {}
};

class MemHashUpdater : public SecurityDataUpdater
{
public:
    // this is public until we get a web api to do this better.
    static void populateMemHashConfigs(Aya::Network::MemHashConfigs& hashConfigs, const std::string& cfgStr)
    {
        hashConfigs.push_back(Aya::Network::MemHashVector());
        Aya::Network::MemHashVector& thisHashVector = hashConfigs.back();

        std::vector<std::string> hashStrInfo;
        boost::split(hashStrInfo, cfgStr, boost::is_any_of(";"));
        for (std::vector<std::string>::const_iterator hpIt = hashStrInfo.begin(); hpIt != hashStrInfo.end(); ++hpIt)
        {
            std::vector<std::string> args;
            boost::split(args, *hpIt, boost::is_any_of(","));
            if (args.size() >= 3)
            {
                Aya::Network::MemHash thisHash;
                thisHash.checkIdx = boost::lexical_cast<unsigned int>(args[0]);
                thisHash.value = boost::lexical_cast<unsigned int>(args[1]);
                thisHash.failMask = boost::lexical_cast<unsigned int>(args[2]);
                thisHashVector.push_back(thisHash);
            }
        }
    };

protected:
    void processDataArray(shared_ptr<const Aya::Reflection::ValueArray> dataArray)
    {
        Aya::Network::MemHashConfigs hashConfigs;
        for (Aya::Reflection::ValueArray::const_iterator it = dataArray->begin(); it != dataArray->end(); ++it)
        {
            populateMemHashConfigs(hashConfigs, it->get<std::string>());
        }

        Aya::Network::Players::setGoldMemHashes(hashConfigs);
    }

public:
    MemHashUpdater(const std::string& url)
        : SecurityDataUpdater(url)
    {
    }
    ~MemHashUpdater() {}
};

class RCCServiceDynamicSettings : public Aya::FastLogJSON
{
    typedef std::map<std::string, std::string> FVariables;
    FVariables fVars;

public:
    virtual void ProcessVariable(const std::string& valueName, const std::string& valueData, bool dynamic)
    {
        AYAASSERT(dynamic);
        fVars.insert(std::make_pair(valueName, valueData));
    }

    virtual bool DefaultHandler(const std::string& valueName, const std::string& valueData)
    {
        // only process dynamic flags and logs
        if (valueName[0] == 'D')
            return FastLogJSON::DefaultHandler(valueName, valueData);

        return false;
    }

    void UpdateSettings()
    {
        for (FVariables::iterator i = fVars.begin(); i != fVars.end(); i++)
            FLog::SetValue(i->first, i->second, FASTVARTYPE_DYNAMIC, false);
        fVars.clear();
    }
};

DATA_MAP_IMPL_START(RCCServiceSettings)
IMPL_DATA(WindowsMD5, "");
IMPL_DATA(MacMD5, "");
IMPL_DATA(WindowsPlayerBetaMD5, "");
IMPL_DATA(SecurityDataTimer, 300);   // in seconds, default 5 min
IMPL_DATA(ClientSettingsTimer, 120); // 2 min

IMPL_DATA(HttpUseCurlPercentageRCC, 0); // do not use CURL by default
DATA_MAP_IMPL_END()

#if !defined(__linux) && !defined(__APPLE__)
static boost::scoped_ptr<MainLogManager> mainLogManager(new MainLogManager("Kiseki Web Service", ".dmp", ".crashevent"));
#endif

#ifdef AYA_TEST_BUILD
std::string RCCServiceSettingsKeyOverwrite;
#endif

class CWebService
{
#if !defined(__linux) && !defined(__APPLE__)
    boost::shared_ptr<CProcessPerfCounter> s_perfCounter;
    ATL::CEvent doneEvent;
#endif
    boost::scoped_ptr<boost::thread> perfData;
    boost::scoped_ptr<boost::thread> fetchSecurityDataThread;
    boost::scoped_ptr<boost::thread> fetchClientSettingsThread;
    RCCServiceSettings rccSettings;
    RCCServiceDynamicSettings rccDynamicSettings;
    std::string settingsKey;
    std::string securityVersionData;
    std::string clientSettingsData;

    std::string thumbnailSettingsData;
    bool isThumbnailer;

    boost::scoped_ptr<MD5Updater> md5Updater;
    boost::scoped_ptr<SecurityDataUpdater> securityKeyUpdater;
    boost::scoped_ptr<MemHashUpdater> memHashUpdater;

    boost::scoped_ptr<CountersClient> counters;

public:
    struct JobItem : boost::noncopyable
    {
        enum JobItemRunStatus
        {
            RUNNING_JOB,
            JOB_DONE,
            JOB_ERROR
        };
        const std::string id;
        shared_ptr<Aya::DataModel> dataModel;
        Aya::Time expirationTime;
        int category;
        double cores;
#if !defined(__linux) && !defined(__APPLE__)
        ATL::CEvent jobCheckLeaseEvent;
#endif
        Aya::signals::connection notifyAliveConnection;

        JobItemRunStatus status;
        std::string errorMessage;

        JobItem(const char* id)
            : id(id)
#if !defined(__linux) && !defined(__APPLE__)
            , jobCheckLeaseEvent(TRUE, FALSE)
#endif
            , status(RUNNING_JOB)
        {
        }

        void touch(double seconds);
        double secondsToTimeout() const;
    };

    typedef std::map<std::string, boost::shared_ptr<JobItem>> JobMap;
    JobMap jobs;
    boost::mutex sync;
    long dataModelCount;
    boost::mutex currentlyClosingMutex;

public:
    CWebService(bool crashUploadOnly);
    ~CWebService();

private:
    void collectPerfData();
    void LoadAppSettings();
    void LoadClientSettings(RCCServiceSettings& dest);
    void LoadClientSettings(std::string& clientDest, std::string& thumbnailDest);
    std::string GetSettingsKey();

    std::vector<std::string> fetchAllowedSecurityVersions();

public:
    static boost::scoped_ptr<CWebService> singleton;
    JobMap::iterator getJob(const std::string& jobID)
    {
        JobMap::iterator iter = jobs.find(jobID);
        if (iter == jobs.end())
            throw Aya::runtime_error("JobItem %s not found", jobID.c_str());
        return iter;
    }

    void validateSecurityData()
    {
#if defined(__linux) || defined(__APPLE__)
        std::mutex mtx;
        std::condition_variable cv;

        std::unique_lock<std::mutex> lock(mtx);

        while (cv.wait_for(lock, std::chrono::milliseconds(static_cast<int>(rccSettings.GetValueSecurityDataTimer() * 1000.0))) ==
               std::cv_status::timeout)
#else
        while (::WaitForSingleObject(doneEvent.m_h, rccSettings.GetValueSecurityDataTimer() * 1000) == WAIT_TIMEOUT)
#endif
        {
            if (DFFlag::UseNewSecurityKeyApi)
            {
                if (securityKeyUpdater)
                    securityKeyUpdater->run();
            }
            else
            {
                std::string prevVersionData = securityVersionData;
                std::vector<std::string> versions = fetchAllowedSecurityVersions();

                // security versions changed, update all jobs with new versions
                if (prevVersionData != securityVersionData)
                    Aya::Network::setSecurityVersions(versions);
            }

            if (DFFlag::UseNewMemHashApi)
            {
                if (memHashUpdater)
                {
                    if (DFString::MemHashConfig.size() < 3)
                    {
                        memHashUpdater->run();
                    }
                    else
                    {
                        Aya::Network::MemHashConfigs hashConfigs;
                        MemHashUpdater::populateMemHashConfigs(hashConfigs, DFString::MemHashConfig);
                        Aya::Network::Players::setGoldMemHashes(hashConfigs);
                    }
                }
            }

            if (md5Updater)
                md5Updater->run();
        }
    }

    void validateClientSettings()
    {

#if defined(__linux) || defined(__APPLE__)
        std::mutex mtx;
        std::condition_variable cv;

        std::unique_lock<std::mutex> lock(mtx);

        while (cv.wait_for(lock, std::chrono::milliseconds(static_cast<int>(rccSettings.GetValueClientSettingsTimer() * 1000.0))) ==
               std::cv_status::timeout)
#else
        while (::WaitForSingleObject(doneEvent.m_h, rccSettings.GetValueClientSettingsTimer() * 1000) == WAIT_TIMEOUT)
#endif
        {
            std::string prevClientSettings = clientSettingsData;
            std::string prevThumbnailSettings = thumbnailSettingsData;

            LoadClientSettings(clientSettingsData, thumbnailSettingsData);

            bool clientChanged = (prevClientSettings != clientSettingsData);
            bool thumbnailChanged = isThumbnailer && (prevThumbnailSettings != thumbnailSettingsData);
            if (clientChanged || thumbnailChanged)
            {
                // collect all dynamic settings
                rccDynamicSettings.ReadFromStream(clientSettingsData.c_str());
                if (isThumbnailer)
                {
                    rccDynamicSettings.ReadFromStream(thumbnailSettingsData.c_str());
                }

                // submit datamodel write task to update all dynamic settings
                boost::mutex::scoped_lock lock(sync);
                if (jobs.size() > 0)
                {
                    // HACK: Client settings are global, meaning all datamodels use the same set,
                    // current we only have 1 datamodel running per rccservice, so submit write task just on first datamodel
                    shared_ptr<JobItem> job = jobs.begin()->second;
                    job->dataModel->submitTask(
                        boost::bind(&RCCServiceDynamicSettings::UpdateSettings, &rccDynamicSettings), Aya::DataModelJob::Write);
                }
                else
                    rccDynamicSettings.UpdateSettings();
            }
        }
    }

    void doCheckLease(boost::shared_ptr<JobItem> job)
    {
        try
        {
#if defined(__linux) || defined(__APPLE__)
            std::mutex mtx;
            std::condition_variable cv;
#endif
            while (true)
            {
#if defined(__linux) || defined(__APPLE__)
                std::unique_lock<std::mutex> lock(mtx);
#endif
                double sleepTimeInSeconds = job->secondsToTimeout();
                if (sleepTimeInSeconds <= 0)
                {
                    closeJob(job->id);
                    return;
                }
#if !defined(__linux) && !defined(__APPLE__)
                else if (::WaitForSingleObject(job->jobCheckLeaseEvent.m_h, (DWORD)(sleepTimeInSeconds * 1000.0) + 3) != WAIT_TIMEOUT)
                    return;
#else
                else if (cv.wait_for(lock, std::chrono::milliseconds(static_cast<int>(sleepTimeInSeconds * 1000.0) + 3)) != std::cv_status::timeout)
                    return;
#endif
            }
        }
        catch (std::exception& e)
        {
            assert(false);
            Aya::StandardOut::singleton()->printf(Aya::MESSAGE_ERROR, "doCheckLease failure: %s", e.what());
        }
    }

    void renewLease(const std::string& id, double expirationInSeconds)
    {
        boost::mutex::scoped_lock lock(sync);
        getJob(id)->second->touch(expirationInSeconds);
    }

    static void convertToLua(const Aya::Reflection::Variant& source, ns1__LuaValue& dest, soap* soap)
    {
        assert(dest.type == ns1__LuaType__LUA_USCORETNIL);
        assert(dest.value == NULL);

        if (source.isType<void>())
        {
            dest.type = ns1__LuaType__LUA_USCORETNIL;
            return;
        }

        if (source.isType<bool>())
        {
            dest.type = ns1__LuaType__LUA_USCORETBOOLEAN;
            dest.value = soap_new_std__string(soap, -1);
            *dest.value = source.get<std::string>();
            return;
        }

        if (source.isNumber())
        {
            dest.type = ns1__LuaType__LUA_USCORETNUMBER;
            dest.value = soap_new_std__string(soap, -1);
            *dest.value = source.get<std::string>();
            return;
        }

        if (source.isType<std::string>())
        {
            dest.type = ns1__LuaType__LUA_USCORETSTRING;
            dest.value = soap_new_std__string(soap, -1);
            *dest.value = source.get<std::string>();
            return;
        }

        if (source.isType<Aya::ContentId>())
        {
            dest.type = ns1__LuaType__LUA_USCORETSTRING;
            dest.value = soap_new_std__string(soap, -1);
            *dest.value = source.get<std::string>();
            return;
        }

        if (source.isType<shared_ptr<const Aya::Reflection::ValueArray>>())
        {
            shared_ptr<const Aya::Reflection::ValueArray> collection = source.cast<shared_ptr<const Aya::Reflection::ValueArray>>();
            dest.type = ns1__LuaType__LUA_USCORETTABLE;
            dest.table = soap_new_ns1__ArrayOfLuaValue(soap, -1);
            dest.table->LuaValue.resize(collection->size());
            for (size_t i = 0; i < collection->size(); ++i)
            {
                dest.table->LuaValue[i] = soap_new_ns1__LuaValue(soap, -1);
                convertToLua((*collection)[i], *dest.table->LuaValue[i], soap);
            }
            return;
        }

        // TODO: enums!
        dest.type = ns1__LuaType__LUA_USCORETNIL;
    }

    static void convert(const ns1__LuaValue* source, Aya::Reflection::Variant& dest)
    {
        switch (source->type)
        {
        case ns1__LuaType__LUA_USCORETNIL:
            break;

        case ns1__LuaType__LUA_USCORETNUMBER:
            dest = *source->value;
            dest.convert<double>();
            break;

        case ns1__LuaType__LUA_USCORETBOOLEAN:
            dest = *source->value;
            dest.convert<bool>();
            break;

        case ns1__LuaType__LUA_USCORETSTRING:
            dest = *source->value;
            break;

        case ns1__LuaType__LUA_USCORETTABLE:
        {
            const size_t count = source->table->LuaValue.size();

            // Create a collection so that we can populate it
            shared_ptr<Aya::Reflection::ValueArray> table(new Aya::Reflection::ValueArray(count));

            for (size_t i = 0; i < count; ++i)
                convert(source->table->LuaValue[i], (*table)[i]);

            // Set the value to a ValueArray type
            dest = shared_ptr<const Aya::Reflection::ValueArray>(table);
        }
        break;
        }
    }

    // Gathers all non-global Instances that are in the heap
    void arbiterActivityDump(Aya::Reflection::ValueArray& result)
    {
        boost::mutex::scoped_lock lock(sync);
        for (JobMap::iterator iter = jobs.begin(); iter != jobs.end(); ++iter)
        {
            shared_ptr<Aya::DataModel> dataModel = iter->second->dataModel;
            shared_ptr<Aya::Reflection::ValueArray> tuple(new Aya::Reflection::ValueArray());
            tuple->push_back(dataModel->arbiterName());
            tuple->push_back(dataModel->getAverageActivity());
            result.push_back(shared_ptr<const Aya::Reflection::ValueArray>(tuple));
        }
    }

    // Gathers all non-global Instances that are in the heap
    void leakDump(Aya::Reflection::ValueArray& result) {}

    // Gathers diagnostic data. Does NOT require locks on datamodel :)
    const Aya::Reflection::ValueArray* diag(int type, shared_ptr<Aya::DataModel> dataModel)
    {
        std::auto_ptr<Aya::Reflection::ValueArray> tuple(new Aya::Reflection::ValueArray());

        /* This is the format of the Diag data:

            type == 0
                DataModel Count in this process
                PerfCounter data
                Task Scheduler
                    (obsolete entry)
                    double threadAffinity
                    double numQueuedJobs
                    double numScheduledJobs
                    double numRunningJobs
                    long threadPoolSize
                    double messageRate
                    double messagePumpDutyCycle
                DataModel Jobs Info
                Machine configuration
                Memory Leak Detection
            type & 1
                leak dump
            type & 2
                attempt to allocate 500k. if success, then true else false
            type & 4
                DataModel dutyCycles
        */
        tuple->push_back(dataModelCount);

        {
// preferably we should fix this
#if !defined(__linux) && !defined(__APPLE__)
            shared_ptr<Aya::Reflection::ValueArray> perfCounterData(new Aya::Reflection::ValueArray());
            perfCounterData->push_back(s_perfCounter->GetProcessCores());
            perfCounterData->push_back(s_perfCounter->GetTotalProcessorTime());
            perfCounterData->push_back(s_perfCounter->GetProcessorTime());
            perfCounterData->push_back(s_perfCounter->GetPrivateBytes());
            perfCounterData->push_back(s_perfCounter->GetPrivateWorkingSetBytes());
            perfCounterData->push_back(-1);
            perfCounterData->push_back(-1);
            perfCounterData->push_back(s_perfCounter->GetElapsedTime());
            perfCounterData->push_back(s_perfCounter->GetVirtualBytes());
            perfCounterData->push_back(s_perfCounter->GetPageFileBytes());
            perfCounterData->push_back(s_perfCounter->GetPageFaultsPerSecond());

            tuple->push_back(shared_ptr<const Aya::Reflection::ValueArray>(perfCounterData));
#endif
        }
        {
            shared_ptr<Aya::Reflection::ValueArray> taskSchedulerData(new Aya::Reflection::ValueArray());
            taskSchedulerData->push_back(0); // obsolete
            taskSchedulerData->push_back(Aya::TaskScheduler::singleton().threadAffinity());
            taskSchedulerData->push_back(Aya::TaskScheduler::singleton().numSleepingJobs());
            taskSchedulerData->push_back(Aya::TaskScheduler::singleton().numWaitingJobs());
            taskSchedulerData->push_back(Aya::TaskScheduler::singleton().numRunningJobs());
            taskSchedulerData->push_back((long)Aya::TaskScheduler::singleton().threadPoolSize());
            taskSchedulerData->push_back(Aya::TaskScheduler::singleton().schedulerRate());
            taskSchedulerData->push_back(Aya::TaskScheduler::singleton().getSchedulerDutyCyclePerThread());
            tuple->push_back(shared_ptr<const Aya::Reflection::ValueArray>(taskSchedulerData));
        }

        if (dataModel)
            tuple->push_back(dataModel->getJobsInfo());
        else
            tuple->push_back(0);

        {
            shared_ptr<Aya::Reflection::ValueArray> machineData(new Aya::Reflection::ValueArray());
            machineData->push_back(0);
            machineData->push_back(0);
            machineData->push_back(0);
            machineData->push_back(0);
            tuple->push_back(shared_ptr<const Aya::Reflection::ValueArray>(machineData));
        }

        {
            // TODO, use Aya::poolAllocationList and Aya::poolAvailablityList to get complete stats
            shared_ptr<Aya::Reflection::ValueArray> memCounters(new Aya::Reflection::ValueArray());
            memCounters->push_back(Aya::Diagnostics::Countable<Aya::Instance>::getCount());
            memCounters->push_back(Aya::Diagnostics::Countable<Aya::PartInstance>::getCount());
            memCounters->push_back(Aya::Diagnostics::Countable<Aya::ModelInstance>::getCount());
            memCounters->push_back(Aya::Diagnostics::Countable<Aya::Explosion>::getCount());
            memCounters->push_back(Aya::Diagnostics::Countable<Aya::Soundscape::SoundChannel>::getCount());
            memCounters->push_back(Aya::Diagnostics::Countable<Aya::signals::connection>::getCount());
            memCounters->push_back(Aya::Diagnostics::Countable<Aya::signals::connection::islot>::getCount());
            memCounters->push_back(Aya::Diagnostics::Countable<Aya::Reflection::GenericSlotWrapper>::getCount());
            memCounters->push_back(Aya::Diagnostics::Countable<Aya::TaskScheduler::Job>::getCount());
            memCounters->push_back(Aya::Diagnostics::Countable<Aya::Network::Player>::getCount());
#ifdef AYA_ALLOCATOR_COUNTS
            memCounters->push_back(Aya::Allocator<Aya::Body>::getCount());
            memCounters->push_back(Aya::Allocator<Aya::Cofm>::getCount());
            memCounters->push_back(Aya::Allocator<Aya::NormalBreakConnector>::getCount());
            memCounters->push_back(Aya::Allocator<Aya::ContactConnector>::getCount());
            memCounters->push_back(Aya::Allocator<Aya::RevoluteLink>::getCount());
            memCounters->push_back(Aya::Allocator<Aya::SimBody>::getCount());
            memCounters->push_back(Aya::Allocator<Aya::BallBallContact>::getCount());
            memCounters->push_back(Aya::Allocator<Aya::BallBlockContact>::getCount());
            memCounters->push_back(Aya::Allocator<Aya::BlockBlockContact>::getCount());
            memCounters->push_back(Aya::Allocator<XmlAttribute>::getCount());
            memCounters->push_back(Aya::Allocator<XmlElement>::getCount());
#else
            for (int i = 0; i < 11; ++i)
                memCounters->push_back(-1);
#endif
            memCounters->push_back((int)ThumbnailGenerator::totalCount);
            tuple->push_back(shared_ptr<const Aya::Reflection::ValueArray>(memCounters));
        }

        if (type & 1)
        {
            shared_ptr<Aya::Reflection::ValueArray> result(new Aya::Reflection::ValueArray());
            leakDump(*result);
            tuple->push_back(shared_ptr<const Aya::Reflection::ValueArray>(result));
        }

        if (type & 2)
        {
            try
            {
                {
                    std::vector<char> v1(100000);
                    std::vector<char> v2(100000);
                    std::vector<char> v3(100000);
                    std::vector<char> v4(100000);
                    std::vector<char> v5(100000);
                }
                tuple->push_back(true);
            }
            catch (...)
            {
                tuple->push_back(false);
            }
        }

        if (type & 4)
        {
            shared_ptr<Aya::Reflection::ValueArray> result(new Aya::Reflection::ValueArray());
            arbiterActivityDump(*result);
            tuple->push_back(shared_ptr<const Aya::Reflection::ValueArray>(result));
        }

        return tuple.release();
    }

    void execute(const std::string& jobID, ns1__ScriptExecution* script, std::vector<ns1__LuaValue*>* result, soap* soap)
    {
        std::string code = *script->script;
        if (Aya::ContentProvider::isHttpUrl(code))
        {
            Aya::Http http(code);
            http.get(code);
        }

        shared_ptr<Aya::DataModel> dataModel;
        {
            boost::mutex::scoped_lock lock(sync);
            JobMap::iterator iter = getJob(jobID);
            dataModel = iter->second->dataModel;
        }

        std::auto_ptr<const Aya::Reflection::Tuple> tuple;
        {
            const size_t count = script->arguments ? script->arguments->LuaValue.size() : 0;
            Aya::Reflection::Tuple args(count);
            for (size_t i = 0; i < count; ++i)
                convert(script->arguments->LuaValue[i], args.values[i]);

            Aya::DataModel::LegacyLock lock(dataModel, Aya::DataModelJob::Write);
            if (dataModel->isClosed())
                throw std::runtime_error("The DataModel is closed");
            Aya::ScriptContext* scriptContext = Aya::ServiceProvider::create<Aya::ScriptContext>(dataModel.get());
            tuple = scriptContext->executeInNewThread(
                Aya::Security::WebService, Aya::ProtectedString::fromTrustedSource(code), script->name->c_str(), args);
        }

        if (tuple.get())
        {
            result->resize(tuple->values.size());

            for (size_t i = 0; i < tuple->values.size(); ++i)
            {
                (*result)[i] = soap_new_ns1__LuaValue(soap, -1);
                convertToLua(tuple->values[i], *(*result)[i], soap);
            }
        }
        else
        {
            result->resize(0);
        }
    }

    void diag(int type, std::string jobID, std::vector<ns1__LuaValue*>* result, soap* soap)
    {
        std::auto_ptr<const Aya::Reflection::ValueArray> tuple;

        {
            shared_ptr<Aya::DataModel> dataModel;
            if (!jobID.empty())
            {
                boost::mutex::scoped_lock lock(sync);
                JobMap::iterator iter = getJob(jobID);
                dataModel = iter->second->dataModel;
            }
            tuple.reset(diag(type, dataModel));
        }

        result->resize(tuple->size());
        for (size_t i = 0; i < tuple->size(); ++i)
        {
            (*result)[i] = soap_new_ns1__LuaValue(soap, -1);
            convertToLua((*tuple)[i], *(*result)[i], soap);
        }
    }

    void contentDataLoaded(shared_ptr<Aya::DataModel>& dataModel)
    {
        Aya::CSGDictionaryService* dictionaryService = Aya::ServiceProvider::create<Aya::CSGDictionaryService>(dataModel.get());
        Aya::NonReplicatedCSGDictionaryService* nrDictionaryService =
            Aya::ServiceProvider::create<Aya::NonReplicatedCSGDictionaryService>(dataModel.get());

        dictionaryService->reparentAllChildData();
    }

    void setupServerConnections(Aya::DataModel* dataModel)
    {
        if (!dataModel)
        {
            return;
        }
    }

    shared_ptr<JobItem> createJob(const ns1__Job& job, bool startHeartbeat, shared_ptr<Aya::DataModel>& dataModel)
    {
        srand(Aya::randomSeed()); // make sure this thread is seeded
        std::string id = job.id;

        dataModel = Aya::DataModel::createDataModel(
            startHeartbeat, new Aya::NullVerb(NULL, ""), false, Aya::GameBasicSettings::singleton().getVirtualVersion());
        Aya::AyaService* ayaService = Aya::ServiceProvider::create<Aya::AyaService>(dataModel.get());
        ayaService->setInitialVersion(Aya::GameBasicSettings::singleton().getVirtualVersion());

        setupServerConnections(dataModel.get());

        Aya::Network::Players* players = dataModel->find<Aya::Network::Players>();
        if (players)
        {
            LoadClientSettings(rccSettings);
            if (rccSettings.GetError())
            {
                AYACRASH(rccSettings.GetErrorString().c_str());
            }

            {
                bool useCurl = rand() % 100 < rccSettings.GetValueHttpUseCurlPercentageRCC();
                FASTLOG1(FLog::RCCDataModelInit, "Using CURL = %d", useCurl);
                Aya::Http::SetUseCurl(useCurl);

                Aya::Http::SetUseStatistics(true);
            }

            FASTLOGS(FLog::RCCDataModelInit, "Creating Data Model, Windows MD5: %s", rccSettings.GetValueWindowsMD5());
            FASTLOGS(FLog::RCCDataModelInit, "Creating Data Model, Mac MD5: %s", rccSettings.GetValueMacMD5());
            FASTLOGS(FLog::RCCDataModelInit, "Creating Data Model, Windows Player Beta MD5: %s", rccSettings.GetValueWindowsPlayerBetaMD5());
            players->setGoldenHashes(rccSettings.GetValueWindowsMD5(), rccSettings.GetValueMacMD5(), rccSettings.GetValueWindowsPlayerBetaMD5());

            Aya::DataModel::LegacyLock lock(dataModel, Aya::DataModelJob::Write);
            dataModel->create<Aya::Network::WebChatFilter>();
        }

        dataModel->workspaceLoadedSignal.connect(
            [this](std::shared_ptr<Aya::DataModel>& dataModel)
            {
                if (Aya::AdService* adService = dataModel->create<Aya::AdService>())
                {
                    adService->sendServerVideoAdVerification.connect(boost::bind(&Aya::AdService::checkCanPlayVideoAd, adService, _1, _2));
                    adService->sendServerRecordImpression.connect(boost::bind(&Aya::AdService::sendAdImpression, adService, _1, _2, _3));
                }

                Aya::CSGDictionaryService* dictionaryService = Aya::ServiceProvider::create<Aya::CSGDictionaryService>(dataModel.get());
                Aya::NonReplicatedCSGDictionaryService* nrDictionaryService =
                    Aya::ServiceProvider::create<Aya::NonReplicatedCSGDictionaryService>(dataModel.get());

                dictionaryService->reparentAllChildData();
            });
        dataModel->jobId = id;
        shared_ptr<JobItem> j(new JobItem(id.c_str()));
        j->dataModel = dataModel;
        j->category = job.category;
        j->cores = job.cores;
        j->touch(job.expirationInSeconds);

        {
            boost::mutex::scoped_lock lock(sync);
            if (jobs.find(id) != jobs.end())
                throw Aya::runtime_error("JobItem %s already exists", id.c_str());
            jobs[id] = j;
        }

        return j;
    }

    void batchJob(const ns1__Job& job, ns1__ScriptExecution* script, std::vector<ns1__LuaValue*>* result, soap* soap)
    {
        std::string id = job.id;

        shared_ptr<Aya::DataModel> dataModel;
#if !defined(__linux) && !defined(__APPLE__)
        ::InterlockedIncrement(&dataModelCount);
#endif
        try
        {
            shared_ptr<JobItem> j = createJob(job, false, dataModel);

            FASTLOGS(FLog::RCCServiceJobs, "Opened Batch JobItem %s", id.c_str());
            FASTLOG1(FLog::RCCServiceJobs, "DataModel: %p", dataModel.get());

            // Spin off a thread for the BatchJob to execute in
#if !defined(__linux) && !defined(__APPLE__)
            boost::thread(Aya::thread_wrapper(boost::bind(&CWebService::asyncExecute, this, id, script, result, soap), "AsyncExecute"));
#else
            // linux - Object destroyed immediately after creation; did you mean to name the object? (fix available)clang-tidybugprone-unused-raii
            boost::thread give_me_a_name(
                Aya::thread_wrapper(boost::bind(&CWebService::asyncExecute, this, id, script, result, soap), "AsyncExecute"));

            std::mutex mtx;
            std::condition_variable cv;
#endif

            while (true)
            {
#if defined(__linux) || defined(__APPLE__)
                std::unique_lock<std::mutex> lock(mtx);
#endif
                double sleepTimeInSeconds = j->secondsToTimeout();

                if (sleepTimeInSeconds <= 0) // This case seems like an edge case (we already ran out of time)
                {
#if !defined(__linux) && !defined(__APPLE__)
                    if (::WaitForSingleObject(j->jobCheckLeaseEvent.m_h, 1) != WAIT_TIMEOUT)
                    {
                        if (j->status == JobItem::JOB_ERROR)
                            throw Aya::runtime_error(j->errorMessage.c_str());
                    }
#endif
                    closeJob(id);
                    throw Aya::runtime_error("BatchJob Timeout");
                }
#if !defined(__linux) && !defined(__APPLE__)
                else if (::WaitForSingleObject(j->jobCheckLeaseEvent.m_h, (DWORD)(sleepTimeInSeconds * 1000.0) + 3) == WAIT_TIMEOUT)
#else
                else if (cv.wait_for(lock, std::chrono::milliseconds(static_cast<int>(sleepTimeInSeconds * 1000.0) + 3)) == std::cv_status::timeout)
#endif
                {
                    // do nothing, just continue looping. Probably sleepTimeInSeconds will be negative, and we'll throw
                }
                else
                {
                    // jobCheckLeaseEvent was set
                    if (j->status == JobItem::JOB_ERROR)
                        throw Aya::runtime_error(j->errorMessage.c_str());

                    // TODO: Shouldn't this be called BEFORE we throw??????
                    closeJob(id);
                    return;
                }
            }
        }
        catch (std::exception&)
        {
            throw;
        }
    }

    void asyncExecute(const std::string& id, ns1__ScriptExecution* script, std::vector<ns1__LuaValue*>* result, soap* soap)
    {
        try
        {
            this->execute(id, script, result, soap);
            closeJob(id);
        }
        catch (std::exception& e)
        {
            char szMsg[1024];
#if !defined(__linux) && !defined(__APPLE__)
            StringCchCopy(szMsg, ARRAYSIZE(szMsg), e.what());
#endif

            closeJob(id, szMsg);
        }
    }
    void openJob(const ns1__Job& job, ns1__ScriptExecution* script, std::vector<ns1__LuaValue*>* result, soap* soap, bool startHeartbeat)
    {
        std::string id = job.id;
        Aya::Http::gameID = job.id;

        shared_ptr<Aya::DataModel> dataModel;
#if !defined(__linux) && !defined(__APPLE__)
        ::InterlockedIncrement(&dataModelCount);
#endif
        try
        {
            shared_ptr<JobItem> j = createJob(job, startHeartbeat, dataModel);

            try
            {
                Aya::DataModel* pDataModel = dataModel.get();
                pDataModel->create<Aya::ContentProvider>()->setBaseUrl(GetBaseURL());

                // Monitor the job and close it if needed
                boost::thread(Aya::thread_wrapper(boost::bind(&CWebService::doCheckLease, this, j), "Check Expiration"));

                FASTLOGS(FLog::RCCServiceJobs, "Opened JobItem %s", id.c_str());
                FASTLOG1(FLog::RCCServiceJobs, "DataModel: %p", pDataModel);

                this->execute(id, script, result, soap);

                Aya::RunService* runService = Aya::ServiceProvider::find<Aya::RunService>(pDataModel);
                AYAASSERT(runService != NULL);
                j->notifyAliveConnection = runService->heartbeatSignal.connect(boost::bind(&CWebService::notifyAlive, this, _1));

                {
                    Aya::DataModel::LegacyLock lock(pDataModel, Aya::DataModelJob::Write);
                    pDataModel->loadCoreScripts();
                }
            }
            catch (std::exception& e)
            {
                char szMsg[1024];
#if !defined(__linux) && !defined(__APPLE__)
                StringCchCopy(szMsg, ARRAYSIZE(szMsg), e.what());
#endif

                boost::mutex::scoped_lock lock(sync);
                jobs.erase(id);
                throw;
            }
        }
        catch (std::exception& e)
        {
            char szMsg[1024];
#if !defined(__linux) && !defined(__APPLE__)
            StringCchCopy(szMsg, ARRAYSIZE(szMsg), e.what());
#endif

            FASTLOGS(FLog::RCCServiceJobs, "Closing DataModel due to exception: %s", szMsg);
            FASTLOG1(FLog::RCCServiceJobs, "DataModel: %p", dataModel.get());

            closeDataModel(dataModel);
            dataModel.reset();
            // ::InterlockedDecrement(&dataModelCount);
            throw;
        }
    }

    void closeDataModel(shared_ptr<Aya::DataModel> dataModel)
    {
        Aya::Security::Impersonator impersonate(Aya::Security::WebService);
        // CrashAfterTimeout crash(90);	// If after 90 seconds the datamodel doesn't close, then CRASH!!!!
        Aya::DataModel::closeDataModel(dataModel);
    }

    void notifyAlive(const Aya::Heartbeat& h)
    {
#if !defined(__linux) && !defined(__APPLE__)
        mainLogManager->NotifyFGThreadAlive();
#endif
    }

    void closeJob(const std::string& jobID, const char* errorMessage = NULL)
    {
        // Take a mutex for the duration of closeJob here. The arbiter thinks that as
        // soon as closeJob returns it is safe to force kill the rcc process, so make
        // sure that if it is in the process of closing the datamodel any parallel requests
        // to close the data model block at the mutex until the first one finishes.
        boost::mutex::scoped_lock closeLock(currentlyClosingMutex);

        shared_ptr<Aya::DataModel> dataModel;
        {
            boost::mutex::scoped_lock lock(sync);
            JobMap::iterator iter = jobs.find(jobID);
            if (iter != jobs.end())
            {
                if (errorMessage)
                {
                    iter->second->errorMessage = errorMessage;
                    iter->second->status = JobItem::JOB_ERROR;
                }
                else
                {
                    iter->second->status = JobItem::JOB_DONE;
                }
                // d9mz - I hope this line isn't important
#if !defined(__linux) && !defined(__APPLE__)
                iter->second->jobCheckLeaseEvent.Set();
#endif
                dataModel = iter->second->dataModel;
                iter->second->notifyAliveConnection.disconnect();
                jobs.erase(iter);
            }
        }

        if (dataModel)
        {
            FASTLOGS(FLog::RCCServiceJobs, "Closing JobItem %s", jobID.c_str());
            FASTLOG1(FLog::RCCServiceJobs, "DataModel: %p", dataModel.get());

            closeDataModel(dataModel);
            dataModel.reset();
#if !defined(__linux) && !defined(__APPLE__)
            if (::InterlockedDecrement(&dataModelCount) == 0)
                mainLogManager->DisableHangReporting();
#endif
        }
    }

    int jobCount()
    {
        return jobs.size();
    }

    void getExpiration(const std::string& jobID, double* timeout)
    {
        boost::mutex::scoped_lock lock(sync);
        *timeout = getJob(jobID)->second->secondsToTimeout();
    }

    void closeExpiredJobs(int* result)
    {
        std::vector<std::string> jobsToClose;
        {
            boost::mutex::scoped_lock lock(sync);
            JobMap::iterator end = jobs.end();
            for (JobMap::iterator iter = jobs.begin(); iter != end; ++iter)
                if (iter->second->secondsToTimeout() <= 0)
                    jobsToClose.push_back(iter->first);
        }
        *result = jobsToClose.size();
        std::for_each(jobsToClose.begin(), jobsToClose.end(), boost::bind(&CWebService::closeJob, this, _1, (const char*)NULL));
    }

    void closeAllJobs(int* result)
    {
        FASTLOG(FLog::RCCServiceJobs, "Closing all jobs command");

        std::vector<std::string> jobsToClose;
        {
            boost::mutex::scoped_lock lock(sync);
            JobMap::iterator end = jobs.end();
            for (JobMap::iterator iter = jobs.begin(); iter != end; ++iter)
                jobsToClose.push_back(iter->first);
        }
        *result = jobsToClose.size();
        std::for_each(jobsToClose.begin(), jobsToClose.end(), boost::bind(&CWebService::closeJob, this, _1, (const char*)NULL));
    }

    void getAllJobs(std::vector<ns1__Job*>& result, soap* soap)
    {
        boost::mutex::scoped_lock lock(sync);
        result.resize(jobs.size());

        int i = 0;
        JobMap::iterator iter = jobs.begin();
        JobMap::iterator end = jobs.end();
        while (iter != end)
        {
            ns1__Job* job = soap_new_ns1__Job(soap, -1);
            job->expirationInSeconds = iter->second->secondsToTimeout();
            job->category = iter->second->category;
            job->cores = iter->second->cores;
            job->id = iter->first;
            result[i] = job;
            ++iter;
            ++i;
        }
    }
};

boost::scoped_ptr<CWebService> CWebService::singleton;

void stop_CWebService()
{
    CWebService::singleton.reset();
}

void start_CWebService()
{
    CWebService::singleton.reset(new CWebService(false));
}

AYA_REGISTER_CLASS(ThumbnailGenerator);

void CWebService::collectPerfData()
{
#if !defined(__linux) && !defined(__APPLE__)
    while (::WaitForSingleObject(doneEvent.m_h, 1000) == WAIT_TIMEOUT) // used as an interruptible sleep.
        s_perfCounter->CollectData();
#endif
}

CWebService::CWebService(bool crashUploadOnly)
#if !defined(__linux) && !defined(__APPLE__)
    : doneEvent(TRUE, FALSE)
    , dataModelCount(0)
#else
    : dataModelCount(0)
#endif
{
    {
#if !defined(__linux) && !defined(__APPLE__)
        CVersionInfo vi;
        vi.Load(_AtlBaseModule.m_hInst);
        Aya::DebugSettings::robloxVersion = vi.GetFileVersionAsDotString();

        Aya::Analytics::setReporter("RCCService");
        Aya::Analytics::setAppVersion(vi.GetFileVersionAsString());
#else
        Aya::DebugSettings::robloxVersion = "69.420.00.00"; // Funny
        Aya::Analytics::setReporter("RCCService");
        Aya::Analytics::setAppVersion("69.420.00.00");
#endif
    }

#if !defined(__linux) && !defined(__APPLE__)
    s_perfCounter = CProcessPerfCounter::getInstance();
#endif
    perfData.reset(new boost::thread(Aya::thread_wrapper(boost::bind(&CWebService::collectPerfData, this), "CWebService::collectPerfData")));

    Aya::Http::init(Aya::Http::WinHttp, Aya::Http::CookieSharingSingleProcessMultipleThreads);
    Aya::Http::requester = "Server";

    Aya::Profiling::init(false);
    static Aya::FactoryRegistrator registerFactoryObjects; // this needs to be here so srand is called before rand

#if !defined(__linux) && !defined(__APPLE__)
    RobloxCrashReporter::silent = true;
    mainLogManager->WriteCrashDump();
#endif

    isThumbnailer = false;
    LoadAppSettings();

    LoadClientSettings(rccSettings);
    if (rccSettings.GetError())
    {
        AYACRASH(rccSettings.GetErrorString().c_str());
    }

    Aya::TaskSchedulerSettings::singleton();
    Aya::TaskScheduler::singleton().setThreadCount(Aya::TaskScheduler::ThreadPoolConfig(FInt::RCCServiceThreadCount));

    // Force loading of settings classes
    Aya::GameSettings::singleton();
    Aya::GameBasicSettings::singleton();
    Aya::LuaSettings::singleton();
    Aya::DebugSettings::singleton();
    Aya::PhysicsSettings::singleton();

    Aya::Soundscape::SoundService::soundDisabled = true;

    // Initialize the network code
    Aya::Network::initWithServerSecurity();

    // If crashUploadOnly = true, don't create a separate thread of control for uploading
#if !defined(__linux) && !defined(__APPLE__)
    static DumpErrorUploader dumpErrorUploader(!crashUploadOnly, "RCCService");

    // KISEKI TODO: This shit hangs very easily bc its slow as fuck..

    std::string dmpHandlerUrl = GetGridUrl(::GetBaseURL(), FFlag::UseDataDomain);
    dumpErrorUploader.InitCrashEvent(dmpHandlerUrl, mainLogManager->getCrashEventName());
    dumpErrorUploader.Upload(dmpHandlerUrl);
#endif

    fetchClientSettingsThread.reset(
        new boost::thread(Aya::thread_wrapper(boost::bind(&CWebService::validateClientSettings, this), "CWebService::validateClientSettings")));

    // load and set security versions immediately at start up so it's guaranteed to be there when server is launched
    /*
    securityKeyUpdater.reset(new SecurityKeyUpdater(GetSecurityKeyUrl2(GetBaseURL(), "2b4ba7fc-5843-44cf-b107-ba22d3319dcd")));
    md5Updater.reset(new MD5Updater(GetMD5HashUrl(GetBaseURL(), "2b4ba7fc-5843-44cf-b107-ba22d3319dcd")));
    memHashUpdater.reset(new MemHashUpdater(GetMemHashUrl(GetBaseURL(), "2b4ba7fc-5843-44cf-b107-ba22d3319dcd")));

    if (DFFlag::UseNewSecurityKeyApi)
    {
        securityKeyUpdater->run();
    }
    else
    {
        std::vector<std::string> versions = fetchAllowedSecurityVersions();
        Aya::Network::setSecurityVersions(versions);
    }

    md5Updater->run();

    if (DFFlag::UseNewMemHashApi)
    {
        if (DFString::MemHashConfig.size() < 3)
        {
            memHashUpdater->run();
        }
        else
        {
            Aya::Network::MemHashConfigs hashConfigs;
            MemHashUpdater::populateMemHashConfigs(hashConfigs, DFString::MemHashConfig);
            Aya::Network::Players::setGoldMemHashes(hashConfigs);
        }
    }

    // this thread uses client setting values, so it must be started AFTER client settings are loaded
    // create a thread to periodically check for security key changes
    fetchSecurityDataThread.reset(new boost::thread(Aya::thread_wrapper(boost::bind(&CWebService::validateSecurityData, this),
    "CWebService::validateSecurityData")));

    counters.reset(new CountersClient(GetBaseURL().c_str(), "76E5A40C-3AE1-4028-9F10-7C62520BD94F", NULL));
    */
    Aya::ViewBase::InitPluginModules();
}

CWebService::~CWebService()
{
    // i hope this line isn't important (again)
#if !defined(__linux) && !defined(__APPLE__)
    doneEvent.Set();
#endif
    perfData->join();
    //	fetchSecurityDataThread->join();
    //	fetchClientSettingsThread->join();

    // TODO: this line crashes sometimes :-(
    int result;
    closeAllJobs(&result);

    Aya::ViewBase::ShutdownPluginModules();
}

std::string CWebService::GetSettingsKey()
{
    return "RccThumbnailers";
}

void CWebService::LoadClientSettings(RCCServiceSettings& dest)
{
    // cache settings in a string before processing
    LoadClientSettings(clientSettingsData, thumbnailSettingsData);
    LoadClientSettingsFromString("RccThumbnailers", clientSettingsData, &dest);
}

void CWebService::LoadClientSettings(std::string& clientDest, std::string& thumbnailDest)
{
    clientDest.clear();
    thumbnailDest.clear();

    std::string key = GetSettingsKey();
    FetchClientSettingsData("RccThumbnailers", "D6925E56-BFB9-4908-AAA2-A5B1EC4B2D79", &thumbnailDest);
    FetchClientSettingsData("RccThumbnailers", "D6925E56-BFB9-4908-AAA2-A5B1EC4B2D79", &clientDest);

    bool invalidClientSettings = (clientDest.empty() || key.length() == 0);
    bool invalidThumbnailerSettings = (thumbnailDest.empty() && isThumbnailer);

    if (invalidClientSettings || invalidThumbnailerSettings)
    {
        Aya::StandardOut::singleton()->printf(
            Aya::MESSAGE_ERROR, "Unable to load ClientSettings / ThumbnailerSettings! Please check the webserver. Unexpected behavior may occur.");
        Aya::StandardOut::singleton()->printf(Aya::MESSAGE_ERROR, "-- invalidClientSettings: %s, invalidThumbnailerSettings: %s",
            invalidClientSettings ? "true" : "false", invalidThumbnailerSettings ? "true" : "false");
        Aya::StandardOut::singleton()->printf(Aya::MESSAGE_ERROR, "-- clientDest.empty(): %s, key.length() == 0: %s",
            clientDest.empty() ? "true" : "false", key.length() == 0 ? "true" : "false");
    }
}

void CWebService::LoadAppSettings()
{
    SetBaseURL(GetBaseURL());
}

std::vector<std::string> CWebService::fetchAllowedSecurityVersions()
{
    const std::string& baseUrl = GetBaseURL();
    if (baseUrl.size() == 0)
        AYACRASH(); // y u no set BaseURL??

    std::vector<std::string> versions;

    std::string url = GetSecurityKeyUrl(baseUrl, "2b4ba7fc-5843-44cf-b107-ba22d3319dcd");

    Aya::Http request(url);

    std::string allowedSecurityVerionsData = "";
    try
    {
        request.get(allowedSecurityVerionsData);

        // no need to continue if security version did not change
        if (allowedSecurityVerionsData == securityVersionData)
            return std::vector<std::string>();

        // parse json data
        rapidjson::Document document;
        if (!document.Parse(allowedSecurityVerionsData.c_str()).HasParseError())
        {
            if (document.HasMember("data") && document["data"].IsArray())
            {
                const rapidjson::Value& dataArray = document["data"];
                for (rapidjson::SizeType i = 0; i < dataArray.Size(); i++)
                {
                    // version = data + salt
#ifndef __linux
                    std::string version = Aya::sha1(dataArray[i].GetString() + std::string("askljfLUZF"));
#else
                    // this causes a linker error, im too lazy to go edit cmakelists
                    std::string version = (dataArray[i].GetString() + std::string("askljfLUZF"));
#endif
                    versions.push_back(version);
                }
            }
        }

        securityVersionData = allowedSecurityVerionsData;
    }
    catch (std::exception ex)
    {
        FASTLOG(FLog::Always, "LoadAllowedPlayerVersions exception");
    }

    return versions;
}

void CWebService::JobItem::touch(double seconds)
{
    expirationTime = Aya::Time::now<Aya::Time::Fast>() + Aya::Time::Interval(seconds);
}

double CWebService::JobItem::secondsToTimeout() const
{
    return (expirationTime - Aya::Time::now<Aya::Time::Fast>()).seconds();
}

int RCCServiceSoapService::HelloWorld(_ns1__HelloWorld* ns1__HelloWorld, _ns1__HelloWorldResponse& ns1__HelloWorldResponse)
{
    BEGIN_PRINT(HelloWorld, "HelloWorld");

    ns1__HelloWorldResponse.HelloWorldResult = soap_new_std__string(this, -1);
    *ns1__HelloWorldResponse.HelloWorldResult = "Hello World";
    END_PRINT(HelloWorld, "HelloWorld");
    return 0;
}

int RCCServiceSoapService::Diag(_ns1__Diag* ns1__Diag, _ns1__DiagResponse& ns1__DiagResponse)
{
    BEGIN_PRINT(Diag, "Diag");

    Aya::Security::Impersonator impersonate(Aya::Security::WebService);
    CWebService::singleton->diag(ns1__Diag->type, *ns1__Diag->jobID, &ns1__DiagResponse.DiagResult, this);

    END_PRINT(Diag, "Diag");
    return 0;
}
int RCCServiceSoapService::DiagEx(_ns1__DiagEx* ns1__DiagEx, _ns1__DiagExResponse& ns1__DiagExResponse)
{
    BEGIN_PRINT(DiagEx, "DiagEx");
    Aya::Security::Impersonator impersonate(Aya::Security::WebService);
    ns1__DiagExResponse.DiagExResult = soap_new_ns1__ArrayOfLuaValue(this, -1);
    CWebService::singleton->diag(ns1__DiagEx->type, *ns1__DiagEx->jobID, &ns1__DiagExResponse.DiagExResult->LuaValue, this);
    END_PRINT(DiagEx, "DiagEx");
    return 0;
}

int RCCServiceSoapService::GetVersion(_ns1__GetVersion* ns1__GetVersion, _ns1__GetVersionResponse& ns1__GetVersionResponse)
{
    BEGIN_PRINT(GetVersion, "GetVersion");
    ns1__GetVersionResponse.GetVersionResult = Aya::DebugSettings::robloxVersion.c_str();
    END_PRINT(GetVersion, "GetVersion");
    return 0;
}

int RCCServiceSoapService::GetStatus(_ns1__GetStatus* ns1__GetStatus, _ns1__GetStatusResponse& ns1__GetStatusResponse)
{
    BEGIN_PRINT(GetStatus, "GetStatus");

    ns1__GetStatusResponse.GetStatusResult = soap_new_ns1__Status(this, -1);
    ns1__GetStatusResponse.GetStatusResult->version = soap_new_std__string(this, -1);
    *ns1__GetStatusResponse.GetStatusResult->version = Aya::DebugSettings::robloxVersion.c_str();
    ns1__GetStatusResponse.GetStatusResult->environmentCount = CWebService::singleton->jobCount();
    END_PRINT(GetStatus, "GetStatus");
    return 0;
}

int RCCServiceSoapService::OpenJob(_ns1__OpenJob* ns1__OpenJob, _ns1__OpenJobResponse& ns1__OpenJobResponse)
{
    BEGIN_PRINT(OpenJob, "OpenJob");
    Aya::Security::Impersonator impersonate(Aya::Security::WebService);
    CWebService::singleton->openJob(*ns1__OpenJob->job, ns1__OpenJob->script, &ns1__OpenJobResponse.OpenJobResult, this, true);
    END_PRINT(OpenJob, "OpenJob");

    return 0;
}

int RCCServiceSoapService::OpenJobEx(_ns1__OpenJobEx* ns1__OpenJobEx, _ns1__OpenJobExResponse& ns1__OpenJobExResponse)
{
    BEGIN_PRINT(OpenJobEx, "OpenJobEx");

    Aya::Security::Impersonator impersonate(Aya::Security::WebService);
    ns1__OpenJobExResponse.OpenJobExResult = soap_new_ns1__ArrayOfLuaValue(this, -1);
    CWebService::singleton->openJob(*ns1__OpenJobEx->job, ns1__OpenJobEx->script, &ns1__OpenJobExResponse.OpenJobExResult->LuaValue, this, true);

    END_PRINT(OpenJobEx, "OpenJobEx");
    return 0;
}

int RCCServiceSoapService::Execute(_ns1__Execute* ns1__Execute, _ns1__ExecuteResponse& ns1__ExecuteResponse)
{
    BEGIN_PRINT(Execute, "Execute");

    try
    {
        Aya::Security::Impersonator impersonate(Aya::Security::WebService);
        CWebService::singleton->execute(ns1__Execute->jobID, ns1__Execute->script, &ns1__ExecuteResponse.ExecuteResult, this);
    }
    catch (std::exception&)
    {
        throw;
    }
    END_PRINT(Execute, "Execute");
    return 0;
}

int RCCServiceSoapService::ExecuteEx(_ns1__ExecuteEx* ns1__ExecuteEx, _ns1__ExecuteExResponse& ns1__ExecuteExResponse)
{
    BEGIN_PRINT(ExecuteEx, "ExecuteEx");

    try
    {
        Aya::Security::Impersonator impersonate(Aya::Security::WebService);
        ns1__ExecuteExResponse.ExecuteExResult = soap_new_ns1__ArrayOfLuaValue(this, -1);
        CWebService::singleton->execute(ns1__ExecuteEx->jobID, ns1__ExecuteEx->script, &ns1__ExecuteExResponse.ExecuteExResult->LuaValue, this);
    }
    catch (std::exception&)
    {
        throw;
    }

    END_PRINT(ExecuteEx, "ExecuteEx");

    return 0;
}

int RCCServiceSoapService::CloseJob(_ns1__CloseJob* ns1__CloseJob, _ns1__CloseJobResponse& ns1__CloseJobResponse)
{
    BEGIN_PRINT(CloseJob, "CloseJob");
    try
    {
        Aya::Security::Impersonator impersonate(Aya::Security::WebService);
        CWebService::singleton->closeJob(ns1__CloseJob->jobID);
    }
    catch (std::exception&)
    {
        throw;
    }
    END_PRINT(CloseJob, "CloseJob");
    return 0;
}

int RCCServiceSoapService::RenewLease(_ns1__RenewLease* ns1__RenewLease, _ns1__RenewLeaseResponse& ns1__RenewLeaseResponse)
{
    BEGIN_PRINT(RenewLease, "RenewLease");

    try
    {
        Aya::Security::Impersonator impersonate(Aya::Security::WebService);
        CWebService::singleton->renewLease(ns1__RenewLease->jobID, ns1__RenewLease->expirationInSeconds);
    }
    catch (std::exception&)
    {
        throw;
    }
    END_PRINT(RenewLease, "RenewLease");
    return 0;
}

int RCCServiceSoapService::BatchJob(_ns1__BatchJob* ns1__BatchJob, _ns1__BatchJobResponse& ns1__BatchJobResponse)
{
    BEGIN_PRINT(BatchJob, "BatchJob");

    Aya::Security::Impersonator impersonate(Aya::Security::WebService);
    // Batch jobs are completed synchronously, so there is no need to start the heartbeat
    try
    {
        CWebService::singleton->batchJob(*ns1__BatchJob->job, ns1__BatchJob->script, &ns1__BatchJobResponse.BatchJobResult, this);
    }
    catch (std::exception&)
    {
        throw;
    }
    END_PRINT(BatchJob, "BatchJob");

    return 0;
}

int RCCServiceSoapService::BatchJobEx(_ns1__BatchJobEx* ns1__BatchJobEx, _ns1__BatchJobExResponse& ns1__BatchJobExResponse)
{
    BEGIN_PRINT(BatchJobEx, "BatchJobEx");

    Aya::Security::Impersonator impersonate(Aya::Security::WebService);
    ns1__BatchJobExResponse.BatchJobExResult = soap_new_ns1__ArrayOfLuaValue(this, -1);
    // Batch jobs are completed synchronously, so there is no need to start the heartbeat
    try
    {
        CWebService::singleton->batchJob(*ns1__BatchJobEx->job, ns1__BatchJobEx->script, &ns1__BatchJobExResponse.BatchJobExResult->LuaValue, this);
    }
    catch (std::exception&)
    {
        throw;
    }

    END_PRINT(BatchJobEx, "BatchJobEx");
    return 0;
}

int RCCServiceSoapService::GetExpiration(_ns1__GetExpiration* ns1__GetExpiration, _ns1__GetExpirationResponse& ns1__GetExpirationResponse)
{
    BEGIN_PRINT(GetExpiration, "GetExpiration");

    try
    {
        CWebService::singleton->getExpiration(ns1__GetExpiration->jobID, &ns1__GetExpirationResponse.GetExpirationResult);
    }
    catch (std::exception&)
    {
#if !defined(__linux) && !defined(__APPLE__)
        ::InterlockedDecrement(&getExpirationCount);
#endif
        throw;
    }
    END_PRINT(GetExpiration, "GetExpiration");
    return 0;
}

int RCCServiceSoapService::GetAllJobs(_ns1__GetAllJobs* ns1__GetAllJobs, _ns1__GetAllJobsResponse& ns1__GetAllJobsResponse)
{
    BEGIN_PRINT(GetAllJobs, "GetAllJobs");

    Aya::Security::Impersonator impersonate(Aya::Security::WebService);
    CWebService::singleton->getAllJobs(ns1__GetAllJobsResponse.GetAllJobsResult, this);
    END_PRINT(GetAllJobs, "GetAllJobs");
    return 0;
}

int RCCServiceSoapService::GetAllJobsEx(_ns1__GetAllJobsEx* ns1__GetAllJobsEx, _ns1__GetAllJobsExResponse& ns1__GetAllJobsExResponse)
{
    BEGIN_PRINT(GetAllJobsEx, "GetAllJobsEx");

    Aya::Security::Impersonator impersonate(Aya::Security::WebService);
    ns1__GetAllJobsExResponse.GetAllJobsExResult = soap_new_ns1__ArrayOfJob(this, -1);
    CWebService::singleton->getAllJobs(ns1__GetAllJobsExResponse.GetAllJobsExResult->Job, this);
    END_PRINT(GetAllJobsEx, "GetAllJobsEx");
    return 0;
}

int RCCServiceSoapService::CloseExpiredJobs(
    _ns1__CloseExpiredJobs* ns1__CloseExpiredJobs, _ns1__CloseExpiredJobsResponse& ns1__CloseExpiredJobsResponse)
{
    BEGIN_PRINT(GetAllJobs, "CloseExpiredJobs");

    Aya::Security::Impersonator impersonate(Aya::Security::WebService);
    CWebService::singleton->closeExpiredJobs(&ns1__CloseExpiredJobsResponse.CloseExpiredJobsResult);
    END_PRINT(GetAllJobs, "CloseExpiredJobs");
    return 0;
}

int RCCServiceSoapService::CloseAllJobs(_ns1__CloseAllJobs* ns1__CloseAllJobs, _ns1__CloseAllJobsResponse& ns1__CloseAllJobsResponse)
{
    BEGIN_PRINT(GetAllJobs, "CloseAllJobs");

    Aya::Security::Impersonator impersonate(Aya::Security::WebService);
    CWebService::singleton->closeAllJobs(&ns1__CloseAllJobsResponse.CloseAllJobsResult);
    END_PRINT(GetAllJobs, "CloseAllJobs");
    return 0;
}