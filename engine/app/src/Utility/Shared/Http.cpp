

// HACK! This avoids "nil" macro compatibility issues between MacOS SDK and Boost
#ifdef nil
#undef nil
#endif

static const bool kSkipTrustCheck = false;

#include <algorithm>

#include "format.hpp"

#include "Utility/Http.hpp"
#include "Utility/HttpPlatformImpl.hpp"

#include "Utility/URL.hpp"
#include "Utility/StandardOut.hpp"
#include "Utility/Statistics.hpp"
#include "Utility/ThreadPool.hpp"

#include "TaskScheduler.hpp"

#include "DataModel/DataStore.hpp"
#include "DataModel/DebugSettings.hpp"
#include "DataModel/MarketplaceService.hpp"
#include "DataModel/Stats.hpp"
#include "Utility/RobloxServicesTools.hpp"



#if defined(_WIN32) && !defined(AYA_PLATFORM_DURANGO)
#include <atlutil.h>
#endif // defined(_WIN32) && !defined(AYA_PLATFORM_DURANGO)

LOGGROUP(Http)
DYNAMIC_FASTINT(ExternalHttpRequestSizeLimitKB)
DYNAMIC_FASTINTVARIABLE(HttpResponseDefaultTimeoutMillis, 60000)
DYNAMIC_FASTINTVARIABLE(HttpSendDefaultTimeoutMillis, 60000)
DYNAMIC_FASTINTVARIABLE(HttpConnectDefaultTimeoutMillis, 60000)
DYNAMIC_FASTINTVARIABLE(HttpDataSendDefaultTimeoutMillis, 60000)

DYNAMIC_LOGVARIABLE(HttpTrace, 0)
DYNAMIC_FASTINTVARIABLE(HttpSendStatsEveryXSeconds, 60)
DYNAMIC_FASTINTVARIABLE(HttpGAFailureReportPercent, 1)
DYNAMIC_FASTINTVARIABLE(HttpRBXEventFailureReportHundredthsPercent, 0)
DYNAMIC_FASTFLAGVARIABLE(DebugHttpAsyncCallsForStatsReporting, true)

DYNAMIC_FASTFLAGVARIABLE(UseAssetTypeHeader, false)

using namespace Aya;


namespace
{

static bool useCurlHttpImpl = true;

static const int kNumberThreadPoolThreads = 16;
static ThreadPool* threadPool;

bool isValidScheme(const char* scheme)
{
    return 0 == strncmp(scheme, "http", 4) || 0 == strncmp(scheme, "https", 5);
}

bool hasEnding(const std::string& fullString, const std::string& ending)
{
    if (fullString.length() >= ending.length())
    {
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    }
    else
    {
        return false;
    }
}

class HTTPStatistics
{
    static HTTPStatistics httpStatistics;

    boost::mutex mutex;

    double successDelays;
    double failureDelays;
    unsigned numSuccesses;
    unsigned numFailures;

    unsigned numDataStoreSuccesses;
    unsigned numDataStoreFailures;

    unsigned numMarketPlaceSuccesses;
    unsigned numMarketPlaceFailures;

    size_t successBytes;

    enum ServiceCategory
    {
        ServiceCategoryDataStore,
        ServiceCategoryMarketPlace,
        ServiceCategoryOther
    };

    struct PendingStat
    {
        std::string url;
        std::string reason;
        double delay;
        size_t size;
        int statusCode;

        PendingStat(const char* url, const char* reason, size_t bytes, double delay, int httpStatusCode)
            : url(url)
            , reason(reason)
            , delay(delay)
            , size(bytes)
            , statusCode(httpStatusCode)
        {
        }
    };

    std::list<PendingStat> pendingStats;

    HTTPStatistics()
        : successDelays(0.0)
        , failureDelays(0.0)
        , numSuccesses(0)
        , numFailures(0)
        , numDataStoreSuccesses(0)
        , numDataStoreFailures(0)
        , numMarketPlaceSuccesses(0)
        , numMarketPlaceFailures(0)
        , successBytes(0)
    {
    }

    void addSuccess(double delay, const char* url, size_t bytes)
    {
        const boost::mutex::scoped_lock lock(mutex);
        ++numSuccesses;
        successDelays += delay;
        successBytes += bytes;

        switch (getServiceCategory(url))
        {
        case ServiceCategoryDataStore:
            ++numDataStoreSuccesses;
            break;
        case ServiceCategoryMarketPlace:
            ++numMarketPlaceSuccesses;
            break;
        default:
            break;
        }
    }

    void addFailure(double delay, const char* url, const char* reason)
    {
        const boost::mutex::scoped_lock lock(mutex);
        ++numFailures;
        failureDelays += delay;

        switch (getServiceCategory(url))
        {
        case ServiceCategoryDataStore:
            ++numDataStoreFailures;
            break;
        case ServiceCategoryMarketPlace:
            ++numMarketPlaceFailures;
            break;
        default:
            break;
        }
    }

    void addPendingStat(double delay, const char* url, size_t bytes, const char* reason, int httpStatusCode = 0)
    {
        const boost::mutex::scoped_lock lock(mutex);
        pendingStats.push_back(PendingStat(url, reason, bytes, delay, httpStatusCode));
    }

    void report() {}

    static ServiceCategory getServiceCategory(const char* url)
    {
        Aya::Url parsed = Aya::Url::fromString(url);
        if (parsed.pathEquals(DataStore::urlApiPath()))
        {
            return ServiceCategoryDataStore;
        }

        if (parsed.pathEquals(MarketplaceService::urlApiPath()))
        {
            return ServiceCategoryMarketPlace;
        }
        return ServiceCategoryOther;
    }

    static const char* serviceToString(const char* url)
    {
        switch (getServiceCategory(url))
        {
        case ServiceCategoryDataStore:
            return "DataStore";
        case ServiceCategoryMarketPlace:
            return "MarketPlace";
        default:
            return "Unknown";
        }
    }


public:
    static void reportingThreadHandler()
    {
        while (true)
        {
            boost::this_thread::sleep(boost::posix_time::seconds(DFInt::HttpSendStatsEveryXSeconds));
            httpStatistics.report();
        }
    }

    static void success(double delay, const char* url, size_t bytes)
    {
        httpStatistics.addSuccess(delay, url, bytes);
        httpStatistics.addPendingStat(delay, url, bytes, "No error");
    }

    static void failure(double delay, const char* url, size_t bytes, const char* reason, int httpStatusCode = 0)
    {
        httpStatistics.addFailure(delay, url, reason);
        httpStatistics.addPendingStat(delay, url, bytes, reason, httpStatusCode);
    }
}; // struct CacheStatistics
HTTPStatistics HTTPStatistics::httpStatistics;
} // namespace

namespace Aya
{
static const int kWindowSize = 256;
std::string Http::accessKey;
std::string Http::gameSessionID;
std::string Http::gameID;
std::string Http::placeID;
std::string Http::requester = "Client";
#if defined(__APPLE__) && !defined(AYA_PLATFORM_IOS)
std::string Http::rbxUserAgent = "Roblox/Darwin";
#elif defined(AYA_PLATFORM_DURANGO)
std::string Http::rbxUserAgent = "Roblox/XboxOne";
#elif defined(_WIN32)
std::string Http::rbxUserAgent = "Roblox/WinInet";
#else
std::string Http::rbxUserAgent;
#endif

int Http::playerCount = 0;
bool Http::useDefaultTimeouts = true;
Http::CookieSharingPolicy Http::cookieSharingPolicy;

#ifdef _WIN32
Http::API Http::defaultApi = Http::WinHttp;
#else
Http::API Http::defaultApi = Http::Uninitialized;
#endif

Aya::atomic<int> Http::cdnSuccessCount = 0;
Aya::atomic<int> Http::cdnFailureCount = 0;
Aya::atomic<int> Http::alternateCdnSuccessCount = 0;
Aya::atomic<int> Http::alternateCdnFailureCount = 0;
double Http::lastCdnFailureTimeSpan = 0;
Aya::atomic<int> Http::robloxSuccessCount = 0;
Aya::atomic<int> Http::robloxFailureCount = 0;
WindowAverage<double, double> Http::robloxResponce(kWindowSize);
WindowAverage<double, double> Http::cdnResponce(kWindowSize);

Aya::mutex* Http::robloxResponceLock = NULL;
Aya::mutex* Http::cdnResponceLock = NULL;

Http::MutexGuard Http::lockGuard;
} // namespace Aya
Http::MutexGuard::MutexGuard()
{
    robloxResponceLock = new Aya::mutex();
    cdnResponceLock = new Aya::mutex();
}

Http::MutexGuard::~MutexGuard()
{
    delete robloxResponceLock;
    robloxResponceLock = NULL;

    delete cdnResponceLock;
    cdnResponceLock = NULL;
}

Aya::mutex* Http::getRobloxResponceLock()
{
    return robloxResponceLock;
}

Aya::mutex* Http::getCdnResponceLock()
{
    return cdnResponceLock;
}

http_status_error::http_status_error(int statusCode)
    : std::runtime_error(Aya::format("HTTP %d", statusCode))
    , statusCode(statusCode)
{
}

http_status_error::http_status_error(int statusCode, const std::string& message)
    : std::runtime_error(Aya::format("HTTP %d (%s)", statusCode, message.c_str()))
    , statusCode(statusCode)
{
}

void Http::init()
{
    recordStatistics = true;
    shouldRetry = true;
    cachePolicy = HttpCache::PolicyDefault;
    doNotUseCachedResponse = false;
    connectTimeoutMillis = DFInt::HttpConnectDefaultTimeoutMillis;
    responseTimeoutMillis = DFInt::HttpResponseDefaultTimeoutMillis;
    sendTimeoutMillis = DFInt::HttpSendDefaultTimeoutMillis;
    dataSendTimeoutMillis = DFInt::HttpDataSendDefaultTimeoutMillis;
}

void Http::init(API api, CookieSharingPolicy sharingPolicy)
{
    static boost::scoped_ptr<ThreadPool> tp(new ThreadPool(kNumberThreadPoolThreads));
    threadPool = tp.get();
    Http::defaultApi = api;
    FASTLOG2(FLog::Http, "Http initialization M1 = 0x%x M2=0x%x", &Http::robloxResponceLock, &Http::cdnResponceLock);
    cookieSharingPolicy = sharingPolicy;
    Http::SetUseCurl(true);
}

void Http::SetUseStatistics(bool value)
{
    if (value)
    {
        boost::function0<void> f = boost::bind(&HTTPStatistics::reportingThreadHandler);
        boost::function0<void> g = boost::bind(&StandardOut::print_exception, f, MESSAGE_ERROR, false);
        boost::thread(thread_wrapper(g, "aya_http_stats_report"));
    }
}

void Http::SetUseCurl(bool value)
{
    FASTLOGS(DFLog::HttpTrace, "Use the new http api: %s", value ? "yes" : "no");
    useCurlHttpImpl = value;

    if (useCurlHttpImpl)
    {
        static boost::mutex mutex;
        static bool initialized = false;

        boost::mutex::scoped_lock lock(mutex);

        if (!initialized)
        {
            HttpPlatformImpl::init(cookieSharingPolicy);
            initialized = true;
        }
    }
}

void Http::setCookiesForDomain(const std::string& domain, const std::string& cookies)
{
    HttpPlatformImpl::setCookiesForDomain(domain, cookies);
}

void Http::getCookiesForDomain(const std::string& domain, std::string& cookies)
{
    HttpPlatformImpl::getCookiesForDomain(domain, cookies);
}

void Http::ThrowIfFailure(bool success, const char* message)
{
    ThrowIfFailure(success, url.c_str(), message);
}

void Http::ThrowIfFailure(bool success, const char* url, const char* message)
{
    if (!success)
    {
#if defined(_WIN32) && !defined(AYA_PLATFORM_DURANGO)
        ThrowLastError(GetLastError(), url, message);
#else
        throw Aya::runtime_error("%s: %s", url, message);
#endif
    }
}

#if defined(_WIN32) && !defined(AYA_PLATFORM_DURANGO)
void Http::ThrowLastError(int err, const char* url, const char* message)
{
    TCHAR buffer[256];
    if (::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT), buffer, 256, NULL) == 0)
        throw Aya::runtime_error("%s: %s, err=0x%X", url, message, err);
    else
        throw Aya::runtime_error("%s: %s, %s", url, message, buffer);
}
#endif // ifdef _WIN32


#if defined(AYA_PLATFORM_DURANGO)
void dprintf(const char* fmt, ...);
#endif

void Http::httpGetPost(bool isPost, std::istream& dataStream, const std::string& contentType, bool compressData,
    const HttpAux::AdditionalHeaders& additionalHeaders, bool externalRequest, std::string& response, bool forceNativeHttp)
{
    Aya::Timer<Aya::Time::Fast> httpTimer;

    AYAASSERT(isPost == !contentType.empty());

    ThrowIfFailure(trustCheck(url.c_str(), externalRequest), "Trust check failed");

    if (externalRequest)
    {
        if (isPost)
        {
            AYAASSERT(0 == dataStream.tellg());
            dataStream.seekg(0, std::ios::end);
            size_t length = dataStream.tellg();
            dataStream.seekg(0, std::ios::beg);
            if ((length / 1024) >= static_cast<size_t>(DFInt::ExternalHttpRequestSizeLimitKB))
            {
                throw Aya::runtime_error(
                    "Post data too large. Limit: %d KB. Post size: %d KB", DFInt::ExternalHttpRequestSizeLimitKB, static_cast<int>(length / 1024));
            }
        }
    }

    HttpAux::AdditionalHeaders headers = additionalHeaders;

    if (!externalRequest)
    {
        if (Http::gameSessionID.length() > 0 && headers.end() == headers.find(kGameSessionHeaderKey))
        {
            headers[kGameSessionHeaderKey] = Http::gameSessionID;
        }

        if (Http::gameID.length() > 0 && headers.end() == headers.find(kGameIdHeaderKey))
        {
            // causes 400???? ==> what the hell is a  character
            // headers[kGameIdHeaderKey] = Http::gameID;
            // it's not like we're ever going to use this thing
        }

        if (Http::placeID.length() > 0 && headers.end() == headers.find(kPlaceIdHeaderKey))
        {
            headers[kPlaceIdHeaderKey] = Http::placeID;
        }

        if (Http::requester.length() > 0 && headers.end() == headers.find(kRequesterHeaderKey))
        {
            headers[kRequesterHeaderKey] = Http::requester;
        }
    }

    HttpPlatformImpl::HttpOptions httpOpts(url, externalRequest, cachePolicy, connectTimeoutMillis, responseTimeoutMillis);
    if (isPost)
    {
        httpOpts.setPostData(&dataStream, compressData);
    }
    httpOpts.setHeaders(&contentType, &headers);

    try
    {
        HttpPlatformImpl::perform(httpOpts, response);
        if (recordStatistics)
        {
            HTTPStatistics::success(httpTimer.delta().msec(), url.c_str(), response.size());
        }
    }
    catch (const Aya::http_status_error& e)
    {
        bool didMakeNativeRequest =
            doHttpGetPostWithNativeFallbackForReporting(isPost, dataStream, contentType, compressData, additionalHeaders, externalRequest, response);

        if (recordStatistics)
        {
            HTTPStatistics::failure(httpTimer.delta().msec(), url.c_str(), response.size(), e.what(), e.statusCode);
        }
        if (!didMakeNativeRequest)
            throw;
    }
    catch (const std::exception& e)
    {
        if (recordStatistics)
        {
            HTTPStatistics::failure(httpTimer.delta().msec(), url.c_str(), response.size(), e.what());
        }
        throw;
    }
}

void Http::post(std::istream& input, const std::string& contentType, bool compress, std::string& response, bool externalRequest)
{
    httpGetPost(true, input, contentType, compress, additionalHeaders, externalRequest, response);
}

static void doGet(Http http, bool externalRequest, boost::function<void(std::string*, std::exception*)> handler)
{
    std::string response;
    try
    {
        http.get(response, externalRequest);
    }
    catch (Aya::base_exception& ex)
    {
        handler(0, &ex);
        return;
    }
    handler(&response, 0);
}

void Http::get(boost::function<void(std::string*, std::exception*)> handler, bool externalRequest)
{
    threadPool->schedule(boost::bind(&doGet, *this, externalRequest, handler));
}

#if defined(_WIN32)
void Http::onWinHttpRedirect(unsigned long dwInternetStatus, std::string redirectUrl)
{
    // This function determines if we are redirecting to a CDN.
    // If so, then record an alternate URL in case the CDN fails.
    // The alternate URL is the corresponding Amazon S3 URL.
    // bitgravity:
    int pos = redirectUrl.find("bg.roblox");
    if (pos != std::string::npos)
    {
        alternateUrl = redirectUrl.substr(0, pos);
        alternateUrl += redirectUrl.substr(pos + 2);
        return;
    }

    // cloudfront:
    pos = redirectUrl.find("-cf.roblox");
    if (pos != std::string::npos)
    {
        alternateUrl = redirectUrl.substr(0, pos);
        alternateUrl += redirectUrl.substr(pos + 3);
        return;
    }
}
#endif // ifdef _WIN32

static void doPostStream(std::string url, boost::shared_ptr<std::istream> input, const std::string& contentType, bool compress, bool externalRequest,
    bool recordStatistics, boost::function<void(std::string*, std::exception*)> handler)
{
    std::string response;
    try
    {
        Http http(url);
        http.recordStatistics = recordStatistics;
        http.post(*input, contentType, compress, response, externalRequest);
    }
    catch (Aya::base_exception& ex)
    {
        handler(0, &ex);
        return;
    }
    handler(&response, 0);
}

static void doPost(std::string url, std::string input, const std::string& contentType, bool compress, bool externalRequest, bool recordStatistics,
    boost::function<void(std::string*, std::exception*)> handler)
{
    shared_ptr<std::istream> inputStream(new std::istringstream(input));
    doPostStream(url, inputStream, contentType, compress, externalRequest, recordStatistics, handler);
}

void Http::post(const std::string& input, const std::string& contentType, bool compress, boost::function<void(std::string*, std::exception*)> handler,
    bool externalRequest)
{
    threadPool->schedule(boost::bind(&doPost, url, input, contentType, compress, externalRequest, recordStatistics, handler));
}

void Http::post(boost::shared_ptr<std::istream> input, const std::string& contentType, bool compress,
    boost::function<void(std::string*, std::exception*)> handler, bool externalRequest)
{
    threadPool->schedule(boost::bind(&doPostStream, url, input, contentType, compress, externalRequest, recordStatistics, handler));
}

void Http::get(std::string& response, bool externalRequest)
{
    Time startTime = Time::now<Time::Fast>();

    std::istringstream dummy;
    try
    {
        httpGetPost(false, dummy, "", false, additionalHeaders, externalRequest, response);
        if (!alternateUrl.empty())
            ++cdnSuccessCount;
    }
    catch (Aya::base_exception& e)
    {
        if (shouldRetry)
        {
            shouldRetry = false;

            response.clear();
            const double elapsed = (Time::now<Time::Fast>() - startTime).seconds();
            if (!alternateUrl.empty())
            {
                ++cdnFailureCount;
                lastCdnFailureTimeSpan = elapsed;
                Aya::StandardOut::singleton()->printf(Aya::MESSAGE_WARNING, "httpGet %s failed. Trying alternate %s. Error: %s.  Elapsed time: %g",
                    url.c_str(), alternateUrl.c_str(), e.what(), elapsed);
                try
                {
                    httpGetPost(false, dummy, "", false, additionalHeaders, externalRequest, response);
                    ++alternateCdnSuccessCount;
                }
                catch (Aya::base_exception&)
                {
                    ++alternateCdnFailureCount;
                    throw;
                }
            }
            else
            {
                Aya::StandardOut::singleton()->printf(
                    Aya::MESSAGE_WARNING, "httpGet %s failed. Trying again. Error: %s.  Elapsed time: %g", url.c_str(), e.what(), elapsed);

                httpGetPost(false, dummy, "", false, additionalHeaders, externalRequest, response);
            }
        }
        else
        {
            // rethrow the existing exception
            throw;
        }
    }
}


void Http::setProxy(const std::string& host, long port)
{
    HttpPlatformImpl::setProxy(host, port);
}

std::string Http::ws2s(const wchar_t* ws)
{
    std::string s;
#ifdef _WIN32
    int len = WideCharToMultiByte(CP_ACP, 0, ws, -1, NULL, 0, NULL, NULL);
    if (len > 0)
    {
        s.resize(len - 1);
        WideCharToMultiByte(CP_ACP, 0, ws, -1, &s[0], len, NULL, NULL);
    }
#else
    s.resize(strlen((char*)ws)); // HACK:
    int len = wcstombs(&s[0], ws, wcslen(ws));
#endif
    return s;
}


bool Http::isScript(const char* url)
{
    static const char* javascript = "javascript:";
    static const char* jscript = "jscript:";
    return 0 == strncmp(url, javascript, strlen(javascript)) || 0 == strncmp(url, jscript, strlen(jscript));
}


bool Http::isInstanceSite(const char* url)
{
    return Aya::Url::fromString(url).isSubdomainOf(Aya::Url::fromString(GetBaseURL()).host());
}

bool Http::trustCheck(const char* url, bool externalRequest)
{
    if (kSkipTrustCheck)
        return true;

    if ((!IsUsingTrustCheck() && !IsUsingInstance()) || IsInsecureMode())
        return true;

    if (IsUsingTrustCheck())
        return Aya::Url::fromString(url).isSubdomainOf(Aya::Url::fromString(GetTrustCheckURL()).host());

    return isInstanceSite(url) || isScript(url) || std::string("about:blank") == url || Aya::Url::fromString(url).isSubdomainOf("localhost");
}

bool Http::doHttpGetPostWithNativeFallbackForReporting(bool isPost, std::istream& dataStream, const std::string& contentType, bool compressData,
    const HttpAux::AdditionalHeaders& additionalHeaders, bool allowExternal, std::string& response)
{
    return false;
}

void Http::applyAdditionalHeaders(Aya::HttpAux::AdditionalHeaders& outHeaders)
{
    for (Aya::HttpAux::AdditionalHeaders::const_iterator itr = outHeaders.begin(); itr != outHeaders.end(); ++itr)
    {
        this->additionalHeaders[itr->first] = itr->second;
    }
}
