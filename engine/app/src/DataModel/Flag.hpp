
#pragma once

#include "Tree/Instance.hpp"
#include "DataModel/Tool.hpp"
#include "Utility/BrickColor.hpp"
#include "DataModel/PartInstance.hpp"

namespace Aya
{


extern const char* const sFlag;

class FlagStand;
class TimerService;
namespace Network
{
class Player;
}

class Flag : public DescribedCreatable<Flag, Tool, sFlag>
{
private:
    typedef DescribedCreatable<Flag, Tool, sFlag> Super;
    Aya::signals::scoped_connection_logged flagTouched;
    void onEvent_flagTouched(shared_ptr<Instance> other);

protected:
    /*override*/
    virtual bool canUnequip()
    {
        return false;
    }                                                       // The flag cannot be unequipped
    virtual bool canBePickedUpByPlayer(Network::Player* p); // The flag cannot be picked up by a member of the same team as the flag.

public:
    BrickColor teamColor;

    BrickColor getTeamColor() const;
    void setTeamColor(BrickColor color);

    void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

    FlagStand* getJoinedStand();

    Flag();
    ~Flag();
};

} // namespace Aya
