


#include "DataModel/ContentProvider.hpp"
#include "Utility/ScriptInformationProvider.hpp"
#include "AyaAssert.hpp"
#include "Xml/Serializer.hpp"
#include "Xml/XmlSerializer.hpp"
#include "Utility/Http.hpp"
#include <string.h>
#include "Utility/StandardOut.hpp"
#include "Utility/FileSystem.hpp"
#include "Utility/ThreadPool.hpp"
#include "Utility/Statistics.hpp"
#include "DataModel/DataModel.hpp"
#include "Crypt.hpp"
#include "StringConv.hpp"
#include "Utility/RobloxServicesTools.hpp"
#include "Utility/Statistics.hpp"
#if defined(_WIN32) && !defined(AYA_PLATFORM_DURANGO)
#include "ATLPath.h"
#include "FastLog.hpp"
#endif

#include <fstream>
#include <iostream>
#include <iterator>
#include <sys/stat.h>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

// Prevent ContentFilter from calling the stats handler
// TODO: Filter even harder so that ContentFilter *only* calls asset handler
#include "boost/date_time/posix_time/posix_time.hpp"

#define BOOST_FILESYSTEM_NO_DEPRECATED
#include "boost/filesystem.hpp"

#ifdef AYA_TEST_BUILD
#include "Utility/Statistics.hpp"
#endif

#include "XStudioBuild.hpp"

using namespace boost::posix_time;
namespace fs = boost::filesystem;

LOGGROUP(ContentProviderCleanup)
LOGGROUP(ContentProviderRequests)
LOGGROUP(ContentProviderRequestsFullContent)
FASTFLAGVARIABLE(NoCacheForLocalContent, false)
DYNAMIC_FASTINTVARIABLE(ContentProviderThreadPoolSize, 16)
DYNAMIC_FASTFLAGVARIABLE(ContentProviderHttpCaching, false)

DYNAMIC_FASTFLAGVARIABLE(ImageFailedToLoadContext, false)
DYNAMIC_FASTFLAG(UrlReconstructToAssetGame)

using namespace boost::posix_time;

#ifdef _WIN32
#define APPLOG(message) \
    do \
    { \
        if (appLog != NULL) \
        { \
            Aya::mutex::scoped_lock lock(*appLogLock); \
            appLog->writeEntry(Aya::Log::Information, message); \
        } \
    } while (0)
#else
#define APPLOG(message) \
    { \
    }
#endif

namespace
{
inline std::string pathToString(const fs::path& path)
{
#ifdef _WIN32
    return Aya::utf8_encode(path.wstring());
#else
    return path.string();
#endif
}

inline fs::path stringToPath(const std::string& str)
{
#ifdef _WIN32
    return Aya::utf8_decode(str.c_str());
#else
    return fs::path(str);
#endif
}
} // namespace

namespace Aya
{
extern void ThrowLastError(const char* message);

const char* const sContentProvider = "ContentProvider";

Log* ContentProvider::appLog = NULL;
Aya::mutex* ContentProvider::appLogLock = NULL;
boost::mutex ContentProvider::preloadContentBlockingMutex;

float ContentProvider::PRIORITY_MFC = 0;
float ContentProvider::PRIORITY_SCRIPT = 10;
float ContentProvider::PRIORITY_MESH = 20;
float ContentProvider::PRIORITY_SOLIDMODEL = 25;
float ContentProvider::PRIORITY_INSERT = 30;
float ContentProvider::PRIORITY_CHARACTER = 40;
float ContentProvider::PRIORITY_ANIMATION = 50;
float ContentProvider::PRIORITY_TEXTURE = 55;
float ContentProvider::PRIORITY_DECAL = 60;
float ContentProvider::PRIORITY_DEFAULT = 70;
float ContentProvider::PRIORITY_SOUND = 80;
float ContentProvider::PRIORITY_GUI = 90;
float ContentProvider::PRIORITY_SKY = 100;


static Reflection::BoundFuncDesc<ContentProvider, void(std::string)> func_setBaseUrl(
    &ContentProvider::setBaseUrl, "SetBaseUrl", "url", Security::LocalUser);
Reflection::PropDescriptor<ContentProvider, std::string> ContentProvider::desc_baseUrl("BaseUrl", category_Data, &ContentProvider::getBaseUrl, NULL);
static Reflection::BoundFuncDesc<ContentProvider, void(std::string)> func_setInstanceName(
    &ContentProvider::setInstanceName, "SetInstanceName", "name", Security::LocalUser);
Reflection::PropDescriptor<ContentProvider, std::string> ContentProvider::desc_instanceName(
    "InstanceName", category_Data, &ContentProvider::getInstanceName, NULL);
static Reflection::BoundFuncDesc<ContentProvider, void(std::string)> func_setInstanceCurrency(
    &ContentProvider::setInstanceCurrency, "SetInstanceCurrency", "currency", Security::LocalUser);
Reflection::PropDescriptor<ContentProvider, std::string> ContentProvider::desc_instanceCurrency(
    "InstanceCurrency", category_Data, &ContentProvider::getInstanceCurrency, NULL);
static Reflection::BoundFuncDesc<ContentProvider, void(std::string)> func_setInstanceMotd(
    &ContentProvider::setInstanceMotd, "SetInstanceMotd", "motd", Security::LocalUser);
Reflection::PropDescriptor<ContentProvider, std::string> ContentProvider::desc_instanceMotd(
    "InstanceMotd", category_Data, &ContentProvider::getInstanceMotd, NULL);
static Reflection::BoundFuncDesc<ContentProvider, void(std::string)> func_setAssetUrl(
    &ContentProvider::setBaseUrl, "SetAssetUrl", "url", Security::LocalUser);
static Reflection::BoundFuncDesc<ContentProvider, void(int)> func_setThreadPool(
    &ContentProvider::setThreadPool, "SetThreadPool", "count", Security::LocalUser);
static Reflection::BoundFuncDesc<ContentProvider, void(int)> func_setCacheSize(
    &ContentProvider::setCacheSize, "SetCacheSize", "count", Security::LocalUser);
static Reflection::BoundFuncDesc<ContentProvider, void(ContentId)> func_preload(
    &ContentProvider::preloadContent, "Preload", "contentId", Security::None);
static Reflection::BoundYieldFuncDesc<ContentProvider, void(shared_ptr<const Reflection::ValueArray>)> func_preloadAsync(
    &ContentProvider::preloadContentBlockingList, "PreloadAsync", "contentIdList", Security::None);

static Reflection::PropDescriptor<ContentProvider, int> desc_requestQueueSize(
    "RequestQueueSize", category_Data, &ContentProvider::getRequestQueueSize, NULL);
REFLECTION_END();

const std::string& ContentProvider::getBaseUrl() const
{
    return GetBaseURL(); // sorry -- SetBaseUrl is now deprecated from Lua, you'll have to modify AppSettings.
}

const std::string& ContentProvider::getInstanceName() const
{
    return GetInstanceName();
}

const std::string& ContentProvider::getInstanceCurrency() const
{
    return GetInstanceCurrency();
}

const std::string& ContentProvider::getInstanceMotd() const
{
    return GetInstanceMotd();
}

const std::string ContentProvider::getApiBaseUrl() const
{
    return getApiBaseUrl(baseUrl);
}

const std::string ContentProvider::getUnsecureApiBaseUrl() const
{
    return getUnsecureApiBaseUrl(baseUrl);
}

std::string ContentProvider::getApiBaseUrl(const std::string& baseUrl)
{
    std::string apiBaseUrl = getUnsecureApiBaseUrl(baseUrl);
    if (!apiBaseUrl.empty())
    {
        std::size_t foundPos = apiBaseUrl.find("https");
        if (foundPos == std::string::npos)
        {
            foundPos = apiBaseUrl.find("http");
            if (foundPos != std::string::npos)
            {
                apiBaseUrl.replace(foundPos, 4, "https");
            }
        }
        return apiBaseUrl;
    }

    return baseUrl;
}

std::string ContentProvider::getUnsecureApiBaseUrl(const std::string& baseUrl)
{
    if (!baseUrl.empty())
    {
        return ReplaceTopSubdomain(baseUrl, "api");
    }

    return baseUrl;
}


void ContentProvider::setBaseUrl(std::string url)
{
    if (this->baseUrl != url)
    {
        this->baseUrl = url;
        raisePropertyChanged(desc_baseUrl);
    }
}

void ContentProvider::setInstanceName(std::string name)
{
    if (this->instanceName != name)
    {
        this->instanceName = name;
        raisePropertyChanged(desc_instanceName);
    }
}

void ContentProvider::setInstanceCurrency(std::string currency)
{
    if (this->instanceCurrency != currency)
    {
        this->instanceCurrency = currency;
        raisePropertyChanged(desc_instanceCurrency);
    }
}

void ContentProvider::setInstanceMotd(std::string motd)
{
    if (this->instanceMotd != motd)
    {
        this->instanceMotd = motd;
        raisePropertyChanged(desc_instanceMotd);
    }
}

void ContentProvider::setThreadPool(int count)
{
    contentCache->setThreadPool(count);
}
void ContentProvider::setCacheSize(int count)
{
    contentCache->setCacheSize(count);
}
shared_ptr<const Reflection::ValueArray> ContentProvider::getFailedUrls()
{
    return contentCache->getFailedUrls();
}
int ContentProvider::getRequestQueueSize() const
{
    return contentCache->getRequestQueueSize();
}
shared_ptr<const Reflection::ValueArray> ContentProvider::getRequestQueueUrls()
{
    return contentCache->getRequestQueueUrls();
}
shared_ptr<const Reflection::ValueArray> ContentProvider::getRequestedUrls()
{
    return contentCache->getRequestedUrls();
}

bool ContentProvider::findLocalFile(const std::string& url, std::string* filename)
{
    ContentId id(url);

    if (id.isAsset() || id.isItem() || id.isLevel() || id.isAppContent())
    {
        *filename = findAsset(id);
        if (filename->empty())
            return false;
    }
    else if (!id.isHttp())
    {
        // This will only work if contendId is actually a hash
        *filename = findHashFile(id);

        if (filename->empty())
        {
            return false;
        }
    }

    return true;
}

ContentProvider::ContentProvider()
    : afm(NULL)
{
    setName(sContentProvider);

    contentCache.reset(new AsyncHttpCache<CachedContent>(this, &findLocalFile, DFInt::ContentProviderThreadPoolSize, 1000));

    if (DFFlag::ContentProviderHttpCaching)
    {
        contentCache->setCachePolicy(HttpCache::PolicyFinalRedirect);
    }
}

ContentProvider::~ContentProvider()
{
    Aya::FileSystem::clearCacheDirectory("ContentProvider");
}

void ContentProvider::verifyRequestedScriptSignature(const ProtectedString& source, const std::string& assetId, bool required)
{
    const char* script = source.getSource().c_str();

    try
    {
        verifyScriptSignature(source, required);

        // search for the asset header
        // looks like "--ayaassetid%1818%"
        const char* assetHeader = "--ayaassetid%";
        const char* idInScript = strstr(script, assetHeader); // will find first occurrence, which should be safe
        if (idInScript)
        {
            idInScript += strlen(assetHeader);                                 // now points to asset id in script, terminated by another %
            if (strncmp(idInScript, assetId.c_str(), assetId.length()) == 0 && // ensure match
                idInScript[assetId.length()] == '%')                           // terminated here
            {
                return; // correct
            }
            else
            {
                throw std::runtime_error(""); // mismatch
            }
        }

        if (required)
        {
            throw std::runtime_error("");
        }
    }
    catch (Aya::base_exception&)
    {
        // Intentionally strip out the exception call stack location
        throw std::runtime_error("");
    }
}

void ContentProvider::verifyScriptSignature(const ProtectedString& source, bool required)
{
#ifdef AYA_TEST_BUILD
    return;
#endif

    const char* script = source.getSource().c_str();

    try
    {
        // sig can be behind a Lua comment
        // looks like "--ayasig%MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCtfLLFT36v5r9bNP7STBteDU5a%"
        const char* sigHeader = "--ayasig%";
        if (strncmp(script, sigHeader, strlen(sigHeader)) == 0)
        {
            const char* sigStart = script + strlen(sigHeader);
            const char* sigEnd = strchr(sigStart, '%');
            if (!sigEnd)
            {
                throw std::runtime_error("");
            }
            std::string signature(sigStart, sigEnd - sigStart);
            const char* signedScript = sigEnd + 1; // skip terminal %. we signed the text after this signature.

            // verify now!, will throw runtime_error
            Crypt().verifySignatureBase64(signedScript, signature);

            return;
        }

        if (required)
        {
            throw std::runtime_error("");
        }
    }
    catch (Aya::base_exception&)
    {
        // Intentionally strip out the exceptions
        throw std::runtime_error("");
    }
}

void ContentProvider::onHeartbeat(const Heartbeat& heartbeatEvent)
{
    contentCache->onHeartbeat(heartbeatEvent);
}

void ContentProvider::setAssetFetchMediator(AssetFetchMediator* afm)
{
    this->afm = afm;
}

std::string ContentProvider::getAssetFile(const char* filePath)
{
    return std::string(assetFolder() + filePath);
}

bool ContentProvider::hasContent(const Aya::ContentId& id)
{
    return isContentLoaded(id);
}

bool ContentProvider::isUrlBad(Aya::ContentId id)
{
    id.convertAssetId(baseUrl, DataModel::get(this)->getUniverseId());
    id.convertToLegacyContent(baseUrl);
    return contentCache->isUrlBad(id.toString());
}

bool ContentProvider::isRequestQueueEmpty()
{
    return contentCache->isRequestQueueEmpty();
}

bool ContentProvider::registerFile(const ContentId& id, CachedContent* item)
{
    if (!item->filename)
    {
        std::istringstream ss(*item->data);
        ContentId hash = registerContent(ss);
        std::string filename = findHashFile(hash);
        if (filename.size() == 0)
            return false;

        item->filename.reset(new std::string(filename));

        contentCache->insertCacheItem(id.c_str(), *item);
    }
    return true;
}

static void returnResponse(AsyncHttpQueue::RequestResult result, boost::function<void(AsyncHttpQueue::RequestResult)> callback)
{
    callback(result);
}

static void parseString(const Aya::ContentId& id, AsyncHttpQueue::RequestResult result, std::istream* stream,
    boost::function<void(AsyncHttpQueue::RequestResult, shared_ptr<const std::string>, shared_ptr<std::exception>)> callback,
    shared_ptr<const std::string> response, shared_ptr<std::exception> requestError)
{
    shared_ptr<std::string> returnedResponse(new std::string());
    if (result == AsyncHttpQueue::Succeeded)
    {

        try
        {
            FASTLOGS(FLog::ContentProviderRequests, "Got content as string: %s", id.c_str());

            std::ostringstream strbuffer;
            strbuffer << stream->rdbuf();
            *returnedResponse = strbuffer.str();
        }
        catch (std::runtime_error e)
        {
            Aya::StandardOut::singleton()->printf(Aya::MESSAGE_ERROR, "Content failed to load for %s because %s", id.c_str(), e.what());
            result = AsyncHttpQueue::Failed;
        }
    }

    callback(result, returnedResponse, requestError);
}

static void parseContent(const Aya::ContentId& id, AsyncHttpQueue::RequestResult result, std::istream* stream,
    boost::function<void(AsyncHttpQueue::RequestResult, shared_ptr<Instances>, shared_ptr<std::exception>)> callback,
    shared_ptr<const std::string> response, shared_ptr<std::exception> requestError)
{
    shared_ptr<Instances> instances(new Instances());

    if (result == AsyncHttpQueue::Succeeded)
    {
        try
        {
            FASTLOGS(FLog::ContentProviderRequests, "Parsing content: %s", id.c_str());

            Serializer().loadInstances(*stream, *instances);
        }
        catch (std::runtime_error e)
        {
            Aya::StandardOut::singleton()->printf(Aya::MESSAGE_ERROR, "Content failed to parse for %s because %s", id.c_str(), e.what());
            result = AsyncHttpQueue::Failed;
        }
    }

    callback(result, instances, requestError);
}

void ContentProvider::preloadContentWithCallback(Aya::ContentId id, float priority, boost::function<void(AsyncHttpQueue::RequestResult)> callback,
    AsyncHttpQueue::ResultJob jobType, const std::string& expectedType)
{
    getContent(id, priority, boost::bind(&returnResponse, _1, callback), jobType, expectedType);
}

void ContentProvider::preloadContentResultCallback(AsyncHttpQueue::RequestResult results, ContentId id)
{
    if (results != AsyncHttpQueue::Succeeded)
    {
        Aya::StandardOut::singleton()->printf(Aya::MESSAGE_ERROR, "ContentProvider:Preload() failed for %s", id.c_str());
    }
}

void ContentProvider::preloadContent(Aya::ContentId id)
{
    if (DFFlag::ImageFailedToLoadContext)
    {
        AsyncHttpQueue::RequestCallback callback = boost::bind(&ContentProvider::preloadContentResultCallback, this, _1, id);
        AsyncHttpQueue::RequestResult requestResult = privateLoadContent(id, AsyncHttpRequest, INT_MAX, NULL, &callback);
        if (requestResult != AsyncHttpQueue::Waiting)
            preloadContentResultCallback(requestResult, id);
    }
    else
    {
        privateLoadContent(id, AsyncHttpRequest, INT_MAX, NULL, NULL);
    }
}

void ContentProvider::preloadContentBlockingListHelper(AsyncHttpQueue::RequestResult results, boost::function<void()> resumeFunction,
    boost::function<void(std::string)> errorFunction, PreloadAsyncRequest* pRequest, ContentId id)
{
    boost::mutex::scoped_lock lock(preloadContentBlockingMutex);

    pRequest->outstanding--;
    if (results != AsyncHttpQueue::Succeeded)
    {
        if (DFFlag::ImageFailedToLoadContext)
        {
            Aya::StandardOut::singleton()->printf(Aya::MESSAGE_ERROR, "ContentProvider:PreloadAsync() failed for %s", id.c_str());
        }
        pRequest->failed++;
    }

    if (pRequest->outstanding == 0)
    {
        resumeFunction();
        delete pRequest;
    }
}

void ContentProvider::preloadContentBlockingList(
    shared_ptr<const Reflection::ValueArray> idList, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction)
{
    const Reflection::ValueArray* pList = idList.get();

    if (pList && pList->size() > 0)
    {
        for (size_t i = 0; i < pList->size(); i++)
        {
            Reflection::Variant v = (*pList)[i];
            ContentId id = v.get<Aya::ContentId>();

            if (!id.isNull() && !id.isAssetId() && !id.isHttp())
            {
                errorFunction("PreloadAsync: Bad format of asset " + v.get<std::string>());
                return;
            }
        }

        PreloadAsyncRequest* pRequest = new PreloadAsyncRequest(pList->size());
        for (size_t i = 0; i < pList->size(); i++)
        {
            Reflection::Variant v = (*pList)[i];
            ContentId id = v.get<Aya::ContentId>();
            AsyncHttpQueue::RequestCallback callback =
                boost::bind(&ContentProvider::preloadContentBlockingListHelper, this, _1, resumeFunction, errorFunction, pRequest, id);
            AsyncHttpQueue::RequestResult requestResult = privateLoadContent(id, AsyncHttpRequest, INT_MAX, NULL, &callback);
            if (requestResult != AsyncHttpQueue::Waiting)
                preloadContentBlockingListHelper(requestResult, resumeFunction, errorFunction, pRequest, id);
        }
    }
    else
    {
        resumeFunction();
    }
}

void ContentProvider::clearContent()
{
    contentCache->clearCache();
}

void ContentProvider::invalidateCache(ContentId id)
{
    id.convertAssetId(baseUrl, DataModel::get(this)->getUniverseId());
    id.convertToLegacyContent(baseUrl);
    contentCache->invalidateCacheItemOrFailure(id.toString());
}

void ContentProvider::loadContent(const Aya::ContentId& id, float priority,
    boost::function<void(AsyncHttpQueue::RequestResult, shared_ptr<Instances>, shared_ptr<std::exception>)> callback,
    AsyncHttpQueue::ResultJob jobType)
{
    getContent(id, priority, boost::bind(&parseContent, id, _1, _2, callback, _3, _4), jobType);
}

void ContentProvider::loadContentString(const Aya::ContentId& id, float priority,
    boost::function<void(AsyncHttpQueue::RequestResult, shared_ptr<const std::string>, shared_ptr<std::exception>)> callback,
    AsyncHttpQueue::ResultJob jobType)
{
    getContent(id, priority, boost::bind(&parseString, id, _1, _2, callback, _3, _4), jobType);
}

static void InvokeFileCallback(AsyncHttpQueue::RequestCallback callback, boost::shared_ptr<const std::string> filename)
{
    std::ifstream fileStream(utf8_decode(*filename).c_str(), std::ios_base::in | std::ios_base::binary);
    shared_ptr<std::exception> nullException;
    callback(AsyncHttpQueue::Succeeded, &fileStream, shared_ptr<const std::string>(), nullException);
}

void ContentProvider::getContent(const Aya::ContentId& id, float priority, AsyncHttpQueue::RequestCallback callback,
    AsyncHttpQueue::ResultJob jobType, const std::string& expectedType)
{
    ContentId activeId = id;
    AYAASSERT(this != NULL);

    CachedContent item;

    AsyncHttpQueue::RequestResult requestResult = privateLoadContent(activeId, AsyncHttpRequest, priority, &item, &callback, jobType, expectedType);
    switch (requestResult)
    {
    case AsyncHttpQueue::Succeeded:
        if (item.data)
        {
            shared_ptr<std::exception> nullException;
            AsyncHttpQueue::dispatchCallback(callback, this, AsyncHttpQueue::Succeeded, item.data, jobType, nullException);
        }
        else
        {
            AsyncHttpQueue::dispatchGenericCallback(boost::bind(&InvokeFileCallback, callback, item.filename), this, jobType);
        }
        break;
    case AsyncHttpQueue::Failed:
        AsyncHttpQueue::dispatchCallback(callback, this, AsyncHttpQueue::Failed, boost::shared_ptr<const std::string>(new std::string("")), jobType,
            shared_ptr<std::exception>(new std::runtime_error("Temp read failed.")));
        break;
    default:
        break;
    }
}

shared_ptr<const std::string> ContentProvider::getContentString(Aya::ContentId id)
{
    APPLOG("ContentProvider::getContentString");

    shared_ptr<const std::string> result = requestContentString(id, ContentProvider::PRIORITY_SCRIPT);
    if (!result)
    {
        CachedContent item;
        if (!blockingLoadContent(id, &item))
            throw Aya::runtime_error("Unable to load %s", id.c_str());

        result = requestContentString(id, ContentProvider::PRIORITY_SCRIPT);
        if (!result)
            throw Aya::runtime_error("Unable to retrieve cache of %s", id.c_str());
    }
    AYAASSERT(result);
    return result;
}

shared_ptr<const std::string> ContentProvider::requestContentString(const Aya::ContentId& id, float priority)
{
    AYAASSERT(this != NULL);
    APPLOG("ContentProvider::requestContentString - enter");
    Aya::ContentId activeId = id;

    CachedContent item;
    if (AsyncHttpQueue::Succeeded != privateLoadContent(activeId, AsyncHttpRequest, priority, &item, NULL))
        return shared_ptr<const std::string>();

    if (!item.data)
    {
        APPLOG("ContentProvider::requestContentString - putting data into cache");

        // Convert file to string data

        std::ifstream stream(utf8_decode(*item.filename).c_str(), std::ios_base::in | std::ios_base::binary);
        std::ostringstream data;

#if (defined(_DEBUG) && defined(__APPLE__))
#include "TargetConditionals.hpp"
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        data << stream.rdbuf(); // iOS in debug does not like boost copy here for some reason
#else
        boost::iostreams::copy(stream, data);
#endif
#else
        boost::iostreams::copy(stream, data);
#endif

        item.data.reset(new std::string(data.str()));

        if (!FFlag::NoCacheForLocalContent)
            contentCache->insertCacheItem(activeId.c_str(), item);
    }
    APPLOG("ContentProvider::requestContentString - leave");
    return item.data;
}

bool ContentProvider::blockingLoadContent(ContentId id, CachedContent* result, const std::string& expectedType)
{
    APPLOG(Aya::format("ContentProvider::requestContentString %s", id.c_str()).c_str());
    return privateLoadContent(id, SyncHttpRequest, -1, result, NULL, AsyncHttpQueue::AsyncInline, expectedType) == AsyncHttpQueue::Succeeded;
}
bool ContentProvider::isContentLoaded(ContentId id)
{
    return privateLoadContent(id, NoHttpRequest, -1, NULL, NULL) == AsyncHttpQueue::Succeeded;
}

AsyncHttpQueue::RequestResult ContentProvider::privateLoadContent(Aya::ContentId& id, RequestType httpRequestType, float priority,
    CachedContent* result, AsyncHttpQueue::RequestCallback* callback, AsyncHttpQueue::ResultJob jobType, const std::string& expectedType)
{
    APPLOG("ContentProvider::privateLoadContent");

    if (id.isNull())
        return AsyncHttpQueue::Failed;

    FASTLOGS(FLog::ContentProviderRequests, "Content requested: %s", id.c_str());

    if (afm)
    {
        if (boost::optional<std::string> content = afm->findCachedAssetOrEmpty(id, DataModel::get(this)->getUniverseId()))
        {
            *result = CachedContent(shared_ptr<std::string>(new std::string(content.get())), shared_ptr<std::string>());
            return AsyncHttpQueue::Succeeded;
        }
    }

    id.convertAssetId(baseUrl, DataModel::get(this)->getUniverseId());
    id.convertToLegacyContent(baseUrl);

    if (!id.reconstructAssetUrl(baseUrl))
        return AsyncHttpQueue::Failed;

    if (contentCache->findCacheItem(id.toString(), result))
    {
        APPLOG("ContentProvider::privateLoadContent - found item in cache");
        return AsyncHttpQueue::Succeeded;
    }

    if (httpRequestType != FullAsyncRequest)
    {
        APPLOG("ContentProvider::privateLoadContent - httpRequestType != FullAsyncRequest");
        if (id.isAsset() || id.isItem() || id.isLevel() || id.isAppContent())
        {

            shared_ptr<const std::string> filename(new std::string(findAsset(id)));
            if (filename->empty())
                return AsyncHttpQueue::Failed;
            contentCache->insertCacheItem(id.toString(), CachedContent(filename));
            return contentCache->findCacheItem(id.toString(), result) ? AsyncHttpQueue::Succeeded : AsyncHttpQueue::Waiting;
        }

        if (id.isFile())
        {
            Aya::Security::Context::current().requirePermission(Aya::Security::WritePlayer, "ContentId");

            const std::string cfilename = id.toString().substr(7);
            boost::filesystem::path path;

            if (boost::filesystem::path(cfilename).is_absolute())
            {
                path = cfilename;
            }
            else
            {
                boost::filesystem::path path = cfilename;

                /*
                                struct stat buffer;
                                // Try once to find the path.  If it fails, try once more with the
                                // parent directory.  The logic for this is that RobloxStudio
                                // will look for scripts relative to the RBXL, but RobloxTest
                                // will look for scripts relative to the ProjectDir for RobloxTest,
                                // which is one higher than the RBXL.  So, in the case of
                                // RobloxStudio, we forcefully push one directory higher if the file
                                // wasn't found next to the RBXL.
                                if (-1 == stat(path.string().c_str(), &buffer))
                                {
                                    path = root / ".." / cfilename;
                                }*/
            }

            shared_ptr<const std::string> filename(new std::string(path.string()));
            contentCache->insertCacheItem(id.toString(), CachedContent(filename));
            return contentCache->findCacheItem(id.toString(), result) ? AsyncHttpQueue::Succeeded : AsyncHttpQueue::Waiting;
        }

        // If content is not http, try seeing if it's hashed on disk (according to Erik, for legacy embedded content feature)
        if (!id.isHttp())
        {
            // This will only work if contendId is actually a hash
            shared_ptr<const std::string> filename(new std::string(findHashFile(id)));
            if (!filename->empty())
            {
                contentCache->insertCacheItem(id.toString(), CachedContent(filename));
                return contentCache->findCacheItem(id.toString(), result) ? AsyncHttpQueue::Succeeded : AsyncHttpQueue::Waiting;
            }
        }
    }

    // If we get all the way here then try to request it for later use
    switch (httpRequestType)
    {
    case NoHttpRequest:
        break;
    case AsyncHttpRequest:
        if (!id.isHttp())
        {
            break;
        }
        // Fallthrough
    case FullAsyncRequest:
    {
        FASTLOGS(FLog::ContentProviderRequests, "Content requested: %s", id.c_str());
        APPLOG("ContentProvider::privateLoadContent - requestType = FullAsyncRequest");
        contentCache->asyncRequest(id.c_str(), priority, callback, jobType, false, expectedType);
        return AsyncHttpQueue::Waiting;
    }

    case SyncHttpRequest:
    {
        APPLOG("ContentProvider::privateLoadContent - requestType = SyncHttpRequest");
        if (contentCache->syncRequest(id.c_str(), expectedType))
        {
            return contentCache->findCacheItem(id.c_str(), result) ? AsyncHttpQueue::Succeeded : AsyncHttpQueue::Failed;
        }
        return AsyncHttpQueue::Waiting;
    }
    }

    return AsyncHttpQueue::Failed;
}

bool ContentProvider::isHttpUrl(const std::string& s)
{
    if (s.find("http://") == 0)
        return true;
    if (s.find("https://") == 0)
        return true;
    return false;
}

bool ContentProvider::assetFolderAlreadyInit = false;

fs::path ContentProvider::assetFolderPath;
fs::path ContentProvider::platformAssetFolderPath;
fs::path ContentProvider::itemsFolderPath;
fs::path ContentProvider::levelsFolderPath;
fs::path ContentProvider::modelsFolderPath;
fs::path ContentProvider::shadersFolderPath;
fs::path ContentProvider::pluginsFolderPath;
std::string ContentProvider::assetFolderString;
std::string ContentProvider::platformAssetFolderString;
std::string ContentProvider::itemsFolderString;
std::string ContentProvider::levelsFolderString;
std::string ContentProvider::modelsFolderString;
std::string ContentProvider::shadersFolderString;
std::string ContentProvider::pluginsFolderString;

std::string ContentProvider::assetFolder()
{
    return assetFolderString;
}

std::string ContentProvider::platformAssetFolder()
{
    return platformAssetFolderString;
}

std::string ContentProvider::itemsFolder()
{
    return itemsFolderString;
}
std::string ContentProvider::levelsFolder()
{
    return levelsFolderString;
}

std::string ContentProvider::modelsFolder()
{
    return modelsFolderString;
}
std::string ContentProvider::shadersFolder()
{
    return shadersFolderString;
}

std::string ContentProvider::pluginsFolder()
{
    return pluginsFolderString;
}

bool ContentProvider::isUrl(const std::string& s)
{
    if (isHttpUrl(s))
        return true;

    if ((s.find("ayaasset://") == 0 || s.find("rbxasset://") == 0) && s.length() > std::string("ayaasset://").length())
        return true;
    if ((s.find("ayaassetid://") == 0 || s.find("rbxassetid://") == 0) && s.length() > std::string("ayaassetid://").length())
        return true;
    if (s.find("ayaitem://") == 0 && s.length() > std::string("ayaitem://").length())
        return true;
    if (s.find("ayalevel://") == 0 && s.length() > std::string("ayalevel://").length())
        return true;
    if (s.find("ayamodel://") == 0 && s.length() > std::string("ayamodel://").length())
        return true;

    if (ContentId(s).isNamedAsset())
        return true;
    return false;
}

void ContentProvider::blockingLoadInstances(ContentId id, std::vector<shared_ptr<Instance>>& instances)
{
    boost::shared_ptr<const std::string> str = getContentString(id);

    std::istringstream stream(*str);

    Serializer().loadInstances(stream, instances);
}

std::string ContentProvider::getFile(Aya::ContentId ticket)
{
    CachedContent item;
    if (!blockingLoadContent(ticket, &item) || !registerFile(ticket, &item))
        throw Aya::runtime_error("Unable to load %s", ticket.c_str());

    return *item.filename;
}

} // namespace Aya



#include <fstream>
#include <sstream>
#include "Utility/MD5Hasher.hpp"

namespace Aya
{
static fs::path getCachePath(const char* subFolder, bool createPath)
{
    // Returns something like "C:\Documents and Settings\All Users\Application Data\Roblox\Cache"
    if (createPath)
    {
        static const fs::path path = Aya::FileSystem::getCacheDirectory(true, subFolder);
        return path;
    }
    else
    {
        static const fs::path path = Aya::FileSystem::getCacheDirectory(false, subFolder);
        return path;
    }
}


static void appendSlashIfRequired(fs::path& path)
{
    std::string pathStr = path.string();
    if (pathStr[pathStr.length() - 1] != '/' && pathStr[pathStr.length() - 1] != '\\')
        path /= "/";
}

static fs::path getLocalCachePath(bool createPath)
{
    static Aya::atomic<int> caching;
    static bool isCached = false;
    static std::string cachedResult;

    if (isCached)
        return fs::path(cachedResult.c_str());

    fs::path path = getCachePath("ContentProvider", createPath);
    boost::system::error_code ec;
    if (!boost::filesystem::exists(path, ec))
        return "";

    appendSlashIfRequired(path);

    if (caching.compare_and_swap(1, 0) == 0)
    {
        cachedResult = std::string(path.string());
        isCached = true;
    }
    return path;
}

Aya::ContentId ContentProvider::registerContent(std::istream& content)
{
    Aya::ContentId contentId;
    {
        // Get the hash
        boost::scoped_ptr<MD5Hasher> hasher(MD5Hasher::create());
        hasher->addData(content);
        contentId = Aya::ContentId(hasher->toString());
    }

    // Get/Create the local cache folder
    fs::path filePath = getLocalCachePath(true);
    filePath /= contentId.c_str();
    boost::system::error_code ec;
    if (!boost::filesystem::exists(filePath, ec))
    {
        // TODO: Threading risk here!!!!

        // Save it to the store
        std::ofstream outStream(filePath.string().c_str(), std::ios_base::out | std::ios::binary);

        content.clear();
        content.seekg(0, std::ios_base::beg);
        do
        {
            char buffer[1024];
            content.read(buffer, 1024);
            outStream.write(buffer, content.gcount());
        } while (content.gcount() > 0);
    }

    return contentId;
}

bool ContentProvider::isInSandbox(const fs::path& path, const fs::path& sandbox)
{
    boost::system::error_code errBase, errPath;
    std::string canonicalPath = pathToString(fs::canonical(path, errPath));
    std::string canonicalBase = pathToString(fs::canonical(sandbox, errBase));
    if (errBase || errPath)
    {
        return false;
    }
    else if (canonicalPath.compare(0, canonicalBase.size(), canonicalBase) != 0)
    {
        return false;
    }
    return true;
}

std::string ContentProvider::findAsset(Aya::ContentId contentId)
{
    AYAASSERT(contentId.isAsset() || contentId.isItem() || contentId.isLevel() || contentId.isAppContent());
    const char* pathName = contentId.c_str();

    fs::path folder = platformAssetFolderPath;
    fs::path queryName(pathName);
    fs::path filePath = platformAssetFolderPath;

    boost::system::error_code ec;

    if (contentId.isAsset())
    {
        pathName += 11;
        filePath /= pathName;

        if (!boost::filesystem::exists(filePath, ec))
        {
            folder = assetFolderPath;
            filePath = assetFolderPath / pathName;
            if (!boost::filesystem::exists(filePath, ec))
                return "";
        }
    }
    else
    {
        if (contentId.isItem())
        {
            pathName += 10;
            filePath = itemsFolderPath / pathName;
        }
        else if (contentId.isLevel())
        {
            pathName += 11;
            filePath = levelsFolderPath / pathName;
        }
        else if (contentId.isModel())
        {
            pathName += 11;
            filePath = modelsFolderPath / pathName;
        }

        if (!boost::filesystem::exists(filePath, ec))
            return "";
    }

    if (isInSandbox(filePath, folder))
    {
        return pathToString(filePath);
    }

    return "";
}

std::string ContentProvider::findHashFile(Aya::ContentId contentId)
{
    // TODO: Confirm that this is a valid "hash" ID

    // Get/Create the local cache folder
    fs::path cachePath = getLocalCachePath(true);
    // TODO: Handle case where cachePath==""

    fs::path filePath = cachePath;
    filePath /= contentId.c_str();

    if (!isInSandbox(filePath, cachePath))
    {
        return "";
    }

    boost::system::error_code ec;
    if (!boost::filesystem::exists(filePath, ec))
        return "";

    return pathToString(filePath);
}

void ContentProvider::setAssetFolder(const char* sPath)
{
    // if (!assetFolderAlreadyInit)
    {
        fs::path path = fs::system_complete(stringToPath(sPath));

        if (!fs::exists(path))
            throw Aya::runtime_error("The path '%s' does not exist", path.string().c_str());
        if (!is_directory(path))
            throw Aya::runtime_error("'%s' is not a directory", path.string().c_str());

        appendSlashIfRequired(path);

        fs::path platformAssetFolderModifier = "../platform/";
        fs::path itemsFolderModifier = "../items/";
        fs::path levelsFolderModifier = "../levels/";
        fs::path modelsFolderModifier = "../models/";
        fs::path shadersFolderModifier = "../shaders/";
        fs::path pluginsFolderModifier = "../plugins/";

        assetFolderString = pathToString(path);
        platformAssetFolderString = assetFolderString + platformAssetFolderModifier.string();
        itemsFolderString = assetFolderString + itemsFolderModifier.string();
        levelsFolderString = assetFolderString + levelsFolderModifier.string();
        modelsFolderString = assetFolderString + modelsFolderModifier.string();
        shadersFolderString = assetFolderString + shadersFolderModifier.string();
        pluginsFolderString = assetFolderString + pluginsFolderModifier.string();

        Aya::StandardOut::singleton()->printf(Aya::MESSAGE_SYSTEM, "Set asset folder path to %s", assetFolderString.c_str());

        try
        {
            assetFolderPath = fs::canonical(path);
            platformAssetFolderPath = fs::canonical(path / platformAssetFolderModifier);
            itemsFolderPath = fs::canonical(path / itemsFolderModifier);
            levelsFolderPath = fs::canonical(path / levelsFolderModifier);
            modelsFolderPath = fs::canonical(path / modelsFolderModifier);
            shadersFolderPath = fs::canonical(path / shadersFolderModifier);
            pluginsFolderPath = fs::canonical(path / pluginsFolderModifier);
        }
        catch (fs::filesystem_error& e)
        {
            // it means one of the paths didn't exist, but luckily we don't care
        }

        assetFolderAlreadyInit = true;
    }
}

void ContentProvider::setPlatformAssetFolder(const char* sPath)
{
    fs::path path = fs::system_complete(stringToPath(sPath));

    if (!fs::exists(path))
        throw Aya::runtime_error("The path '%s' does not exist", path.string().c_str());
    if (!is_directory(path))
        throw Aya::runtime_error("'%s' is not a directory", path.string().c_str());

    appendSlashIfRequired(path);

    platformAssetFolderString = pathToString(path);
    platformAssetFolderPath = fs::canonical(path);

    Aya::StandardOut::singleton()->printf(Aya::MESSAGE_SYSTEM, "Set platform asset folder path to %s", platformAssetFolderString.c_str());
}

void ContentProvider::setItemsFolder(const char* sPath)
{
    fs::path path = fs::system_complete(stringToPath(sPath));

    if (!fs::exists(path))
        throw Aya::runtime_error("The path '%s' does not exist", path.string().c_str());
    if (!is_directory(path))
        throw Aya::runtime_error("'%s' is not a directory", path.string().c_str());

    appendSlashIfRequired(path);

    itemsFolderString = pathToString(path);
    itemsFolderPath = fs::canonical(path);

    Aya::StandardOut::singleton()->printf(Aya::MESSAGE_SYSTEM, "Set items folder path to %s", itemsFolderString.c_str());
}

void ContentProvider::setLevelsFolder(const char* sPath)
{
    fs::path path = fs::system_complete(stringToPath(sPath));

    if (!fs::exists(path))
        throw Aya::runtime_error("The path '%s' does not exist", path.string().c_str());
    if (!is_directory(path))
        throw Aya::runtime_error("'%s' is not a directory", path.string().c_str());

    appendSlashIfRequired(path);

    levelsFolderString = pathToString(path);
    levelsFolderPath = fs::canonical(path);

    Aya::StandardOut::singleton()->printf(Aya::MESSAGE_SYSTEM, "Set levels folder path to %s", levelsFolderString.c_str());
}

void ContentProvider::setModelsFolder(const char* sPath)
{
    fs::path path = fs::system_complete(stringToPath(sPath));

    if (!fs::exists(path))
        throw Aya::runtime_error("The path '%s' does not exist", path.string().c_str());
    if (!is_directory(path))
        throw Aya::runtime_error("'%s' is not a directory", path.string().c_str());

    appendSlashIfRequired(path);

    modelsFolderString = pathToString(path);
    modelsFolderPath = fs::canonical(path);

    Aya::StandardOut::singleton()->printf(Aya::MESSAGE_SYSTEM, "Set models folder path to %s", modelsFolderString.c_str());
}

void ContentProvider::setShadersFolder(const char* folder)
{
    fs::path path = fs::system_complete(stringToPath(folder));

    if (!fs::exists(path))
        throw Aya::runtime_error("The path '%s' does not exist", path.string().c_str());
    if (!is_directory(path))
        throw Aya::runtime_error("'%s' is not a directory", path.string().c_str());

    appendSlashIfRequired(path);

    shadersFolderString = pathToString(path);
    shadersFolderPath = fs::canonical(path);

    Aya::StandardOut::singleton()->printf(Aya::MESSAGE_SYSTEM, "Set shaders folder path to %s", shadersFolderString.c_str());
}

void ContentProvider::setPluginsFolder(const char* folder)
{
    fs::path path = fs::system_complete(stringToPath(folder));

    if (!fs::exists(path))
        throw Aya::runtime_error("The path '%s' does not exist", path.string().c_str());
    if (!is_directory(path))
        throw Aya::runtime_error("'%s' is not a directory", path.string().c_str());

    appendSlashIfRequired(path);

    pluginsFolderString = pathToString(path);
    pluginsFolderPath = fs::canonical(path);

    Aya::StandardOut::singleton()->printf(Aya::MESSAGE_SYSTEM, "Set plugins folder path to %s", pluginsFolderString.c_str());
}

std::auto_ptr<std::istream> ContentProvider::getContent(const Aya::ContentId& ticket, const std::string& expectedType)
{
    APPLOG("ContentProvider::getContent - ticket");

    CachedContent item;
    if (!blockingLoadContent(ticket, &item, expectedType))
        throw Aya::runtime_error("Unable to load %s", ticket.c_str());

    if (item.data)
    {
        return std::auto_ptr<std::istream>(new std::istringstream(*item.data));
    }
    else
    {
        std::ifstream* stream = new std::ifstream(utf8_decode(*item.filename).c_str(), std::ios_base::in | std::ios_base::binary);
        return std::auto_ptr<std::istream>(stream);
    }
}
} // namespace Aya

void SetAssetFolder(const std::string& path)
{
    Aya::ContentProvider::setAssetFolder(path.c_str());
}

std::string GetAssetFolder()
{
    return Aya::ContentProvider::assetFolder();
}

void SetPlatformAssetFolder(const std::string& path)
{
    Aya::ContentProvider::setPlatformAssetFolder(path.c_str());
}

std::string GetPlatformAssetFolder()
{
    return Aya::ContentProvider::platformAssetFolder();
}

void SetItemsFolder(const std::string& path)
{
    Aya::ContentProvider::setItemsFolder(path.c_str());
}

std::string GetItemsFolder()
{
    return Aya::ContentProvider::itemsFolder();
}

void SetLevelsFolder(const std::string& path)
{
    Aya::ContentProvider::setLevelsFolder(path.c_str());
}

std::string GetLevelsFolder()
{
    return Aya::ContentProvider::levelsFolder();
}

void SetModelsFolder(const std::string& path)
{
    Aya::ContentProvider::setModelsFolder(path.c_str());
}

std::string GetModelsFolder()
{
    return Aya::ContentProvider::modelsFolder();
}

void SetShadersFolder(const std::string& folder)
{
    Aya::ContentProvider::setShadersFolder(folder.c_str());
}

std::string GetShadersFolder()
{
    return Aya::ContentProvider::shadersFolder();
}

void SetPluginsFolder(const std::string& folder)
{
    Aya::ContentProvider::setPluginsFolder(folder.c_str());
}
