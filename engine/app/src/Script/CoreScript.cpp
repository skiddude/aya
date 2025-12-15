

#include "Script/CoreScript.hpp"
#include "Script/ScriptContext.hpp"

#include "DataModel/DataModel.hpp"
#include "DataModel/ContentProvider.hpp"
#include "Lua/lua.hpp"
#include "Utility/FileSystem.hpp"
#include "Log.hpp"

#include "StringConv.hpp"

namespace Aya
{
const char* const sCoreScript = "CoreScript";

CoreScript::CoreScript()
    : Super()
{
    setName(sCoreScript);
    setRobloxLocked(true);
}

// used to make sure corescripts not in the workspace still get removed properly
void CoreScript::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
    if (oldProvider && !workspace)
    {
        Aya::ScriptContext* sc = ServiceProvider::find<ScriptContext>(oldProvider);
        AYAASSERT(sc);

        AYAASSERT(sc->hasScript(this));
        sc->removeScript(weak_from(this));
    }
    Super::onServiceProvider(oldProvider, newProvider);
}

boost::optional<ProtectedString> CoreScript::fetchSource(const std::string& name)
{
    if (LuaVM::canCompileScripts())
    {
        std::string path = BaseScript::hasCoreScriptReplacements() ? BaseScript::adminScriptsPath : ContentProvider::assetFolder() + "scripts";
        std::string file = path + "/" + name + ".lua";

        std::ifstream in(utf8_decode(file).c_str(), std::ios::in | std::ios::binary);

        if (!in)
            return boost::optional<ProtectedString>();

        std::stringstream ss;
        ss << in.rdbuf();

        return ProtectedString::fromTrustedSource(ss.str());
    }
    else
    {
        std::string bytecode = LuaVM::getBytecodeCore(name);

        if (bytecode.empty())
            return boost::optional<ProtectedString>();
        else
            return ProtectedString::fromBytecode(bytecode);
    }
}

BaseScript::Code CoreScript::requestCode(ScriptInformationProvider* scriptInfoProvider)
{
    if (boost::optional<ProtectedString> source = fetchSource(getName()))
    {
        return Code(boost::flyweight<ProtectedString>(*source));
    }
    else
    {
        throw Aya::runtime_error("Error loading core script %s", getName().c_str());
    }
}

// log the error in a special file if it's from a CoreScript
void CoreScript::extraErrorReporting(lua_State* thread)
{
    std::stringstream out;

    // format and collect error information
    const char* error = Aya::Lua::safe_lua_tostring(thread, -1);
    if (error && strlen(error) > 0)
        out << error;
    else
        out << "Error occurred, no output from Lua.";

    int line;
    shared_ptr<BaseScript> source;
    out << "\n\nCallStack:\n" << ScriptContext::extractCallStack(thread, source, line);
    AYAASSERT(source.get() == this);

    if (DataModel* dataModel = DataModel::get(this))
        out << "\nPlaceID: " << dataModel->getPlaceIDOrZeroInStudio();
    else
        out << "\nError finding PlaceID!";
    std::string errorReport = out.str();

    std::stringstream fileOut;
    fileOut << Aya::FileSystem::getUserDirectory(true, Aya::DirAppData, "logs") << this->getName() << "_ln" << line << "_" << ".cse";
    std::string filename = fileOut.str();

    std::ofstream errorLog;
    errorLog.open(filename.c_str());
    if (errorLog.is_open())
    {
        Aya::Log::timeStamp(errorLog, true);
        errorLog << errorReport;
    }
    errorLog.close();
}
} // namespace Aya
