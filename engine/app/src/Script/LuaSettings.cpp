

#include "Script/LuaSettings.hpp"
#include "Lua/luaconf.h"

const char* const Aya::sLuaSettings = "LuaSettings";


static Aya::Reflection::BoundProp<int> prop_1("GcPause", "Garbage Collection", &Aya::LuaSettings::gcPause);
static Aya::Reflection::BoundProp<int> prop_2("GcStepMul", "Garbage Collection", &Aya::LuaSettings::gcStepMul);
static Aya::Reflection::BoundProp<double> prop_3("DefaultWaitTime", "Settings", &Aya::LuaSettings::defaultWaitTime);
static Aya::Reflection::BoundProp<int> prop_4("GcLimit", "Garbage Collection", &Aya::LuaSettings::gcLimit);
static Aya::Reflection::BoundProp<int> prop_5("GcFrequency", "Garbage Collection", &Aya::LuaSettings::gcFrequency);
static Aya::Reflection::BoundProp<bool> prop_areScriptStartsReported(
    "AreScriptStartsReported", "Diagnostics", &Aya::LuaSettings::areScriptStartsReported);
static Aya::Reflection::BoundProp<float> prop_AreWaitingThreadsBudgeted("WaitingThreadsBudget", "Settings", &Aya::LuaSettings::waitingThreadsBudget);
REFLECTION_END();

Aya::LuaSettings::LuaSettings(void)
    : gcPause(LUAI_GCPAUSE)
    , gcStepMul(LUAI_GCMUL)
    , defaultWaitTime(0.03)
    , smallestWaitTime(0.016667)
    , gcLimit(2)
    , gcFrequency(0)
    , areScriptStartsReported(false)
    , waitingThreadsBudget(0.1)
{
    setName("Lua");
}
