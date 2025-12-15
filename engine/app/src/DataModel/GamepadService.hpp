//
//  GamepadService.h
//  App
//
//  Created by Ben Tkacheff on 1/21/15.
//
//

#pragma once

#include "Tree/Instance.hpp"
#include "Tree/Service.hpp"
#include "DataModel/InputObject.hpp"
#include "Utility/UserInputBase.hpp"
#define AYA_MAX_GAMEPADS 8
#include "DataModel/DataModel.hpp"
#include "DataModel/GuiObject.hpp"

namespace Aya
{

class BasePlayerGui;

extern const char* const sGamepadService;

typedef boost::unordered_map<Aya::KeyCode, shared_ptr<Aya::InputObject>> Gamepad;
typedef boost::unordered_map<int, Gamepad> Gamepads;

class GamepadService
    : public DescribedNonCreatable<GamepadService, Instance, sGamepadService>
    , public Service
{
private:
    typedef DescribedNonCreatable<GamepadService, Instance, sGamepadService> Super;

    Gamepads gamepads;

    boost::unordered_map<InputObject::UserInputType, bool> gamepadNavigationEnabledMap;

    Aya::Timer<Aya::Time::Fast> repeatGuiSelectionTimer;
    Aya::Timer<Aya::Time::Fast> fastRepeatGuiSelectionTimer;

    bool autoGuiSelectionAllowed; // whether this game allows for automatic gui selection with gamepad keys

    Vector2 lastGuiSelectionDirection; // helper for determining how to use thumbstick for gui selection

    Aya::signals::scoped_connection updateInputConnection;
    Aya::signals::scoped_connection inputEndedConnection;
    Aya::signals::scoped_connection inputChangedConnection;
    Aya::signals::scoped_connection cameraCframeUpdateConnection;

    shared_ptr<Aya::InputObject> createInputObjectForGamepadKeyCode(Aya::KeyCode keyCode, Aya::InputObject::UserInputType gamepadType);
    void createControllerKeyMapForController(int controllerIndex);

    Vector2 getGuiSelectionDirection(const shared_ptr<InputObject>& event);

    bool isVectorInDeadzone(const Vector2& correctGuiDirection) const;

    GuiResponse autoSelectGui();
    GuiObject* getRandomShownGuiObject(Instance* object);

    void currentCameraChanged(shared_ptr<Camera> newCurrentCamera);
    void cameraCframeChanged(CoordinateFrame cframe);

    void updateOnInputStep();
    void onInputChanged(const shared_ptr<Instance>& event);
    void onInputEnded(const shared_ptr<Instance>& event);

    GuiResponse process(const shared_ptr<InputObject>& event, BasePlayerGui* guiToProcess);

    // Instance
    /*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

public:
    GamepadService();

    static InputObject::UserInputType getGamepadEnumForInt(int controllerIndex);
    static int getGamepadIntForEnum(InputObject::UserInputType gamepadEnum);

    Gamepad getGamepadState(int controllerIndex);

    bool setAutoGuiSelectionAllowed(bool value);
    bool getAutoGuiSelectionAllowed() const
    {
        return autoGuiSelectionAllowed;
    }

    bool isNavigationGamepad(InputObject::UserInputType gamepadType);
    void setNavigationGamepad(InputObject::UserInputType gamepadType, bool enabled);

    boost::unordered_map<InputObject::UserInputType, bool> getNavigationGamepadMap()
    {
        return gamepadNavigationEnabledMap;
    }

    GuiResponse processDev(const shared_ptr<InputObject>& event);
    GuiResponse processCore(const shared_ptr<InputObject>& event);

    GuiResponse trySelectGuiObject(const Vector2& inputVector, const shared_ptr<InputObject>& event, BasePlayerGui* guiToProcess);
    GuiResponse trySelectGuiObject(const Vector2& inputVector);
};
} // namespace Aya