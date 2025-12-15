

#include "Script/ScriptEvent.hpp"
#include "Script/ScriptContext.hpp"
#include "Lua/lua.hpp"
#include "Utility/StandardOut.hpp"
#include "Script/LuaInstanceBridge.hpp"
#include "time.hpp"
#include "Script/LuaSettings.hpp"
#include "FastLog.hpp"

DYNAMIC_FASTFLAGVARIABLE(FixYieldThrottling, false)

using namespace Aya;
using namespace Aya::Lua;

YieldingThreads::YieldingThreads(ScriptContext* context)
    : context(context)
{
}


void YieldingThreads::queueWaiter(lua_State* L)
{
    queueWaiter(L, 0.0);
}


void YieldingThreads::queueWaiter(lua_State* L, LUA_NUMBER delay)
{
    AYAASSERT(!RobloxExtraSpace::get(L)->yieldCaptured);
    RobloxExtraSpace::get(L)->yieldCaptured = true;

    waitingThreads.push(WaitingThread(L, Aya::Time::Interval(delay)));
}

std::size_t YieldingThreads::waiterCount() const
{
    return waitingThreads.size();
}


void YieldingThreads::resume(double wallTime, Time expirationTime, bool& throttling)
{
    FASTLOG(FLog::ScriptContext, "Resuming waiting threads");

    int count = waitingThreads.size();

    while ((!DFFlag::FixYieldThrottling || count-- > 0) && !waitingThreads.empty())
    {
        Aya::Time now = Aya::Time::now<Aya::Time::Precise>();

        const WaitingThread& w = waitingThreads.top();

        if (now < w.resumeTime)
            break; // we're done

        Aya::Time::Interval elapsedTime = now - w.waitTime;
        ThreadRef thread = w.thread->lock();

        waitingThreads.pop();

        if (thread)
        {
            lua_pushnumber(thread, elapsedTime.seconds());
            lua_pushnumber(thread, wallTime);

            // resume the waiting thread
            if (context->resume(thread, 2) != ScriptContext::Yield)
            {
                // success or error means thread is finished, clear the stack
                lua_resetstack(thread, 0);
            }

            context->scriptResumedFromEvent();
        }

        // We always do at least one thread, so as to make forward progress
        if (now > expirationTime)
        {
            throttling = true;
            break;
        }
    }
}

void YieldingThreads::clearAllSinks()
{
    while (!waitingThreads.empty())
        waitingThreads.pop();
}

namespace Aya
{
namespace Lua
{
// The default implementation for on_tostring is available in LuaBridge.cpp. This is a specialization.
template<>
int Bridge<Aya::signals::connection>::on_tostring(const Aya::signals::connection& object, lua_State* L)
{
    lua_pushliteral(L, "Connection");
    return 1;
}

// The default implementation for on_tostring is available in LuaBridge.cpp. This is a specialization.
template<>
int Bridge<boost::intrusive_ptr<class WeakThreadRef::Node>>::on_tostring(const boost::intrusive_ptr<class WeakThreadRef::Node>& object, lua_State* L)
{
    lua_pushliteral(L, "WeakThreadRef");
    return 1;
}

// The default implementation for on_tostring is available in LuaBridge.cpp. This is a specialization.
template<>
int Bridge<shared_ptr<GenericFunction>>::on_tostring(const shared_ptr<GenericFunction>& object, lua_State* L)
{
    lua_pushliteral(L, "GenericFunction");
    return 1;
}

// The default implementation for on_tostring is available in LuaBridge.cpp. This is a specialization.
template<>
int Bridge<shared_ptr<GenericAsyncFunction>>::on_tostring(const shared_ptr<GenericAsyncFunction>& object, lua_State* L)
{
    lua_pushliteral(L, "GenericAsyncFunction");
    return 1;
}
} // namespace Lua
} // namespace Aya
