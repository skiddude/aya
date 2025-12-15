

#pragma once

#include <string>
#include <istream>
#include "boost.hpp"
#include "time.hpp"
#include "Tree/Service.hpp"
#include "Utility/AsyncHttpCache.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "Utility/HeartbeatInstance.hpp"

namespace Aya
{
class Instance;

extern const char* const sScriptInformationProvider;
class ScriptInformationProvider
    : public DescribedNonCreatable<ScriptInformationProvider, Instance, sScriptInformationProvider>
    , public Service
{
private:
    typedef DescribedNonCreatable<ScriptInformationProvider, Instance, sScriptInformationProvider> Super;

    std::string assetUrl;
    std::string access;

public:
    ScriptInformationProvider();

    void setAssetUrl(std::string url)
    {
        assetUrl = url;
    }
    void setAccessKey(std::string access)
    {
        this->access = access;
    }
};
} // namespace Aya