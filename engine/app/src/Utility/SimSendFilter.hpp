

#pragma once

#include "Utility/Region2.hpp"
#include "Utility/SystemAddress.hpp"
#include <string>

namespace Aya
{

class SimSendFilter
{
public:
    typedef enum
    {
        EditVisit,
        Client,
        Server,
        dPhysClient,
        dPhysServer
    } Mode;

    Mode mode;
    Aya::SystemAddress networkAddress;
    Region2 region;

    SimSendFilter()
        : mode(Client)
    {
    }
};

} // namespace Aya
