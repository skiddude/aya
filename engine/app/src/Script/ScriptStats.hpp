#pragma once
#include "RunningAverage.hpp"
#include "boost/weak_ptr.hpp"
#include "Utility/Utilities.hpp"
#include "DataModel/Stats.hpp"
#include "Script/ScriptContext.hpp"
#include <stack>
#include <map>

namespace Aya
{

class ScriptStats
{
public:
    struct StatCollection
    {
        boost::shared_ptr<ActivityMeter<2>> activity;
        boost::shared_ptr<InvocationMeter<2>> invocations;
    };
    typedef std::map<std::string, StatCollection> ScriptActivityMeterMap;

protected:
    ScriptActivityMeterMap scriptActivityMap;

    std::stack<std::string> scriptStack;

    void stopCollection(const std::string& scriptHash);
    void startCollection(const std::string& scriptHash, bool firstTime);

public:
    ScriptStats();

    void scriptResumeStarted(const std::string& scriptHash);
    void scriptResumeStopped(const std::string& scriptHash);

    const ScriptActivityMeterMap& getScriptActivityMap() const
    {
        return scriptActivityMap;
    }
};

class LuaStatsItem : public Stats::Item
{
    ScriptContext* scriptContext;
    Stats::Item* averageGcInterval;
    Stats::Item* averageGcTime;
    Stats::Item* resumedThreads;
    Stats::Item* deferredThreads;

public:
    LuaStatsItem(ScriptContext* context)
        : scriptContext(context)
    {
        setName("Lua");
    }

    static shared_ptr<LuaStatsItem> create(ScriptContext* context)
    {
        shared_ptr<LuaStatsItem> result = Creatable<Instance>::create<LuaStatsItem>(context);
        result->init();
        return result;
    }

    void init();

    virtual void update();
};

} // namespace Aya