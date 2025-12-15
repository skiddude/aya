


#include "DataModel/Stats.hpp"
#include "DataModel/DataModel.hpp"
#include "Log.hpp"
#include "Utility/Http.hpp"
#include "Utility/Profiling.hpp"
#include "Utility/Statistics.hpp"
#include "DataModel/ContentProvider.hpp"
#include "DataModel/DebugSettings.hpp"
#include "Script/ScriptContext.hpp"
#include "format_string.hpp"
#include "Utility/RobloxServicesTools.hpp"
#include "Players.hpp"

#include <boost/algorithm/string.hpp>

#ifdef _WIN32
#include "Utility/FileSystem.hpp"
#elif __ANDROID__
namespace Aya
{
namespace JNI
{
extern std::string robloxVersion; // JNIMain.cpp
}
} // namespace Aya
#endif

#include <boost/algorithm/string/replace.hpp>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>



extern const char* const sTotalCountTimeIntervalItem = "TotalCountTimeIntervalItem";

class TotalCountTimeIntervalItem : public Aya::DescribedNonCreatable<TotalCountTimeIntervalItem, Aya::Stats::Item, sTotalCountTimeIntervalItem>
{
    const Aya::TotalCountTimeInterval<>& tcti;

public:
    TotalCountTimeIntervalItem(const Aya::TotalCountTimeInterval<>* tcti)
        : tcti(*tcti)
    {
    }
    virtual void update()
    {
        int v = tcti.getCount();
        if (val != v)
        {
            val = v;
            sValue = Aya::format("%d", v);
        }
    }
};

template<class T>
class RunningAverageItem : public Aya::Stats::Item
{
    const Aya::RunningAverage<T, double>& ra;

public:
    RunningAverageItem(const Aya::RunningAverage<T, double>* ra)
        : ra(*ra)
    {
    }

    virtual void update()
    {
        double v = ra.value();
        if (val != v)
        {
            val = v;
            sValue = Aya::format("%g (%.0f%%CV)", v, 100.0 * ra.coefficient_of_variation());
        }
    }
};

extern const char* const sRunningAverageItemInt = "RunningAverageItemInt";

class RunningAverageItemInt : public Aya::DescribedNonCreatable<RunningAverageItemInt, RunningAverageItem<int>, sRunningAverageItemInt>
{
public:
    RunningAverageItemInt(const Aya::RunningAverage<int, double>* ra)
        : Aya::DescribedNonCreatable<RunningAverageItemInt, RunningAverageItem<int>, sRunningAverageItemInt>(ra)
    {
    }
};


extern const char* const sRunningAverageItemDouble = "RunningAverageItemDouble";

class RunningAverageItemDouble : public Aya::DescribedNonCreatable<RunningAverageItemDouble, RunningAverageItem<double>, sRunningAverageItemDouble>
{
public:
    RunningAverageItemDouble(const Aya::RunningAverage<double, double>* ra)
        : Aya::DescribedNonCreatable<RunningAverageItemDouble, RunningAverageItem<double>, sRunningAverageItemDouble>(ra)
    {
    }
};

template<Aya::Time::SampleMethod sampleMethod = Aya::Time::Benchmark>
class RunningAverageTimeIntervalItem : public Aya::Stats::Item
{
    const Aya::RunningAverageTimeInterval<sampleMethod>& ra;

public:
    RunningAverageTimeIntervalItem(const Aya::RunningAverageTimeInterval<sampleMethod>* ra)
        : ra(*ra)
    {
    }

    virtual void update()
    {
        double v = ra.rate();
        if (val != v)
        {
            val = v;
            sValue = Aya::format("%g (%.2f%%CV)", v, 100.0 * ra.coefficient_of_variation());
        }
    }
};

extern const char* const sRunningAverageTimeInterval = "RunningAverageTimeIntervalItem";

class RunningAverageTimeIntervalItemTimeBenchmark
    : public Aya::DescribedNonCreatable<RunningAverageTimeIntervalItemTimeBenchmark, RunningAverageTimeIntervalItem<Aya::Time::Benchmark>,
          sRunningAverageTimeInterval>
{
public:
    RunningAverageTimeIntervalItemTimeBenchmark(const Aya::RunningAverageTimeInterval<Aya::Time::Benchmark>* ra)
        : Aya::DescribedNonCreatable<RunningAverageTimeIntervalItemTimeBenchmark, RunningAverageTimeIntervalItem<Aya::Time::Benchmark>,
              sRunningAverageTimeInterval>(ra)
    {
    }
};

extern const char* const sProfilingItem = "ProfilingItem";

class ProfilingItem : public Aya::DescribedNonCreatable<ProfilingItem, Aya::Stats::Item, sProfilingItem>
{
    const Aya::Profiling::Profiler& p;

public:
    ProfilingItem(const Aya::Profiling::Profiler* p)
        : p(*p)
    {
    }

    // Returns a Tuple of 4 numbers
    shared_ptr<const Aya::Reflection::Tuple> getTimes(double window)
    {
        Aya::Profiling::Bucket b = p.getWindow(window);
        shared_ptr<Aya::Reflection::Tuple> result(new Aya::Reflection::Tuple(3));
        result->values[0] = b.getWallTime();
        result->values[1] = b.getSampleTime();
        result->values[2] = b.frames;
        return result;
    }

    shared_ptr<const Aya::Reflection::Tuple> getTimesForFrames(int frames)
    {
        Aya::Profiling::Bucket b = p.getFrames(frames);
        shared_ptr<Aya::Reflection::Tuple> result(new Aya::Reflection::Tuple(3));
        result->values[0] = b.getWallTime();
        result->values[1] = b.getSampleTime();
        result->values[2] = b.frames;
        return result;
    }

    virtual void update()
    {
        const double window = Aya::Profiling::Profiler::profilingWindow;
        Aya::Profiling::Bucket b = p.getWindow(window);
        if (b.sampleTimeElapsed > 0)
        {
            val = b.getWallTime() / b.sampleTimeElapsed;
            char buffer[256];
            if (b.frames > 0)
            {
                std::string t = Aya::Log::formatTime(b.getNominalFramePeriod());
                sprintf(buffer, "%s %.3g/s nom%.3g/s %.3g%%", t.c_str(), b.getActualFPS(), b.getNominalFPS(), 100.0 * val);
            }
            else
            {
                sprintf(buffer, "%.3g%%", 100.0 * val);
            }
            sValue = buffer;
        }
        else
        {
            val = 0;
            sValue = "?";
        }
    }
};


static Aya::Reflection::BoundFuncDesc<ProfilingItem, shared_ptr<const Aya::Reflection::Tuple>(double)> func_GetTimes(
    &ProfilingItem::getTimes, "GetTimes", "window", 0.0, Aya::Security::Plugin);
static Aya::Reflection::BoundFuncDesc<ProfilingItem, shared_ptr<const Aya::Reflection::Tuple>(int)> func_GetTimesForFrames(
    &ProfilingItem::getTimesForFrames, "GetTimesForFrames", "frames", 1, Aya::Security::Plugin);
REFLECTION_END();


namespace Aya
{
namespace Stats
{
std::string browserTrackerId;
std::string appVersion;
std::string reporter;
std::string userId;
std::string location;

static void HttpHelper(std::string* response, std::exception* exception, const std::string& url)
{
    if (exception)
    {
#if !defined(AYA_PLATFORM_DURANGO) // remove this define in future?
        StandardOut::singleton()->printf(MESSAGE_ERROR, "%s: %s", url.c_str(), exception->what());
#endif
    }
}

static void httpPost(const std::string& url, std::string data, bool synchronous, std::string optionalContentType, bool externalRequest = false)
{
    if (synchronous)
    {
        try
        {
            std::string response;
            std::istringstream stream(data);
            Http http(url);
            http.recordStatistics = false;
            http.post(stream, optionalContentType.empty() ? Http::kContentTypeUrlEncoded : optionalContentType, data.size() > 256, response,
                externalRequest);
        }
        catch (std::exception& e)
        {
            StandardOut::singleton()->printf(MESSAGE_ERROR, "Stats http error: %s, %s", url.c_str(), e.what());
        }
    }
    else
    {
        Http http(url);
        http.recordStatistics = false;
        http.post(data, optionalContentType.empty() ? Http::kContentTypeUrlEncoded : optionalContentType, data.size() > 256,
            boost::bind(&HttpHelper, _1, _2, url), externalRequest);
    }
}

void setUserId(int id)
{
    userId = Aya::format("%d", id);
}

void setBrowserTrackerId(const std::string& trackerId)
{
    browserTrackerId = trackerId;
}
const char* const sStats = "Stats";


static Reflection::BoundFuncDesc<StatsService, void(std::string, shared_ptr<const Reflection::ValueTable>)> func_report(
    &StatsService::report, "Report", "category", "data", Aya::Security::RobloxScript);
static Reflection::BoundFuncDesc<StatsService, void(bool)> func_reportTaskScheduler(
    &StatsService::reportTaskScheduler, "ReportTaskScheduler", "includeJobs", false, Aya::Security::RobloxScript);
static Reflection::BoundFuncDesc<StatsService, void()> func_reportJobsStepWindow(
    &StatsService::reportJobsStepWindow, "ReportJobsStepWindow", Aya::Security::RobloxScript);
static Reflection::BoundFuncDesc<StatsService, void(std::string)> prop_reportUrl(
    &StatsService::setReportUrl, "SetReportUrl", "url", Security::RobloxScript);
static Reflection::BoundProp<std::string> prop_reporterType(
    "ReporterType", "Reporting", &StatsService::reporterType, Reflection::PropertyDescriptor::UI, Security::RobloxScript);
static Reflection::BoundProp<double> prop_minReportInterval(
    "MinReportInterval", "Reporting", &StatsService::minReportInterval, Reflection::PropertyDescriptor::UI, Security::RobloxScript);

const char* const sStatsItem = "StatsItem";

static Reflection::BoundFuncDesc<Item, std::string()> func_values(&Item::getStringValue2, "GetValueString", Aya::Security::Plugin);
static Reflection::BoundFuncDesc<Item, double()> func_value(&Item::getValue, "GetValue", Aya::Security::Plugin);
REFLECTION_END();

static void reportResult(std::string* message, std::exception* exception)
{
    if (exception)
        Aya::StandardOut::singleton()->printf(Aya::MESSAGE_WARNING, "StatsService Report failed: %s", exception->what());
    // if (message)
    //	Aya::StandardOut::singleton()->printf(Aya::MESSAGE_INFO, "StatsService Report result: %s", message->c_str());
}

void JsonWriter::writeTableEntries(const Reflection::ValueTable& data)
{
    std::for_each(data.begin(), data.end(), boost::bind(&JsonWriter::writeTableEntry, this, _1));
}

void JsonWriter::writeArrayEntries(const Reflection::ValueArray& data)
{
    std::for_each(data.begin(), data.end(), boost::bind(&JsonWriter::writeArrayEntry, this, _1));
}

void JsonWriter::writeTableEntry(const std::pair<std::string, Reflection::Variant>& value)
{
    if (needComma)
        s << ',';

    writeKeyValue(value);

    needComma = true;
}

void JsonWriter::writeKeyValue(const std::pair<std::string, Reflection::Variant>& value)
{
    std::string key = value.first;

    // Make the key legal for MongoDB
    while (key[0] == '$')
        key.erase(0, 1);

    while (true)
    {
        size_t p = key.find('.');
        if (p == std::string::npos)
            break;
        key[p] = '_';
    }

    // TODO: escape " and \ characters
    s << " \"" << key << "\":";

    writeValue(value.second);
}

void JsonWriter::writeArrayEntry(const Reflection::Variant& value)
{
    if (needComma)
        s << ',';
    else
        needComma = true;

    writeValue(value);

    needComma = true;
}

static std::string escape(const std::string& string)
{
    std::string result = "";

    for (std::string::size_type i = 0; i < string.length(); ++i)
    {
        char c = string.at(i);
        switch (c)
        {
        case '\0':
            result += "\\0";
            break;

        case '\r':
            result += "\\r";
            break;

        case '\n':
            result += "\\n";
            break;

        case '\t':
            result += "\\t";
            break;

        case '\\':
            result += "\\\\";
            break;

        case '\"':
            result += "\\\"";
            break;

        default:
            result += c;
        }
    }

    return result;
}

void JsonWriter::writeValue(const Reflection::Variant& value)
{
    if (value.isNumber())
    {
        s << value.get<std::string>();
    }
    else if (value.type() == Aya::Reflection::Type::singleton<bool>())
    {
        s << value.get<std::string>(); // v = "true" | "false"
    }
    else if (value.type() == Aya::Reflection::Type::singleton<shared_ptr<const Reflection::ValueTable>>())
    {
        s << "{ ";
        bool needComma = false;
        JsonWriter writer(s, needComma);
        writer.writeTableEntries(*value.get<shared_ptr<const Reflection::ValueTable>>());
        s << " }";
        hasNonJsonType = writer.seenNonJsonType();
    }
    else if (value.type() == Aya::Reflection::Type::singleton<shared_ptr<const Reflection::ValueArray>>())
    {
        s << "[ ";
        bool needComma = false;
        JsonWriter writer(s, needComma);
        writer.writeArrayEntries(*value.get<shared_ptr<const Reflection::ValueArray>>());
        s << " ]";
        hasNonJsonType = writer.seenNonJsonType();
    }
    else if (value.isVoid())
    {
        s << "null";
    }
    else
    {
        if (!value.isString())
            hasNonJsonType = true;

        std::string valueString = value.get<std::string>();
        std::string escaped = escape(valueString);
        s << "\"" << escaped << "\"";
    }
}

struct JobStepWindowWriter
{
    JobStepWindowWriter(std::stringstream* s)
        : stream(s)
        , firstTime(true) {};
    std::stringstream* stream;
    bool firstTime;
    void operator()(double dt)
    {
        if (firstTime)
            firstTime = false;
        else
            *stream << ", ";
        *stream << dt;
    }
};

bool StatsService::addHeader(shared_ptr<std::stringstream> stream)
{
    *stream << "{";
    int id = 0;
    bool needComma = false;

    if (Aya::DataModel* dataModel = DataModel::get(this))
    {
        id = dataModel->getPlaceIDOrZeroInStudio();
        if (id != 0)
        {
            *stream << " \"placeId\":" << id;
            needComma = true;
        }
    }

    if (reporterType.size() > 0)
    {
        if (needComma)
            *stream << ',';
        *stream << " \"reporter\":\"" << reporterType << "\"";
        needComma = true;
    }

    return needComma;
}

void StatsService::addCategoryAndTable(const std::string& category, const Reflection::ValueTable& data, shared_ptr<std::stringstream> stream)
{
    *stream << " \"category\":\"" << category << "\"";

    bool needsComma = true;
    JsonWriter writer(*stream, needsComma);
    writer.writeTableEntries(data);
}

static const bool jobsAsArray = true;

std::string StatsService::getDefaultReportUrl(const std::string& baseUrlInput, const std::string& shard)
{
    // copy base url, to allow modification
    std::string baseUrl = baseUrlInput;
    baseUrl = GetBaseURL();

    return baseUrl + "Gatherer/LogEntry?Shard=" + shard;
}

std::string StatsService::getReportUrl() const
{
    if (customReportUrl.size() > 0)
        return customReportUrl;

    if (ContentProvider* contentProvider = ServiceProvider::find<ContentProvider>(this))
    {
        return getDefaultReportUrl(contentProvider->getBaseUrl(), "Client");
    }

    throw std::runtime_error("No logging url provided");
}

void StatsService::setReportUrl(std::string url)
{
    customReportUrl = url;
}

void StatsService::postReportWithUrl(const std::string& url, shared_ptr<std::stringstream> stream)
{
    // DEPRECATED. TODO: remove


    // Http http(url);
    // http.post(stream, Http::kContentTypeApplicationJson, false, reportResult);
}

void StatsService::postReport(shared_ptr<std::stringstream> stream)
{
    postReportWithUrl(getReportUrl(), stream);
}

void StatsService::reportJob(boost::shared_ptr<const TaskScheduler::Job> job, shared_ptr<std::stringstream> stream, bool& isFirst)
{
    if (job->getArbiter().get() != DataModel::get(this))
        return;

    if (isFirst)
        isFirst = false;
    else
        *stream << ',';
    if (jobsAsArray)
        *stream << " { \"name\":\"" << job->getDebugName() << "\",";
    else
        *stream << " \"" << job->getDebugName() << "\": {";
    *stream << " \"dutyCycle\": " << job->averageDutyCycle();
    *stream << ", \"stepRate\": " << job->averageStepsPerSecond();
    *stream << ", \"stepTime\": " << job->averageStepTime();
    *stream << ", \"error\": " << job->averageError();
    //*stream << ", \"isRunning\": " << job->isRunning();
    *stream << " }";
}

void StatsService::reportJobsStepWindow()
{
    const char* category = "JobsStepWindow";

    if (!checkLastReport(category))
        return;

    shared_ptr<std::stringstream> stream(new std::stringstream());

    bool needComma = addHeader(stream);

    if (needComma)
        *stream << ',';

    *stream << " \"category\":\"" << category << "\"";

    *stream << ", \"ThreadPoolSize\": " << Aya::TaskScheduler::singleton().threadPoolSize();

    std::vector<boost::shared_ptr<const TaskScheduler::Job>> jobs;
    TaskScheduler::singleton().getJobsInfo(jobs);

    if (jobsAsArray)
        *stream << ", \"jobs\": [ ";
    else
        *stream << ", \"jobs\": { ";

    // jobs loop
    for (unsigned int i = 0; i < jobs.size(); i++)
    {
        if (jobsAsArray)
            *stream << " { \"name\":\"" << jobs[i]->getDebugName() << "\",";
        else
            *stream << " \"" << jobs[i]->getDebugName() << "\": {";

        *stream << "\"stepTimes\": ";

        // step times for each job
        *stream << "[ ";
        JobStepWindowWriter writer(stream.get());
        jobs[i]->getStepStats().stepTime().iter(writer);
        *stream << " ]";

        if (i + 1 == jobs.size())
            *stream << " } ";
        else
            *stream << " }, ";
    }

    if (jobsAsArray)
        *stream << " ]";
    else
        *stream << " }";

    *stream << " }";

    postReport(stream);
}

void StatsService::reportTaskScheduler(bool includeJobs)
{
    const char* category = "TaskScheduler";

    if (!checkLastReport(category))
        return;

    shared_ptr<std::stringstream> stream(new std::stringstream());

    bool needComma = addHeader(stream);

    if (needComma)
        *stream << ',';

    *stream << " \"category\":\"" << category << "\"";

    *stream << ", \"DataModelCount\": " << DataModel::count;
    *stream << ", \"ThreadAffinity\": " << Aya::TaskScheduler::singleton().threadAffinity();
    *stream << ", \"NumSleepingJobs\": " << Aya::TaskScheduler::singleton().numSleepingJobs();
    *stream << ", \"NumWaitingJobs\": " << Aya::TaskScheduler::singleton().numWaitingJobs();
    *stream << ", \"NumRunningJobs\": " << Aya::TaskScheduler::singleton().numRunningJobs();
    *stream << ", \"ThreadPoolSize\": " << Aya::TaskScheduler::singleton().threadPoolSize();
    *stream << ", \"SchedulerRate\": " << Aya::TaskScheduler::singleton().schedulerRate();
    *stream << ", \"SchedulerDutyCyclePerThread\": " << Aya::TaskScheduler::singleton().getSchedulerDutyCyclePerThread();
    *stream << ", \"PriorityMethod\": " << Aya::TaskScheduler::priorityMethod;

    if (includeJobs)
    {
        if (jobsAsArray)
            *stream << ", \"jobs\": [ ";
        else
            *stream << ", \"jobs\": { ";
        std::vector<boost::shared_ptr<const TaskScheduler::Job>> jobs;
        TaskScheduler::singleton().getJobsInfo(jobs);
        bool isFirst = true;
        std::for_each(jobs.begin(), jobs.end(), boost::bind(&StatsService::reportJob, this, _1, stream, boost::ref(isFirst)));
        if (jobsAsArray)
            *stream << " ]";
        else
            *stream << " }";
    }
    *stream << " }";

    postReport(stream);
}

void StatsService::report(std::string category, shared_ptr<const Reflection::ValueTable> data, int percentage)
{
    if (G3D::uniformRandomInt(0, 99) >= percentage)
        return;

    this->report(category, data);
}

// TODO: Make this an yielding function so you can get result?
void StatsService::report(std::string category, shared_ptr<const Reflection::ValueTable> data)
{
    if (!checkLastReport(category))
        return;

    shared_ptr<std::stringstream> stream(new std::stringstream());

    if (addHeader(stream))
        *stream << ',';

    addCategoryAndTable(category, *data, stream);

    *stream << " }";

    postReport(stream);
}

void StatsService::report_BypassThrottlingAndCustomUrl(std::string category, const Reflection::ValueTable& data, const char* shard)
{
    return;
}

bool StatsService::checkLastReport(const std::string& category)
{
    const Time now(Time::now<Time::Fast>());

    LastReportTimes::iterator iter = lastReportTimes.find(category);

    if (iter == lastReportTimes.end())
    {
        lastReportTimes[category] = now;
        return true; // A new value
    }

    if (iter->second + Time::Interval(minReportInterval) <= now)
    {
        iter->second = now;
        return true;
    }

    Aya::StandardOut::singleton()->printf(Aya::MESSAGE_WARNING, "The report '%s' was throttled. Adhere to MinReportInterval", category.c_str());
    return false;
}

static void runScript(shared_ptr<DataModel> d, std::string script)
{
    try
    {
        shared_ptr<ScriptContext> sc = shared_from(d->create<ScriptContext>());
        if (sc)
        {
            ProtectedString verifiedSource = ProtectedString::fromTrustedSource(script);
            ContentProvider::verifyRequestedScriptSignature(verifiedSource, "StatsScript", true);
            sc->executeInNewThread(Security::RobloxGameScript_, verifiedSource, "StatsReporting");
        }
    }
    catch (Aya::base_exception&)
    {
        // Swallow exception to make it harder to see (security)
    }
}

bool StatsService::tryToStartScript()
{
    // TODO: This is a hack until we have a common way of getting the correct url
    std::string baseUrl;
    if (ContentProvider* contentProvider = ServiceProvider::find<ContentProvider>(this))
    {
        baseUrl = contentProvider->getBaseUrl();
        if (baseUrl.empty())
            return false;
    }
    else
        return false;

    if (!Network::Players::serverIsPresent(this))
        return false;

    baseUrlConnection.disconnect();

    return true;
}

void StatsService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
    Instance::onServiceProvider(oldProvider, newProvider);

    if (newProvider)
    {
        if (!tryToStartScript())
            if (ContentProvider* contentProvider = ServiceProvider::create<ContentProvider>(this))
                baseUrlConnection = contentProvider->propertyChangedSignal.connect(boost::bind(&StatsService::tryToStartScript, this));
    }
}

void Item::update()
{
    Item* parent = Instance::fastDynamicCast<Item>(getParent());
    if (parent)
        parent->update();
}

void Item::formatMem(size_t bytes)
{
    setValue(bytes, Log::formatMem(bytes));
}
void Item::formatRate(const RunningAverageTimeInterval<Time::Benchmark>& interval)
{
    formatValue(interval.rate(), "%.1f/s (%.0f%%CV)", interval.rate(), 100.0 * interval.coefficient_of_variation());
}
void Item::formatValue(double val, const char* fmt, ...)
{
    va_list argList;
    va_start(argList, fmt);
    setValue(val, G3D::vformat(fmt, argList));
    va_end(argList);
}



Item* Item::createChildItem(const char* name)
{
    shared_ptr<Item> item = Creatable<Instance>::create<Item>();
    item->setName(name);
    item->setParent(this);
    return item.get();
}

template<>
void Item::formatValue(const double& value)
{
    formatValue(value, "%.3g", value);
}

template<>
void Item::formatValue(const float& value)
{
    formatValue(value, "%.3g", value);
}

template<>
void Item::formatValue(const int& value)
{
    formatValue(value, "%d", value);
}

template<>
void Item::formatValue(const long& value)
{
    formatValue(value, "%d", value);
}

#if !defined(__linux)
template<>
void Item::formatValue(const unsigned long& value)
{
    formatValue(value, "%d", value);
}
#endif

template<>
#ifdef _WIN32
void Item::formatValue(const unsigned __int64& value)
#else
#include <stdint.h>
void Item::formatValue(const uint64_t& value)
#endif
{
    formatValue(double(value), "%llu", value);
}

template<>
void Item::formatValue(const unsigned int& value)
{
    formatValue(value, "%d", value);
}

template<>
void Item::formatValue(const bool& value)
{
    this->sValue = value ? "true" : "false";
    this->val = value ? 1 : 0;
}

template<>
Item* Item::createBoundChildItem(const char* name, const TotalCountTimeInterval<>& ra)
{
    shared_ptr<Item> item = Creatable<Instance>::create<TotalCountTimeIntervalItem>(&ra);
    item->setName(name);
    item->setParent(this);
    return item.get();
}

template<>
Item* Item::createBoundChildItem(const char* name, const RunningAverage<int, double>& ra)
{
    shared_ptr<Item> item = Creatable<Instance>::create<RunningAverageItemInt>(&ra);
    item->setName(name);
    item->setParent(this);
    return item.get();
}

template<>
Item* Item::createBoundChildItem(const char* name, const RunningAverage<double, double>& ra)
{
    shared_ptr<Item> item = Creatable<Instance>::create<RunningAverageItemDouble>(&ra);
    item->setName(name);
    item->setParent(this);
    return item.get();
}

template<>
Item* Item::createBoundChildItem(const char* name, const RunningAverageTimeInterval<Aya::Time::Benchmark>& ra)
{
    shared_ptr<Item> item = Creatable<Instance>::create<RunningAverageTimeIntervalItemTimeBenchmark>(&ra);
    item->setName(name);
    item->setParent(this);
    return item.get();
}

Item* Item::createBoundChildItem(const Profiling::Profiler& p)
{
    const Profiling::Profiler* ptr = &p;
    shared_ptr<Item> item = Creatable<Instance>::create<ProfilingItem>(ptr);
    item->setName(p.name.c_str());
    item->setParent(this);
    return item.get();
}

Item* Item::createBoundMemChildItem(const char* name, const size_t& v)
{
    shared_ptr<Item> item = Creatable<Instance>::create<TypedMemItem>(&v);
    item->setName(name);
    item->setParent(this);
    return item.get();
}

Item* Item::createBoundPercentChildItem(const char* name, const float& v)
{
    shared_ptr<Item> item = Creatable<Instance>::create<TypedPercentItem>(&v);
    item->setName(name);
    item->setParent(this);
    return item.get();
}

} // namespace Stats
} // namespace Aya


AYA_REGISTER_CLASS(Aya::Stats::Item);
AYA_REGISTER_CLASS(RunningAverageItemInt);
AYA_REGISTER_CLASS(RunningAverageItemDouble);
AYA_REGISTER_CLASS(ProfilingItem);
AYA_REGISTER_CLASS(Aya::Stats::StatsService);
AYA_REGISTER_CLASS(TotalCountTimeIntervalItem);
AYA_REGISTER_CLASS(RunningAverageTimeIntervalItemTimeBenchmark);

void Aya::registerStatsClasses()
{
    // Prevent optimizer from stripping code
    Aya::Stats::StatsService::classDescriptor();
}
