

#undef min
#undef max

#include "Utility/Statistics.hpp"

#include <sstream>
#include <algorithm>

#include "AyaFormat.hpp"
#include "Utility/StandardOut.hpp"
#include "Utility/Http.hpp"
#include "format_string.hpp"
#include "SimpleJSON.hpp"
#include "Utility/FileSystem.hpp"
#include "time.hpp"
#include "Utility/RobloxServicesTools.hpp"

#include "Xml/WebParser.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include "Utility/Utilities.hpp"
#include "make_shared.hpp"

#include "DataModel/ContentProvider.hpp"

#include "SystemUtil.hpp"
#include <rapidjson/document.h>
LOGGROUP(ClientSettings);

#include "StandardOut.hpp"

#undef GetObject

ABTEST_NEWSTUDIOUSERS_VARIABLE(DummyTest);

#if defined(AYA_TEST_BUILD)
static std::string defaultFilePath;

void SetDefaultFilePath(const std::string& path)
{
    defaultFilePath = path;
}

const std::string& GetDefaultFilePath()
{
    return defaultFilePath;
}
#endif

static std::string defaltMasterServerUrl;
void SetMasterServerURL(const std::string& baseUrl)
{
    defaltMasterServerUrl = baseUrl;
}

const std::string& GetMasterServerURL()
{
    return defaltMasterServerUrl;
}

static bool usingMasterServer = false;
void SetUsingMasterServer(bool useMasterServer)
{
    usingMasterServer = useMasterServer;
}

const bool IsUsingMasterServer()
{
    return usingMasterServer;
}

static std::string defaltMasterServerKey;
void SetMasterServerKey(const std::string& key)
{
    defaltMasterServerKey = key;
}

const std::string& GetMasterServerKey()
{
    return defaltMasterServerKey;
}

static std::string defaultBaseUrl;
void SetBaseURL(const std::string& baseUrl)
{
    // legacy holdover
    if (baseUrl.rbegin() != baseUrl.rend() && *baseUrl.rbegin() != '/')
        defaultBaseUrl = baseUrl + "/";
    else
        defaultBaseUrl = baseUrl;
}


static std::string defaultInstanceAccessKey;
void SetInstanceAccessKey(const std::string& accessKey)
{
    defaultInstanceAccessKey = accessKey;
}

const std::string& GetInstanceAccessKey()
{
    return defaultInstanceAccessKey;
}

static bool usingInstance = false;
void SetUsingInstance(bool useInstance)
{
    usingInstance = useInstance;

    if (usingInstance)
        FetchInstanceMetadata();
}

const bool IsUsingInstance()
{
    return usingInstance;
}

// we default initialize with empty strings because note contentprovider needs these
static std::string instanceName = "";
static std::string instanceCurrency = "";
static std::string instanceMotd = "";

void FetchInstanceMetadata()
{
    std::string response;
    Aya::Http request(GetBaseURL() + "api/metadata");
    try
    {
        request.get(response);
    }
    catch (Aya::base_exception&)
    {
        Aya::StandardOut::singleton()->print(Aya::MESSAGE_WARNING, "Error fetching instance metadata");
        return;
    }

    // try parsing the json
    try
    {
        rapidjson::Document metadata;
        metadata.Parse(response.c_str());
        if (metadata.HasParseError())
            throw std::runtime_error("Failed to parse instance metadata JSON");

        if (metadata.HasMember("error"))
            throw std::runtime_error(("Error fetching instance metadata: " + std::string(metadata["error"].GetString())).c_str());

        if (metadata.HasMember("result") && metadata["result"].IsObject())
        {
            rapidjson::Value& result = metadata["result"].GetObject();
            if (result.HasMember("motd") && result["motd"].IsString())
                instanceMotd = result["motd"].GetString();
            if (result.HasMember("currency") && result["currency"].IsString())
                instanceCurrency = result["currency"].GetString();
            if (result.HasMember("name") && result["name"].IsString())
                instanceName = result["name"].GetString();
        }
    }
    catch (std::exception& e)
    {
        Aya::StandardOut::singleton()->print(Aya::MESSAGE_WARNING, e.what());
    }
}

const std::string& GetInstanceCurrency()
{
    return instanceCurrency;
}

const std::string& GetInstanceName()
{
    return instanceName;
}

const std::string& GetInstanceMotd()
{
    return instanceMotd;
}

static bool insecureMode = false;
void SetInsecureMode(bool mode)
{
    insecureMode = mode;
}

const bool IsInsecureMode()
{
    return insecureMode;
}

static bool verboseLogging = false;
void SetVerboseLogging(bool verbose)
{
    verboseLogging = verbose;
}

const bool IsVerboseLogging()
{
    return verboseLogging;
}

const std::string& GetBaseURL()
{
    return defaultBaseUrl;
}

static std::string defaultTrustCheckUrl;
void SetTrustCheckURL(const std::string& url)
{
    defaultTrustCheckUrl = url;
}

const std::string& GetTrustCheckURL()
{
    return defaultTrustCheckUrl;
}

static bool usingTrustCheck = false;
void SetUsingTrustCheck(bool useTrustCheck)
{
    usingTrustCheck = useTrustCheck;
}

const bool IsUsingTrustCheck()
{
    return usingTrustCheck;
}

std::string Aya::Http::urlEncode(const std::string& url)
{
    std::string result;

    const size_t strLen = url.size();
    for (unsigned i = 0; i < strLen; i++)
    {
        unsigned char c = url[i];
        if ((c <= 47) || (c >= 58 && c <= 64) || (c >= 91 && c <= 96) || (c >= 123))
        {
            result += Aya::format("%%%02X", c);
        }
        else
            result += c;
    }

    return result;
}

std::string Aya::Http::urlDecode(const std::string& fragment)
{
    std::string result;

    const size_t strLen = fragment.size();
    std::string::size_type i = 0;
    char hexBuffer[3];
    while (i < strLen)
    {
        unsigned char c = fragment[i];
        if (c == '%')
        {
            AYAASSERT((i + 3) <= strLen);
            hexBuffer[0] = fragment[i + 1];
            hexBuffer[1] = fragment[i + 2];
            hexBuffer[2] = '\0';
            result += (char)strtol(hexBuffer, NULL, 16);
            i += 3;
        }
        else
        {
            result += c;
            ++i;
        }
    }
    return result;
}

std::string UploadLogFile(const std::string& baseUrl, const std::string& data)
{
    try
    {
        std::stringstream request;
        request << baseUrl + "/Analytics/LogFile.ashx" << char(0);

        std::stringstream dataStream;
        dataStream << data << char(0);

        std::string result;
        Aya::Http(request.str()).post(dataStream, Aya::Http::kContentTypeDefaultUnspecified, true, result);
        return result;
    }
    catch (std::bad_alloc&)
    {
        throw;
    }
    catch (Aya::base_exception&)
    {
        return "";
    }
}

bool FetchLocalClientSettingsData(const char* group, SimpleJSON* dest)
{
#if defined(AYA_TEST_BUILD) || defined(AYA_SERVER)
    std::string localSettingsData;
    // Load from file
    boost::filesystem::path localFilename = Aya::FileSystem::getUserDirectory(false, Aya::DirExe, "ClientSettings") / (group + std::string(".json"));
    std::ifstream localConfigFile(localFilename.native().c_str());

    if (localConfigFile.is_open())
    {
        std::stringstream localConfigBuffer;
        localConfigBuffer << localConfigFile.rdbuf();
        localSettingsData = localConfigBuffer.str();
        if (localSettingsData.length() > 0)
        {
            FASTLOG(FLog::ClientSettings, "Found local json file");
            dest->ReadFromStream(localSettingsData.c_str());
            return true;
        }
    }
#endif
    return false;
}

void LoadClientSettingsFromString(const char* group, const std::string& settingsData, SimpleJSON* dest)
{
    FASTLOGS(FLog::ClientSettings, "Loading group %s", group);

    if (settingsData.length() > 0)
    {
        dest->ReadFromStream(settingsData.c_str());
    }

    bool localDataFetched = FetchLocalClientSettingsData(group, dest);

    if (localDataFetched && settingsData.length() == 0)
    {
        FASTLOG(FLog::ClientSettings, "Couldn't find any data to fetch");
    }
}

static bool FetchLocalClientSettings = false;

void SetFetchLocalClientSettings(bool status)
{
    FetchLocalClientSettings = status;
}

// returns if the fetch from web was successful, useful if you want to terminate the operation if web fetch fails
bool FetchClientSettingsData(const char* group, const char* apiKey, SimpleJSON* dest)
{
    bool fetchedFromWeb = false;
    std::string settingsData = "";
    FetchClientSettingsData(group, apiKey, &settingsData);

    if (settingsData.length())
        fetchedFromWeb = true;

    LoadClientSettingsFromString(group, settingsData, dest);

    return fetchedFromWeb;
}

void FetchClientSettingsData(const char* group, const char* apiKey, std::string* dest)
{
    FASTLOGS(FLog::ClientSettings, "Loading group %s to string", group);

    if (!FetchLocalClientSettings)
    {
        const std::string& baseUrl = GetBaseURL();

        std::string url = GetSettingsUrl(baseUrl, group, apiKey);

        Aya::Http request(url);

        try
        {
            request.get(*dest);
        }
        catch (Aya::base_exception&)
        {
            FASTLOG(FLog::Always, "FetchClientSettingsData exception");
        }
    }
    else
    {
        std::string file = Aya::ContentProvider::getAssetFile("FastFlags.json");
        std::ifstream inSettings(file);
        if (!inSettings)
        {
            Aya::StandardOut::singleton()->printf(Aya::MESSAGE_ERROR, "Could not open %s", file.c_str());
        }
        else
        {
            std::stringstream buffer;
            buffer << inSettings.rdbuf();
            *dest = buffer.str();
        }
    }
}


struct ABPass
{
    std::string entry;
    shared_ptr<Aya::Reflection::ValueArray> experiments;
};

void addABTest(const std::string& name, const std::string& value, void* context)
{
    ABPass* pass = (ABPass*)context;

    shared_ptr<Aya::Reflection::ValueTable> experimentTable = Aya::make_shared<Aya::Reflection::ValueTable>();
    (*experimentTable)["Name"] = name;
    (*experimentTable)["Type"] = pass->entry;

    pass->experiments->push_back(shared_ptr<const Aya::Reflection::ValueTable>(experimentTable));
}

Aya::HttpFuture FetchABTestDataAsync(const std::string& url)
{
    FASTLOG(FLog::ClientSettings, "Loading AB tests to http future");

    shared_ptr<Aya::Reflection::ValueArray> experiments = Aya::make_shared<Aya::Reflection::ValueArray>();
    ABPass passData;
    passData.experiments = experiments;

    passData.entry = "NewUsers";
    FLog::ForEachVariable(addABTest, &passData, FASTVARTYPE_AB_NEWUSERS);
    passData.entry = "NewStudioUsers";
    FLog::ForEachVariable(addABTest, &passData, FASTVARTYPE_AB_NEWSTUDIOUSERS);
    passData.entry = "AllUsers";
    FLog::ForEachVariable(addABTest, &passData, FASTVARTYPE_AB_ALLUSERS);

    std::string jsonRequest;
    Aya::WebParser::writeJSON(shared_ptr<const Aya::Reflection::ValueArray>(experiments), jsonRequest);

    Aya::HttpPostData postData(jsonRequest, Aya::Http::kContentTypeApplicationJson, false);

    return Aya::HttpAsync::post(url, postData);
}

std::string LoadABTestFromString(const std::string& responseData)
{
    std::string trackerId = "";
    shared_ptr<const Aya::Reflection::ValueTable> jsonResponse;
    bool result = Aya::WebParser::parseJSONTable(responseData, jsonResponse);
    AYAASSERT(result);
    if (!result)
        return trackerId;

    try
    {
        shared_ptr<const Aya::Reflection::ValueTable> experiments =
            jsonResponse->at("Experiments").cast<shared_ptr<const Aya::Reflection::ValueTable>>();

        for (Aya::Reflection::ValueTable::const_iterator it = experiments->begin(); it != experiments->end(); ++it)
        {
            shared_ptr<const Aya::Reflection::ValueTable> experimentDesc = it->second.cast<shared_ptr<const Aya::Reflection::ValueTable>>();
            int value = experimentDesc->at("Variation").cast<int>();
            bool locked = experimentDesc->at("IsLocked").cast<bool>();

            // -1, 0, 1 and 2 are control groups, 3+ are variations
            int variation = std::max(value - 2, 0);
            FLog::SetValue(it->first, Aya::StringConverter<int>::convertToString(variation));
        }

        if (jsonResponse->find("BrowserTrackerId") != jsonResponse->end())
            trackerId = jsonResponse->at("BrowserTrackerId").cast<std::string>();
    }
    catch (std::exception& e)
    {
        AYAASSERT(false);
        FASTLOGS(FLog::ClientSettings, "Failed to parse AB test JSON - %s", e.what());
    }

    return trackerId;
}


void ReportStatisticPost(const std::string& baseUrl, const std::string& id, const std::string& postData, const char* primaryFilterName,
    const char* primaryFilterValue, const char* secondaryFilterName, const char* secondaryFilterValue);

void ReportStatisticWithMessage(const std::string& baseUrl, const std::string& id, const std::string& simpleMessage, const char* secondaryFilterName,
    const char* secondaryFilterValue)
{
    std::stringstream data;
    if (simpleMessage.length() > 0)
    {
        std::string messageStr(simpleMessage);
        while (messageStr.find("\n") != std::string::npos)
        {
            messageStr.replace(messageStr.find("\n"), 1, "%20");
        }
        while (messageStr.find("\t") != std::string::npos)
        {
            messageStr.replace(messageStr.find("\t"), 1, "%20");
        }

        data << "string\tmessage\t" << messageStr;
    }
    data << char(0);

    ReportStatisticPost(baseUrl, id, data.str(), NULL, NULL, secondaryFilterName, secondaryFilterValue);
}

void ReportStatistic(const std::string& baseUrl, const std::string& id, const std::string& primaryFilterName, const std::string& primaryFilterValue,
    const std::string& secondaryFilterName, const std::string& secondaryFilterValue)
{
    ReportStatisticPost(baseUrl, id, "",
        primaryFilterName.empty() ? 0 : primaryFilterName.c_str(), // 0 means use IPFilter!
        primaryFilterValue.c_str(), secondaryFilterName.c_str(), secondaryFilterValue.c_str());
}

void ReportStatisticPost(
    const std::string& baseUrl, const std::string& id, const std::string& postData, const char* secondaryFilterName, const char* secondaryFilterValue)
{
    ReportStatisticPost(baseUrl, id, postData, NULL, NULL, secondaryFilterName, secondaryFilterValue);
}

void DontCareResponse(std::string* response, std::exception* exception) {}
void ReportStatisticPost(const std::string& baseUrl, const std::string& id, const std::string& postData, const char* primaryFilterName,
    const char* primaryFilterValue, const char* secondaryFilterName, const char* secondaryFilterValue)
{
    //
}
