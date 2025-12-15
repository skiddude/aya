

#include "DataModel/ServerScriptService.hpp"
#include "DataModel/Folder.hpp"
#include "Script/script.hpp"
#include "Script/CoreScript.hpp"
#include "Script/ModuleScript.hpp"
#include "Players.hpp"

using namespace Aya;

const char* const Aya::sServerScriptService = "ServerScriptService";


Reflection::PropDescriptor<ServerScriptService, bool> ServerScriptService::desc_loadStringEnabled("LoadStringEnabled", category_Behavior,
    &ServerScriptService::getLoadStringEnabled, &ServerScriptService::setLoadStringEnabled, Reflection::PropertyDescriptor::PUBLIC_SERIALIZED);
REFLECTION_END();

ServerScriptService::ServerScriptService(void)
    : loadStringEnabled(false)
{
    setName(sServerScriptService);
}

void ServerScriptService::setLoadStringEnabled(bool value)
{
    bool changed = value != loadStringEnabled;
    loadStringEnabled = value;
    if (changed)
    {
        raisePropertyChanged(desc_loadStringEnabled);
    }
}

bool ServerScriptService::askAddChild(const Instance* instance) const
{
    if (Instance::fastDynamicCast<Folder>(instance) != NULL)
        return true;

    return ((Instance::fastDynamicCast<Script>(instance) != NULL) && (Instance::fastDynamicCast<LocalScript>(instance) == NULL)) ||
           (Instance::fastDynamicCast<ModuleScript>(instance) != NULL);
}

bool ServerScriptService::scriptShouldRun(BaseScript* script)
{
    bool isAncestor = isAncestorOf(script);
    AYAASSERT(isAncestor);

    if (!isAncestor)
        return false;

    if (!Aya::Network::Players::backendProcessing(script))
        return false;

    if (script->fastDynamicCast<Script>() && !script->fastDynamicCast<LocalScript>())
        return true;

    return false;
}