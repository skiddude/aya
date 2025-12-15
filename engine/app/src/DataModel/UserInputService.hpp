//
//  UserInputService.h
//  App
//
//  Created by Ben Tkacheff on 8/28/12.
//
//
#pragma once

#include "Utility/TextureId.hpp"
#include "DataModel/DataModel.hpp"
#include "DataModel/InputObject.hpp"

#define NKEYSTATES 512

namespace Aya
{
class Humanoid;
class Camera;
class Frame;
class ScreenGui;

extern const char* const sUserInputService;

class UserInputService
    : public DescribedCreatable<UserInputService, Instance, sUserInputService, Reflection::ClassDescriptor::INTERNAL_LOCAL>
    , public Service
{
public:
    typedef std::vector<shared_ptr<InputObject>> InputObjectVector;

    typedef enum
    {
        WRAP_AUTO = 0,
        WRAP_CENTER,
        WRAP_HYBRID,
        WRAP_NONEANDCENTER,
        WRAP_NONE
    } WrapMode;


    typedef enum
    {
        MOUSETYPE_DEFAULT = 0,
        MOUSETYPE_LOCKCENTER,
        MOUSETYPE_LOCKCURRENT
    } MouseType;

    typedef enum
    {
        OVERRIDE_BEHAVIOR_NONE = 0,
        OVERRIDE_BEHAVIOR_FORCESHOW,
        OVERRIDE_BEHAVIOR_FORCEHIDE
    } OverrideMouseIconBehavior;


    typedef enum
    {
        GESTURE_TAP = 0,
        GESTURE_LONGPRESS,
        GESTURE_SWIPE,
        GESTURE_PINCH,
        GESTURE_ROTATE,
        GESTURE_PAN,
        GESTURE_NONE
    } Gesture;

    enum UserCFrame
    {
        USERCFRAME_HEAD,
        USERCFRAME_LEFTHAND,
        USERCFRAME_RIGHTHAND,

        USERCFRAME_COUNT
    };

private:
    struct KeyboardData
    {
        Aya::KeyCode keyCode;
        ModCode modCode;
        char modifiedKey;
        bool isDown;
    };

    typedef DescribedCreatable<UserInputService, Instance, sUserInputService, Reflection::ClassDescriptor::INTERNAL_LOCAL> Super;

    typedef std::vector<std::pair<shared_ptr<InputObject>, void*>> EventsVector;
    typedef std::vector<std::pair<UserInputService::Gesture, std::pair<shared_ptr<Aya::Reflection::ValueArray>, shared_ptr<const Reflection::Tuple>>>>
        GestureEventsVector;

    typedef std::vector<std::pair<shared_ptr<std::string>, bool>> TextBoxEventsVector;

    // for touch input debugging
    static bool DrawTouchEvents;
    static Aya::Color3 DrawTouchColor;
    static Aya::Color3 DrawMoveColor;
    shared_ptr<Aya::Frame> drawTouchFrame;
    shared_ptr<Instance> currentTextBox;

    bool touchEnabled;
    bool keyboardEnabled;
    bool mouseEnabled;
    bool gamepadEnabled;
    bool accelerometerEnabled;
    bool gyroscopeEnabled;

    InputObject::UserInputType lastInputType;

    bool modalEnabled;

    bool localCharacterJumpEnabled;

    static bool isStudioEmulatingMobile;

    bool isJump;

    std::vector<TextureId> mouseIconStack;

    shared_ptr<Aya::InputObject> mouseEventObject;

    float maxWalkDelta;
    Aya::Vector2 walkDirection;
    Aya::Vector2 lastWalkDirection;

    boost::mutex GestureEventsMutex;
    boost::mutex ToolEventsMutex;
    boost::mutex TextboxEventsMutex;
    boost::mutex MotionEventsMutex;
    boost::mutex KeyboardEventsMutex;

    EventsVector changeEventsToProcess;
    EventsVector beginEventsToProcess;
    EventsVector endEventsToProcess;
    EventsVector tempEndEvents;
    GestureEventsVector gestureEventsToProcess;
    std::vector<KeyboardData> keyboardEventsToProcess;

    InputObjectVector currentTouches;
    InputObjectVector toolEventsVector;

    TextBoxEventsVector textboxFinishedVector;

    Aya::Vector2 cameraPanDelta;
    float cameraZoom;
    Aya::Vector2 cameraMouseTrack;
    Aya::Vector2 cameraMouseWrap;

    bool fireJumpRequestEvent;

    bool mouseIconEnabled; // Devs use this to hide mouse in their game
    MouseType mouseType;
    OverrideMouseIconBehavior overrideMouseIconBehavior; // Core Scripts use this to determine whether mouse shows in menus

    // this is used to store mouse events for touch input (for legacy compatibility with gui objects)
    boost::unordered_map<InputObject::UserInputType, shared_ptr<InputObject>> fakeMouseEventsMap;

    boost::unordered_map<InputObject::UserInputType, shared_ptr<const Reflection::ValueArray>> supportedGamepadKeyCodes;
    boost::unordered_map<InputObject::UserInputType, bool> connectedGamepadsMap;
    weak_ptr<GuiObject> lastDownGuiObjectLeft;
    weak_ptr<GuiObject> lastDownGuiObjectRight;

    bool shouldFireAccelerationEvent;
    bool shouldFireRotationEvent;
    bool shouldFireGravityEvent;
    shared_ptr<Instance> rotationInputObject;
    CoordinateFrame rotationCFrame;
    shared_ptr<Instance> accelerationInputObject;
    shared_ptr<Instance> gravityInputObject;

    void processMotionEvents();

    // Mouse Stuff
    WrapMode wrapMode;
    shared_ptr<InputObject> currentMousePosition;


    // Keyboard Stuff
    boost::unordered_map<Aya::KeyCode, shared_ptr<InputObject>> newKeyState;
    std::vector<std::pair<Aya::KeyCode, Aya::ModCode>> modPairs;
    bool capsLocked;

    int studioCamFlySteps;

    // VR Stuff
    bool vrEnabled;
    bool recenterUserHeadCFrameRequested;

    void processToolEvents();
    void processCameraInternal();
    void processTextboxInternal();
    void processKeyboardEvents();

    Aya::ModCode getCurrentModKey();

    void clearKeyStateInternal();
    void setKeyStateInternal(Aya::KeyCode keyCode, ModCode modCode, char modifiedKey, bool isDown);

    void fireLegacyMouseEvent(const shared_ptr<InputObject>& inputObject, void* nativeInputObject, bool fireImmediately = false);

    void signalInputEventOnService(
        const shared_ptr<InputObject>& inputObject, const InputObject::UserInputState& inputState, bool processedEvent, bool menuIsOpen);

    void processInputVector(EventsVector& eventVector, const InputObject::UserInputState& inputState);
    bool inputVectorHasInputPair(const std::pair<shared_ptr<InputObject>, void*>& eventPair, const EventsVector& vectorToCheck);

    bool eraseInputTypeFromInputVector(const InputObject::UserInputType inputType, EventsVector& vectorToEraseFrom);
    void eraseInputPairFromInputVector(const std::pair<shared_ptr<InputObject>, void*>& eventPair, EventsVector& vectorToCheck);

    void processGestures();
    void fireGestureEvent(const UserInputService::Gesture gesture, const shared_ptr<const Aya::Reflection::ValueArray>& touchPositions,
        const shared_ptr<const Reflection::Tuple>& args, const bool wasSunk = false);

    void processInputObjects();

    static void moveLocalCharacterInternal(weak_ptr<DataModel> weakDataModel, Vector2 walkDir, float walkDelta);
    static void setLocalHumanoidJump(weak_ptr<DataModel> weakDataModel, bool isJump);

    void doDataModelProcessInput(
        DataModel* dataModel, const shared_ptr<InputObject>& inputObject, void* nativeInputObject, const InputObject::UserInputState& inputState);
    void dataModelProcessInput(const shared_ptr<InputObject>& inputObject, void* nativeInputObject, const InputObject::UserInputState& inputState);

    void updateLastInputType(const shared_ptr<InputObject>& inputObject);

    void updateCurrentMousePosition(const shared_ptr<InputObject>& inputObject);

    void cleanupCurrentTouches();

public:
    UserInputService();
    ~UserInputService() {}

    typedef enum
    {
        DIRECTION_RIGHT = 0,
        DIRECTION_LEFT,
        DIRECTION_UP,
        DIRECTION_DOWN,
        DIRECTION_NONE
    } SwipeDirection;

    typedef enum
    {
        PLATFORM_WINDOWS = 0,
        PLATFORM_OSX,
        PLATFORM_IOS,
        PLATFORM_ANDROID,
        PLATFORM_XBOXONE,
        PLATFORM_PS4,
        PLATFORM_PS3,
        PLATFORM_XBOX360,
        PLATFORM_WIIU,
        PLATFORM_NX,
        PLATFORM_OUYA,
        PLATFORM_ANDROIDTV,
        PLATFORM_CHROMECAST,
        PLATFORM_LINUX,
        PLATFORM_STEAMOS,
        PLATFORM_WEBOS,
        PLATFORM_DOS,
        PLATFORM_BEOS,
        PLATFORM_UWP,
        PLATFORM_NONE
    } Platform;

    static Reflection::EventDesc<UserInputService, void(shared_ptr<Instance>, bool), Aya::signal<void(shared_ptr<Instance>, bool)>,
        Aya::signal<void(shared_ptr<Instance>, bool)>* (UserInputService::*)(bool)>
        event_InputBegin;
    static Reflection::EventDesc<UserInputService, void(shared_ptr<Instance>, bool), Aya::signal<void(shared_ptr<Instance>, bool)>,
        Aya::signal<void(shared_ptr<Instance>, bool)>* (UserInputService::*)(bool)>
        event_InputUpdate;
    static Reflection::EventDesc<UserInputService, void(shared_ptr<Instance>, bool), Aya::signal<void(shared_ptr<Instance>, bool)>,
        Aya::signal<void(shared_ptr<Instance>, bool)>* (UserInputService::*)(bool)>
        event_InputEnd;

    static bool IsUsingNewKeyboardEvents();

    // Signals
    // IF THESE ARE BEING CALLED DIRECTLY FROM OUTSIDE DATAMODEL, STOP THAT!
    // could cause a deadlock, please submit a task to datamodel and call the signal
    Aya::signal<void(bool, void*, const shared_ptr<Aya::InputObject>&)> processedMouseEvent;
    Aya::signal<void(shared_ptr<Instance>, bool)> processedEventSignal;

    // touch gesture event signals
    Aya::signal<void(shared_ptr<const Reflection::ValueArray>, bool)> tapGestureEvent;
    Aya::signal<void(shared_ptr<const Reflection::ValueArray>, float, float, InputObject::UserInputState, bool)> pinchGestureEvent;
    Aya::signal<void(SwipeDirection, int, bool)> swipeGestureEvent;
    Aya::signal<void(shared_ptr<const Reflection::ValueArray>, InputObject::UserInputState, bool)> longPressGestureEvent;
    Aya::signal<void(shared_ptr<const Reflection::ValueArray>, float, float, InputObject::UserInputState, bool)> rotateGestureEvent;
    Aya::signal<void(shared_ptr<const Reflection::ValueArray>, Aya::Vector2, Aya::Vector2, InputObject::UserInputState, bool)> panGestureEvent;

    // low level touch event signals
    Aya::signal<void(shared_ptr<Instance>, bool)> touchStartedEvent;
    Aya::signal<void(shared_ptr<Instance>, bool)> coreTouchStartedEvent;

    Aya::signal<void(shared_ptr<Instance>, bool)> touchEndedEvent;
    Aya::signal<void(shared_ptr<Instance>, bool)> coreTouchEndedEvent;

    Aya::signal<void(shared_ptr<Instance>, bool)> touchMovedEvent;
    Aya::signal<void(shared_ptr<Instance>, bool)> coreTouchMovedEvent;

    Aya::signal<void(shared_ptr<Instance>, bool)>* getTouchBeganEvent(bool whatever = true);
    Aya::signal<void(shared_ptr<Instance>, bool)>* getTouchChangedEvent(bool whatever = true);
    Aya::signal<void(shared_ptr<Instance>, bool)>* getTouchEndedEvent(bool whatever = true);

    // low level generic event signals
    Aya::signal<void(shared_ptr<Instance>, bool)> inputBeganEvent;
    Aya::signal<void(shared_ptr<Instance>, bool)> coreInputBeganEvent;

    Aya::signal<void(shared_ptr<Instance>, bool)> inputUpdatedEvent;
    Aya::signal<void(shared_ptr<Instance>, bool)> coreInputUpdatedEvent;

    Aya::signal<void(shared_ptr<Instance>, bool)> inputEndedEvent;
    Aya::signal<void(shared_ptr<Instance>, bool)> coreInputEndedEvent;

    Aya::signal<void(shared_ptr<Instance>, bool)>* getInputBeganEvent(bool whatever = true);
    Aya::signal<void(shared_ptr<Instance>, bool)>* getInputChangedEvent(bool whatever = true);
    Aya::signal<void(shared_ptr<Instance>, bool)>* getInputEndedEvent(bool whatever = true);

    // Motion Events
    Aya::signal<void(shared_ptr<Instance>, CoordinateFrame)> gyroChangedSignal;
    Aya::signal<void(shared_ptr<Instance>)> accelerometerChangedSignal;
    Aya::signal<void(shared_ptr<Instance>)> gravityChangedSignal;

    Aya::signal<void(shared_ptr<Instance>, CoordinateFrame)>* getOrCreateScriptGyroEventSignal(bool create = true);
    Aya::signal<void(shared_ptr<Instance>)>* getOrCreateScriptAccelerometerEventSignal(bool create = true);
    Aya::signal<void(shared_ptr<Instance>)>* getOrCreateScriptGravityEventSignal(bool create = true);

    Aya::signal<void(std::string)> motionEventListeningStarted;

    // Text Events
    Aya::signal<void(shared_ptr<Aya::Instance>)> textBoxGainFocus;
    Aya::signal<void(shared_ptr<Aya::Instance>)> textBoxReleaseFocus;
    Aya::signal<void(const char*, bool, const shared_ptr<InputObject>&)> textBoxFinishedEditing;

    Aya::signals::connection textboxFocusBeganConnection;
    Aya::signals::connection textboxFocusEndedConnection;

    // Gamepad Events
    Aya::signal<void(InputObject::UserInputType)> gamepadDisconnectedSignal;
    Aya::signal<void(InputObject::UserInputType)> gamepadConnectedSignal;
    Aya::signal<void(InputObject::UserInputType)> getSupportedGamepadKeyCodesSignal;

    // Window Event
    Aya::signal<void()> windowFocused;
    Aya::signal<void()> windowFocusReleased;

    // Misc Events
    Aya::signal<void()> jumpRequestEvent;
    Aya::signal<void(bool)> mouseIconEnabledEvent;
    Aya::signal<void()> updateInputSignal;
    Aya::signal<void(InputObject::UserInputType)> lastInputTypeChangedSignal;

    // Touch Functions
    InputObjectVector getCurrentTouches()
    {
        return currentTouches;
    }

    // Motion Functions
    shared_ptr<Instance> getAcceleration();
    shared_ptr<const Reflection::Tuple> getRotation();
    shared_ptr<Instance> getGravity();

    void fireAccelerationEvent(const Aya::Vector3& newAcceleration);
    void fireRotationEvent(const Aya::Vector3& newRotation, const Aya::Vector4& quaternion);
    void fireGravityEvent(const Aya::Vector3& newGravity);

    // take a lock on this whenever editing InputObjects stored in UserInputService
    static boost::mutex InputEventsMutex;

    void onRenderStep();

    // Mouse Functions
    void setMouseWrapMode(WrapMode set)
    {
        wrapMode = set;
    }
    WrapMode getMouseWrapMode() const;

    shared_ptr<InputObject> getCurrentMousePosition() const
    {
        return currentMousePosition;
    }
    void setCurrentMousePosition(shared_ptr<InputObject> value)
    {
        currentMousePosition = value;
    }

    bool isMouseOverGui();

    ContentId getDefaultMouseCursor(bool hoverOverActive, bool relativePathNoExtension = false);
    TextureId getCurrentMouseIcon();
    void pushMouseIcon(TextureId newMouseIcon);
    void popMouseIcon();
    void popMouseIcon(TextureId mouseIconToRemove);

    void setMouseIconEnabled(bool enabled);
    bool getMouseIconEnabled() const
    {
        return mouseIconEnabled;
    }

    void setMouseType(MouseType value);
    MouseType getMouseType() const
    {
        return mouseType;
    }

    void setOverrideMouseIconBehavior(OverrideMouseIconBehavior value)
    {
        overrideMouseIconBehavior = value;
    }
    OverrideMouseIconBehavior getOverrideMouseIconBehavior() const
    {
        return overrideMouseIconBehavior;
    }

    void setLastDownGuiObject(weak_ptr<GuiObject> object, InputObject::UserInputType type);
    weak_ptr<GuiObject> getLastDownGuiObject(InputObject::UserInputType type);

    // Keyboard Functions
    static char getModifiedKey(Aya::KeyCode key, Aya::ModCode mod);

    void setKeyState(Aya::KeyCode keyCode, ModCode m, char key, bool isDown);
    bool isKeyDown(Aya::KeyCode keyCode);
    bool isCapsLocked();
    void resetKeyState();
    shared_ptr<const Reflection::ValueArray> getKeyboardState();

    Vector2 getKeyboardGuiSelectionDirection(const shared_ptr<InputObject>& inputObject);

    bool isCtrlDown()
    {
        return isKeyDown(AYA_SDLK_LCTRL) || isKeyDown(AYA_SDLK_RCTRL);
    }
    bool isShiftDown()
    {
        return isKeyDown(AYA_SDLK_LSHIFT) || isKeyDown(AYA_SDLK_RSHIFT);
    }
    bool isAltDown()
    {
        return isKeyDown(AYA_SDLK_LALT) || isKeyDown(AYA_SDLK_RALT);
    }

    // Gamepad Functions
    void safeFireGamepadConnected(InputObject::UserInputType gamepadType);
    void safeFireGamepadDisconnected(InputObject::UserInputType gamepadType);

    void setConnectedGamepad(InputObject::UserInputType gamepadType, bool isConnected);
    bool getGamepadConnected(InputObject::UserInputType gamepadType);
    shared_ptr<const Reflection::ValueArray> getConnectedGamepads();

    void setSupportedGamepadKeyCodes(InputObject::UserInputType gamepadType, shared_ptr<const Reflection::ValueArray> newSupportedGamepadKeyCodes);
    shared_ptr<const Reflection::ValueArray> getSupportedGamepadKeyCodes(InputObject::UserInputType gamepadType);
    bool gamepadSupports(InputObject::UserInputType gamepadType, Aya::KeyCode keyCode);
    shared_ptr<const Reflection::ValueArray> getGamepadState(InputObject::UserInputType gamepadType);

    shared_ptr<const Reflection::ValueArray> getNavigationGamepads();
    bool isNavigationGamepad(InputObject::UserInputType gamepadType);
    void setNavigationGamepad(InputObject::UserInputType gamepadType, bool enabled);

    // Instance
    /*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

    // fires input events on datamodel w/o taking lock!!!! If you use this make sure you have lock! (usually called from renderstepped externally)
    void dangerousFireInputEvent(const shared_ptr<InputObject>& inputObject, void* nativeInputObject);
    // Takes a lock on datamodel, and will update input on render stepped
    void fireInputEvent(const shared_ptr<InputObject>& inputObject, void* nativeInputObject, bool processed = false);
    void addGestureEventToProcess(
        const UserInputService::Gesture gesture, shared_ptr<Aya::Reflection::ValueArray>& touchPositions, shared_ptr<Reflection::Tuple>& args);

    // Interaction styles
    bool getTouchEnabled() const;
    bool getKeyboardEnabled() const;
    bool getMouseEnabled() const;
    bool getGamepadEnabled() const;
    bool getAccelerometerEnabled() const;
    bool getGyroscopeEnabled() const;

    void setTouchEnabled(bool value);
    void setKeyboardEnabled(bool value);
    void setMouseEnabled(bool value);
    void setGamepadEnabled(bool value);
    void setAccelerometerEnabled(bool value);
    void setGyroscopeEnabled(bool value);

    static Platform getPlatform();

    Platform getPlatformLua()
    {
        return getPlatform();
    }

    static bool isTenFootInterface();

    bool getModalEnabled() const;
    void setModalEnabled(bool value);

    bool getStudioEmulatingMobileEnabled();
    void setStudioEmulatingMobileEnabled(bool value);

    InputObject::UserInputType getLastInputType()
    {
        return lastInputType;
    }

    // textBox functions
    void textboxDidFinishEditing(const char* text, bool shouldReturn);
    void textboxFocused(shared_ptr<Instance> textBox);
    void textboxFocusReleased(shared_ptr<Instance> textBox);
    shared_ptr<Instance> getFocusedTextBox();

    // Show Stats based on Input
    bool showStatsBasedOnInputString(const char* text);

    // Local character switches
    bool getLocalCharacterJumpEnabled() const;
    void setLocalCharacterJumpEnabled(bool value);

    // text functions
    std::string getPasteText();
    std::vector<Aya::ModCode> getCommandModCodes(); // returns control for windows, command for os x, etc.

    // local character manipulations
    void jumpLocalCharacterLua();
    void moveLocalCharacterLua(Vector2 walkDir, float maxWalkDelta);
    void moveLocalCharacter(Aya::Vector2 movementVector, float maxMovementDelta);
    void jumpLocalCharacter(bool jumpValue);
    void jumpOnceLocalCharacter(bool jumpValue);

    // camera manipulations
    void rotateCameraLua(Aya::Vector2 mouseDelta);
    void rotateCamera(Aya::Vector2 mouseDelta);
    void wrapCamera(Aya::Vector2 mouseWrap);

    void zoomCameraLua(float zoom);
    void zoomCamera(float zoom);

    void mouseTrackCamera(Aya::Vector2 mouseDelta);

    // raw input mechanisms
    void sendMouseEvent(const shared_ptr<Aya::InputObject>& event, void* nativeEventObject);
    void processToolEvent(const shared_ptr<Aya::InputObject>& event);
    void sendJumpRequestEvent();


    static Reflection::PropDescriptor<UserInputService, bool> prop_ModalEnabled;
    bool getVREnabled() const
    {
        return false;
    }

    void recenterUserHeadCFrame();

    Aya::signal<void(UserCFrame, CoordinateFrame)> userCFrameChanged;

    // internal (to client, don't access from lua)

    // returns walkDirection, which is a Vector2 that is the raw input from user (not translated into any sort of actual world coordinates)
    Vector2 getInputWalkDirection()
    {
        return walkDirection;
    }
    // returns maxWalkDelta, which is a float that is the max value that x or y can be in walkDirection
    float getMaxInputWalkValue()
    {
        return maxWalkDelta;
    }

    bool canUseMouseLockCenter() const;
};
} // namespace Aya
