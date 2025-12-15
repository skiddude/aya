#pragma once

#include "stdio.h"
#include "Utility/Name.hpp"
#include <string>
#include <istream>
#include <memory>
#include <vector>
#include "boost.hpp"
#include "time.hpp"
#include "Utility/ContentId.hpp"
#include "Utility/HeartbeatInstance.hpp"
#include "Utility/ThreadPool.hpp"
#include "Utility/AsyncHttpCache.hpp"
#include "Utility/LRUCache.hpp"
#include "Tree/Service.hpp"
#include "Log.hpp"
#include "Utility/ProtectedString.hpp"
#include "boost/filesystem.hpp"
#include "boost/optional.hpp"


void SetAssetFolder(const std::string& folder);
std::string GetAssetFolder();

void SetPlatformAssetFolder(const std::string& folder);
std::string GetPlatformAssetFolder();

void SetItemsFolder(const std::string& folder);
std::string GetItemsFolder();

void SetLevelsFolder(const std::string& folder);
std::string GetLevelsFolder();

void SetModelsFolder(const std::string& folder);
std::string GetModelsFolder();

void SetShadersFolder(const std::string& folder);
std::string GetShadersFolder();

void SetPluginsFolder(const std::string& folder);
std::string GetPluginsFolder();

namespace Aya
{

class AssetFetchMediator;
class Instance;
class Http;

extern const char* const sContentProvider;
class ContentProvider
    : public DescribedNonCreatable<ContentProvider, Instance, sContentProvider, Reflection::ClassDescriptor::RUNTIME_LOCAL>
    , public Service
    , public HeartbeatInstance
{
private:
    typedef DescribedNonCreatable<ContentProvider, Instance, sContentProvider, Reflection::ClassDescriptor::RUNTIME_LOCAL> Super;

public:
    static Log* appLog;
    static Aya::mutex* appLogLock;

    static float PRIORITY_DEFAULT;

    static float PRIORITY_MFC;
    static float PRIORITY_SCRIPT;
    static float PRIORITY_MESH;
    static float PRIORITY_SOLIDMODEL;
    static float PRIORITY_INSERT;
    static float PRIORITY_CHARACTER;
    static float PRIORITY_ANIMATION;
    static float PRIORITY_TEXTURE;
    static float PRIORITY_DECAL;
    static float PRIORITY_SOUND;
    static float PRIORITY_GUI;
    static float PRIORITY_SKY;

private:
    boost::shared_ptr<boost::thread> legacyContentCleanupThread;
    boost::shared_ptr<boost::thread> contentCleanupThread;

    static boost::filesystem::path assetFolderPath;
    static boost::filesystem::path platformAssetFolderPath;
    static boost::filesystem::path itemsFolderPath;
    static boost::filesystem::path levelsFolderPath;
    static boost::filesystem::path modelsFolderPath;
    static boost::filesystem::path shadersFolderPath;
    static boost::filesystem::path pluginsFolderPath;
    static std::string assetFolderString;
    static std::string platformAssetFolderString;
    static std::string itemsFolderString;
    static std::string levelsFolderString;
    static std::string modelsFolderString;
    static std::string shadersFolderString;
    static std::string pluginsFolderString;
    static bool assetFolderAlreadyInit;

    struct CachedContent
    {
        shared_ptr<const std::string> data;
        shared_ptr<const std::string> filename;
        CachedContent() {}
        CachedContent(shared_ptr<const std::string> filename)
            : filename(filename)
        {
        }
        CachedContent(shared_ptr<const std::string> data, shared_ptr<const std::string> filename)
            : data(data)
            , filename(filename)
        {
        }
    };
    boost::shared_ptr<AsyncHttpCache<CachedContent>> contentCache;

    class PreloadAsyncRequest
    {
    public:
        int outstanding;
        int failed;

        PreloadAsyncRequest()
            : outstanding(0)
            , failed(0)
        {
        }

        PreloadAsyncRequest(int requestCount)
            : outstanding(requestCount)
            , failed(0)
        {
        }
    };

public:
    ContentProvider();
    ~ContentProvider();

    static ContentId registerContent(std::istream& stream);

    bool isUrlBad(Aya::ContentId id);

    void blockingLoadInstances(ContentId id, std::vector<shared_ptr<Instance>>& instances);

    bool isRequestQueueEmpty();

    const std::string& getInstanceName() const;
    const std::string& getInstanceCurrency() const;
    const std::string& getInstanceMotd() const;

    const std::string& getBaseUrl() const;
    const std::string getApiBaseUrl() const;
    const std::string getUnsecureApiBaseUrl() const;
    static std::string getApiBaseUrl(const std::string& baseUrl);
    static std::string getUnsecureApiBaseUrl(const std::string& baseUrl);

    void setInstanceName(std::string name);
    void setInstanceCurrency(std::string currency);
    void setInstanceMotd(std::string motd);
    void setBaseUrl(std::string url);
    void setThreadPool(int count);
    void setCacheSize(int count);
    void preloadContentWithCallback(Aya::ContentId id, float priority, boost::function<void(AsyncHttpQueue::RequestResult)> callback,
        AsyncHttpQueue::ResultJob jobType = AsyncHttpQueue::AsyncInline, const std::string& expectedType = "");
    void preloadContent(Aya::ContentId id);

    void preloadContentBlockingList(
        shared_ptr<const Reflection::ValueArray> idList, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction);
    static boost::mutex preloadContentBlockingMutex;
    void preloadContentBlockingListHelper(AsyncHttpQueue::RequestResult results, boost::function<void()> resumeFunction,
        boost::function<void(std::string)> errorFunction, PreloadAsyncRequest* pRequest, ContentId id);
    void preloadContentResultCallback(AsyncHttpQueue::RequestResult results, ContentId id);

    void clearContent();
    void invalidateCache(ContentId contentId);

    // returns true if the content is available.
    bool hasContent(const ContentId& id);

    // returns non-NULL if the content is available. If not available, the provider does an asynchronous request of the content for later
    shared_ptr<const std::string> requestContentString(const ContentId& id, float priority);

    // Async request
    void getContent(const Aya::ContentId& id, float priority, AsyncHttpQueue::RequestCallback callback,
        AsyncHttpQueue::ResultJob jobType = AsyncHttpQueue::AsyncInline, const std::string& expectedType = "");
    void loadContent(const Aya::ContentId& id, float priority,
        boost::function<void(AsyncHttpQueue::RequestResult, shared_ptr<Instances>, shared_ptr<std::exception>)> callback,
        AsyncHttpQueue::ResultJob jobType = AsyncHttpQueue::AsyncInline);
    void loadContentString(const Aya::ContentId& id, float priority,
        boost::function<void(AsyncHttpQueue::RequestResult, shared_ptr<const std::string>, shared_ptr<std::exception>)> callback,
        AsyncHttpQueue::ResultJob jobType = AsyncHttpQueue::AsyncInline);

    // The following functions throw exceptions upon failure or return NULL
    shared_ptr<const std::string> getContentString(ContentId id);
    std::auto_ptr<std::istream> getContent(const ContentId& contentId, const std::string& expectedType = "");
    std::string getFile(ContentId contentId);
    static std::string getAssetFile(const char* filePath);

    // throws an exception
    static void verifyRequestedScriptSignature(const ProtectedString& source, const std::string& assetId, bool required);
    static void verifyScriptSignature(const ProtectedString& source, bool required);

    int getRequestQueueSize() const;
    shared_ptr<const Reflection::ValueArray> getFailedUrls();
    shared_ptr<const Reflection::ValueArray> getRequestQueueUrls();
    shared_ptr<const Reflection::ValueArray> getRequestedUrls();

    static void setAssetFolder(const char* path);
    static void setPlatformAssetFolder(const char* path);
    static void setLevelsFolder(const char* path);
    static void setItemsFolder(const char* path);
    static void setModelsFolder(const char* path);
    static void setShadersFolder(const char* path);
    static void setPluginsFolder(const char* path);
    static std::string getAssetFolder()
    {
        return assetFolderPath.string();
    };
    static std::string assetFolder();
    static std::string platformAssetFolder();
    static std::string getPlatformAssetFolder()
    {
        return platformAssetFolderPath.string();
    };
    static std::string itemsFolder();
    static std::string getItemsFolder()
    {
        return itemsFolderPath.string();
    };
    static std::string levelsFolder();
    static std::string getLevelsFolder()
    {
        return levelsFolderPath.string();
    };
    static std::string modelsFolder();
    static std::string getModelsFolder()
    {
        return modelsFolderPath.string();
    };
    static std::string shadersFolder();
    static std::string getShadersFolder()
    {
        return shadersFolderPath.string();
    };
    static std::string pluginsFolder();
    static std::string getPluginsFolder()
    {
        return pluginsFolderPath.string();
    };

    static bool isUrl(const std::string& s);
    static bool isHttpUrl(const std::string& s);

    static std::string findAsset(Aya::ContentId contentId);

    static Reflection::PropDescriptor<ContentProvider, std::string> desc_baseUrl;
    static Reflection::PropDescriptor<ContentProvider, std::string> desc_instanceName;
    static Reflection::PropDescriptor<ContentProvider, std::string> desc_instanceCurrency;
    static Reflection::PropDescriptor<ContentProvider, std::string> desc_instanceMotd;

    /*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
    {
        contentCache->resetStatsItem(newProvider);
        Super::onServiceProvider(oldProvider, newProvider);
        onServiceProviderHeartbeatInstance(oldProvider, newProvider); // hooks up heartbeat
    }
    /*implement*/ virtual void onHeartbeat(const Heartbeat& event);

    void printContentNames()
    {
        contentCache->printContentNames();
    }

    void setAssetFetchMediator(AssetFetchMediator* afm);


private:
    // FullAsyncRequest: all requests (including disk requests) can be asynchronous. disk requests are returned as memory streams, exactly like http
    // responses.
    typedef enum
    {
        NoHttpRequest,
        AsyncHttpRequest,
        SyncHttpRequest,
        FullAsyncRequest
    } RequestType;

    bool clearFinishFlag;
    AssetFetchMediator* afm;

    std::string baseUrl;
    std::string instanceName;
    std::string instanceCurrency;
    std::string instanceMotd;

    bool isContentLoaded(ContentId id);
    bool blockingLoadContent(ContentId id, CachedContent* result, const std::string& expectedType = "");
    AsyncHttpQueue::RequestResult privateLoadContent(ContentId& id, RequestType httpRequestType, float priority, CachedContent* result,
        AsyncHttpQueue::RequestCallback* callback, AsyncHttpQueue::ResultJob jobType = AsyncHttpQueue::AsyncInline,
        const std::string& expectedType = "");

    static std::string findHashFile(ContentId contentId);
    static bool findLocalFile(const std::string& url, std::string* filename);

    bool registerFile(const ContentId& id, CachedContent* item);

    static bool isInSandbox(const boost::filesystem::path& path, const boost::filesystem::path& sandbox);
};

class AssetFetchMediator
{
public:
    virtual boost::optional<std::string> findCachedAssetOrEmpty(const ContentId& contentId, int universeId) = 0;
};
} // namespace Aya
