

#pragma once

// #include "Utility/SoundWorld.hpp"
#include <string>


namespace Aya
{

class Action
{
public:
    enum ActionType
    {
        NO_ACTION = 0,
        PAUSE_ACTION,
        LOSE_ACTION,
        DRAW_ACTION,
        WIN_ACTION,
        NUM_ACTION_TYPES
    };

private:
    Action();
};

} // namespace Aya