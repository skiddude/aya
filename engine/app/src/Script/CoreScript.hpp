#pragma once

#include "Script/script.hpp"

#include <boost/optional.hpp>

namespace Aya
{
extern const char* const sCoreScript;
class CoreScript : public DescribedNonCreatable<CoreScript, BaseScript, sCoreScript, Aya::Reflection::ClassDescriptor::INTERNAL_LOCAL>
{
private:
    typedef DescribedNonCreatable<CoreScript, BaseScript, sCoreScript, Aya::Reflection::ClassDescriptor::INTERNAL_LOCAL> Super;
    Code code;

public:
    CoreScript();

    static boost::optional<ProtectedString> fetchSource(const std::string& name);

    virtual Code requestCode(ScriptInformationProvider* scriptInfoProvider = NULL);

    virtual void extraErrorReporting(lua_State* thread);

protected:
    // Instance
    /*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
};
} // namespace Aya
