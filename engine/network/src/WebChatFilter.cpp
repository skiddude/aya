#include "WebChatFilter.hpp"


#include <algorithm>

#include "Utility/RobloxServicesTools.hpp"

#include "time.hpp"

#include "DataModel/Stats.hpp"

#include "DataModel/DataModel.hpp"

#include "Utility/Guid.hpp"

#include "Utility/Http.hpp"

#include "Utility/HttpAsync.hpp"

#include "Utility/StandardOut.hpp"

#include "Reflection/Type.hpp"
#include "Utility/Statistics.hpp"


#include <boost/lexical_cast.hpp>
#include <boost/thread/once.hpp>
#include <boost/shared_ptr.hpp>

#include <rapidjson/document.h>
#include <string>

DYNAMIC_LOGVARIABLE(WebChatFiltering, 0)

DYNAMIC_FASTINTVARIABLE(WebChatFilterHttpTimeoutSeconds, 60)
DYNAMIC_FASTFLAGVARIABLE(ConstructModerationFilterTextParamsAndHeadersUseLegacyFilterParams, true)

using namespace Aya;
using namespace Aya::Network;

namespace
{
static Timer<Aya::Time::Precise> lastFailureTime;

void initFailureTimer()
{
    lastFailureTime.reset();
}

inline void logContent(const std::string& varName, const std::string& msg)
{
    if (DFLog::WebChatFiltering)
    {
        std::stringstream ss;
        ss << varName << " (" << msg.length() << " bytes) " << msg;
        FASTLOGS(DFLog::WebChatFiltering, "%s", ss.str().c_str());
    }
}

inline void logResponseData(const std::string& unfiltered, const ChatFilter::Result& result, const Aya::Time::Interval& webChatFilterResponseDelta)
{
    FASTLOG1F(DFLog::WebChatFiltering, "ResponseLatencyMillis: %f", webChatFilterResponseDelta.msec());
    logContent("FilteredContent (blacklist)", result.blacklistFilteredMessage);
    logContent("FilteredContent (whitelist)", result.whitelistFilteredMessage);
}
} // namespace

void filterMessageHelper(const std::string message, const Aya::Network::ChatFilter::FilteredChatMessageCallback filteredCallback,
    shared_ptr<Aya::Network::Player> playerFilter)
{
    // gross
}

void WebChatFilter::filterMessage(shared_ptr<Aya::Network::Player> sender, shared_ptr<Aya::Instance> receiver, const std::string& message,
    const Aya::Network::ChatFilter::FilteredChatMessageCallback filteredCallback)
{
    Result result;
    result.whitelistFilteredMessage = message;
    result.blacklistFilteredMessage = message;
    boost::thread t(boost::bind(filteredCallback, result));

    return;
}