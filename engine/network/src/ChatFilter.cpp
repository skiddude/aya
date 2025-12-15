#include "ChatFilter.hpp"

#include "Utility/Http.hpp"


#include <boost/algorithm/string/predicate.hpp>
#include <boost/shared_ptr.hpp>

#include <string>

DYNAMIC_LOGGROUP(WebChatFiltering)

using namespace Aya;
using namespace Aya::Network;

const char* const Aya::Network::sChatFilter = "ChatFilter";

bool ChatFilter::filterMessageBase(shared_ptr<Player> sourcePlayer, shared_ptr<Instance> receiver, const std::string& message, const ChatFilter::FilteredChatMessageCallback callback)
{
    // no filter
    Result result;
    result.whitelistFilteredMessage = message;
    result.blacklistFilteredMessage = message;
    boost::thread t(boost::bind(callback, result));
    return true;
}
