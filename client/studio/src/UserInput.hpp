

#pragma once

// Roblox Headers
#include "Utility/UserInputBase.hpp"

#include "Utility/RunStateOwner.hpp"

#include "Utility/Rect.hpp"

#include "DataModel/InputObject.hpp"

#include "SDLGameController.hpp"

class RobloxView;

namespace Aya
{
class DataModel;
}

class UserInput : public Aya::UserInputBase
{
private:
    int debugI;

    Aya::signals::scoped_connection processedMouseConnection;

    size_t tableSize;

    // Mouse Stuff
    bool isMouseCaptured; // poor man's tracker of button state
    bool leftMouseButtonDown;
    bool middleMouseButtonDown;
    bool rightMouseButtonDown;
    G3D::Vector2 wrapMousePosition; // in normalized coordinates (center is 0,0. radius is getWrapRadius)
    bool wrapping;
    bool leftMouseDown;
    bool rightMouseDown;
    bool autoMouseMove;

    // Keyboard Stuff
    int externallyForcedKeyDown; // + this from exteranal source (like a gui button)

    class RobloxView* wnd;
    shared_ptr<Aya::RunService> runService;

    // InputObject stuff
    boost::unordered_map<Aya::InputObject::UserInputType, shared_ptr<Aya::InputObject>> inputObjectMap;

    shared_ptr<SDLGameController> sdlGameController;


    ////////////////////////////////
    //
    // Events


    void sendEvent(shared_ptr<Aya::InputObject> event, bool processed = false);
    void sendMouseEvent(Aya::InputObject::UserInputType mouseEventType, Aya::InputObject::UserInputState mouseEventState, Aya::Vector3& position,
        Aya::Vector3& delta);

    ////////////////////////////////////
    //
    // Keyboard Mouse
    bool isMouseInside;

    G3D::Vector2 posToWrapTo;

    // Studio mobile emulation
    shared_ptr<Aya::InputObject> touchInput;

    // window stuff
    Aya::Vector2int16 getWindowSize() const;
    G3D::Rect2D getWindowRect() const; // { return windowRect; }
    bool isFullScreenMode() const;
    bool movementKeysDown();
    bool keyDownInternal(Aya::KeyCode code) const;

    void doWrapMouse(const G3D::Vector2& delta, G3D::Vector2& wrapMouseDelta);

    G3D::Vector2 getGameCursorPositionInternal();
    G3D::Vector2 getGameCursorPositionExpandedInternal(); // prevent hysteresis
    G3D::Vector2 getWindowsCursorPositionInternal();
    Aya::Vector2 getCursorPositionInternal();

    void doDiagnostics();

    void postProcessUserInput(bool cursorMoved, bool leftMouseUp, Aya::Vector2 wrapMouseDelta, Aya::Vector2 mouseDelta);
    void mouseEventWasProcessed(bool datamodelSunkEvent, void* nativeEventObject, const shared_ptr<Aya::InputObject>& inputObject);

    void renderStepped();

public:
    shared_ptr<Aya::DataModel> dataModel;

    //	const static int WM_CALL_SETFOCUS = WM_USER + 187;
    //	HCURSOR hInvisibleCursor;

    ////////////////////////////////
    //
    // UserInputBase

    /*implement*/ Aya::Vector2 getCursorPosition();

    // todo: no longer used, remove this
    void doWrapHybrid(bool cursorMoved, bool leftMouseUp, G3D::Vector2& wrapMouseDelta, G3D::Vector2& wrapMousePosition, G3D::Vector2& posToWrapTo);

    /*implement*/ bool keyDown(Aya::KeyCode code) const;
    /*implement*/ void setKeyState(Aya::KeyCode code, Aya::ModCode modCode, char modifiedKey, bool isDown);
    bool isMiddleMouseDown() const
    {
        return middleMouseButtonDown;
    }
    bool isLeftMouseDown() const
    {
        return leftMouseButtonDown;
    }

    bool areEditModeMovementKeysDown();

    /*implement*/ void centerCursor()
    {
        wrapMousePosition = Aya::Vector2::zero();
    }

    /*override*/ Aya::TextureProxyBaseRef getGameCursor(Aya::Adorn* adorn);

    void PostUserInputMessage(Aya::InputObject::UserInputType eventType, Aya::InputObject::UserInputState eventState, int wParam, int lParam,
        char extraParam = 0, bool processed = false);
    // Call this only within a DataModel lock:
    void ProcessUserInputMessage(Aya::InputObject::UserInputType eventType, Aya::InputObject::UserInputState eventState, int wParam, int lParam,
        char extraParam = 0, bool processed = false);

    UserInput(RobloxView* wnd, shared_ptr<Aya::DataModel> dataModel);
    ~UserInput();

    void setDataModel(shared_ptr<Aya::DataModel> dataModel);

    void onMouseInside();
    void onMouseLeave();

    void resetKeyState();
};
