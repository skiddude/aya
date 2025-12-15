#pragma once

#include "ChatFilter.hpp"

#include "Player.hpp"

#include "Utility/Http.hpp"

#include "Utility/HttpAux.hpp"


#include <boost/shared_ptr.hpp>

static const std::string kWebChatWhiteListPolicy = "white";
static const std::string kWebChatBlackListPolicy = "black";

namespace Aya
{
namespace Network
{

class WebChatFilter : public Aya::Network::ChatFilter
{
public:
    /*override*/
    virtual void filterMessage(shared_ptr<Aya::Network::Player> sourcePlayer, shared_ptr<Aya::Instance> receiver, const std::string& message,
        const Aya::Network::ChatFilter::FilteredChatMessageCallback callback);
}; // class WebChatFilter

} // namespace Network
} // namespace Aya
