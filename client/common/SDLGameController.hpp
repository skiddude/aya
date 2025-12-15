#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/weak_ptr.hpp>

#include "SDL3/SDL.h"
#include "SDL3/SDL_gamepad.h"

#include "Utility/KeyCode.hpp"
#include "DataModel/InputObject.hpp"
#include "DataModel/HapticService.hpp"

namespace Aya
{
class DataModel;
class UserInputService;

class GamepadService;

typedef boost::unordered_map<Aya::KeyCode, boost::shared_ptr<Aya::InputObject>> Gamepad;
} // namespace Aya

struct HapticData
{
    int hapticEffectId;
    float currentLeftMotorValue;
    float currentRightMotorValue;
    SDL_Haptic* hapticDevice;
};

class SDLGameController
{
private:
    boost::weak_ptr<Aya::DataModel> dataModel;
    boost::unordered_map<int, std::pair<int, SDL_Gamepad*>> gamepadIdToGameController;
    boost::unordered_map<int, HapticData> hapticsFromGamepadId;
    boost::unordered_map<int, int> joystickIdToGamepadId;

    Aya::signals::scoped_connection renderSteppedConnection;
    Aya::signals::scoped_connection getSupportedGamepadKeyCodesConnection;

    Aya::signals::scoped_connection setEnabledVibrationMotorsConnection;
    Aya::signals::scoped_connection setVibrationMotorConnection;

    void initSDL();

    Aya::UserInputService* getUserInputService();
    Aya::HapticService* getHapticService();

    Aya::GamepadService* getGamepadService();
    Aya::Gamepad getRbxGamepadFromJoystickId(int joystickId);

    void setupControllerId(int joystickId, int gamepadId, SDL_Gamepad* pad);
    SDL_Gamepad* removeControllerMapping(int joystickId);

    int getGamepadIntForEnum(Aya::InputObject::UserInputType gamepadType);

    void findAvailableGamepadKeyCodesAndSet(Aya::InputObject::UserInputType gamepadType);
    boost::shared_ptr<const Aya::Reflection::ValueArray> getAvailableGamepadKeyCodes(Aya::InputObject::UserInputType gamepadType);

    void bindToDataModel();

    // Haptic Functions
    void refreshHapticEffects();
    bool setupHapticsForDevice(int id);

    void setVibrationMotorsEnabled(Aya::InputObject::UserInputType gamepadType);
    void setVibrationMotor(Aya::InputObject::UserInputType gamepadType, Aya::HapticService::VibrationMotor vibrationMotor,
        shared_ptr<const Aya::Reflection::Tuple> args);

public:
    SDLGameController(boost::shared_ptr<Aya::DataModel> newDM);
    ~SDLGameController();

    void updateControllers();

    void onControllerAxis(const SDL_GamepadAxisEvent sdlEvent);
    void onControllerButton(const SDL_GamepadButtonEvent sdlEvent);
    void removeController(int joystickId);
    void addController(int gamepadId);
};
