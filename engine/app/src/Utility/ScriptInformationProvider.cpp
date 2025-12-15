

#include "Utility/ScriptInformationProvider.hpp"
#include "Utility/Http.hpp"
#include <sstream>

namespace Aya
{
const char* const sScriptInformationProvider = "ScriptInformationProvider";


static Reflection::BoundFuncDesc<ScriptInformationProvider, void(std::string)> func_setAssetUrl(
    &ScriptInformationProvider::setAssetUrl, "SetAssetUrl", "url", Security::LocalUser);
static Reflection::BoundFuncDesc<ScriptInformationProvider, void(std::string)> func_setAccessKey(
    &ScriptInformationProvider::setAccessKey, "SetAccessKey", "access", Security::Roblox);
REFLECTION_END();

ScriptInformationProvider::ScriptInformationProvider()
    : DescribedNonCreatable<ScriptInformationProvider, Instance, sScriptInformationProvider>()
{
    this->setName("ScriptInformationProvider");
}

} // namespace Aya
