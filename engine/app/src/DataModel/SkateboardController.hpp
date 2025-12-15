

#pragma once

#include "Utility/SteppedInstance.hpp"
#include "DataModel/UserController.hpp"


namespace Aya
{

class UserInputBase;
class SkateboardPlatform;


extern const char* const sSkateboardController;
class SkateboardController : public DescribedCreatable<SkateboardController, Controller, sSkateboardController>
{
private:
    // IStepped
    /*override*/ void onStepped(const Stepped& event);

    float throttle;
    float steer;

    void setThrottle(float value);
    void setSteer(float value);

    void onSteppedTouchInput();
    void onSteppedKeyboardInput();

    weak_ptr<SkateboardPlatform> skateboardPlatform;

public:
    SkateboardController();

    void setSkateboardPlatform(SkateboardPlatform* value);

    // lua api:
    float getThrottle() const
    {
        return throttle;
    }
    float getSteer() const
    {
        return steer;
    }

    Aya::signal<void(std::string)> axisChangedSignal;
};


} // namespace Aya