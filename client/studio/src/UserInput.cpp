


#include "UserInput.hpp"

// Roblox Headers
#include "DataModel/Workspace.hpp"

#include "DataModel/GameBasicSettings.hpp"

#include "DataModel/SleepingJob.hpp"

#include "DataModel/UserInputService.hpp"

#include "Utility/NavKeys.hpp"

#include "TaskScheduler.hpp"

#include "DataModel/RenderSettingsItem.hpp"

// Roblox Studio Headers
#include "RobloxView.hpp"
#include "UpdateUIManager.hpp"
#include "UserInputUtil.hpp"
#include "RobloxMainWindow.hpp"
#include "StudioUtilities.hpp"
#include "StudioDeviceEmulator.hpp"

LOGGROUP(UserProfile)

FASTFLAG(StudioRemoveUpdateUIThread)
DYNAMIC_FASTFLAGVARIABLE(MouseDeltaWhenNotMouseLocked, false)
DYNAMIC_FASTFLAGVARIABLE(UserInputViewportSizeFixStudio, true)

UserInput::UserInput(RobloxView* wnd, shared_ptr<Aya::DataModel> dataModel)
    : tableSize(0)
    , isMouseCaptured(false)
    , leftMouseButtonDown(false)
    , middleMouseButtonDown(false)
    , wrapping(false)
    , leftMouseDown(false)
    , rightMouseDown(false)
    , autoMouseMove(false)
    , wnd(wnd)
    , isMouseInside(false)
    , posToWrapTo(0, 0)
{
    setDataModel(dataModel);
}

void UserInput::setDataModel(shared_ptr<Aya::DataModel> dataModel)
{
    this->dataModel = dataModel;
    runService = shared_from(Aya::ServiceProvider::create<Aya::RunService>(dataModel.get()));

    sdlGameController = shared_ptr<SDLGameController>(new SDLGameController(dataModel));

    processedMouseConnection.disconnect();

    if (Aya::UserInputService* userInputService = Aya::ServiceProvider::find<Aya::UserInputService>(dataModel.get()))
    {
        processedMouseConnection = userInputService->processedMouseEvent.connect(boost::bind(&UserInput::mouseEventWasProcessed, this, _1, _2, _3));
    }
}

UserInput::~UserInput()
{
    sdlGameController.reset();
    runService.reset();
}

void UserInput::mouseEventWasProcessed(bool datamodelSunkEvent, void* nativeEventObject, const shared_ptr<Aya::InputObject>& inputObject)
{
    if (inputObject)
    {
        if (inputObject->getUserInputType() == Aya::InputObject::TYPE_MOUSEBUTTON1 &&
            inputObject->getUserInputState() == Aya::InputObject::INPUT_STATE_END)
        {
            UpdateUIManager::Instance().updateToolBars();
        }
        else if (inputObject->getUserInputType() == Aya::InputObject::TYPE_MOUSEWHEEL)
        {
            UpdateUIManager::Instance().updateAction(*UpdateUIManager::Instance().getMainWindow().zoomInAction);
            UpdateUIManager::Instance().updateAction(*UpdateUIManager::Instance().getMainWindow().zoomOutAction);
        }
    }
}

void UserInput::PostUserInputMessage(
    Aya::InputObject::UserInputType eventType, Aya::InputObject::UserInputState eventState, int wParam, int lParam, char extraParam, bool processed)
{
    FASTLOG4(FLog::UserInputProfile, "Pushing UI event to queue, event: %u, wParam: %u, lParam: %u, extraParam: %c", eventType, wParam, lParam,
        extraParam);

    if (eventType == Aya::InputObject::TYPE_MOUSEMOVEMENT)
        if (dataModel)
            dataModel->mouseStats.osMouseMove.sample();

    ProcessUserInputMessage(eventType, eventState, wParam, lParam, extraParam, processed);
}

void UserInput::ProcessUserInputMessage(
    Aya::InputObject::UserInputType eventType, Aya::InputObject::UserInputState eventState, int wParam, int lParam, char extraParam, bool processed)
{
    bool cursorMoved = false;
    bool leftMouseUp = false;
    Aya::Vector2 wrapMouseDelta;
    G3D::Vector2 mouseDelta;
    Aya::Vector3 cursorPosition = Aya::Vector3(getCursorPositionInternal(), 0);
    Aya::Vector3 deltaVec = Aya::Vector3(wrapMouseDelta.x, wrapMouseDelta.y, 0);

    switch (eventType)
    {
    case Aya::InputObject::TYPE_KEYBOARD:
    {
        Aya::KeyCode keyCode = (Aya::KeyCode)wParam;
        Aya::ModCode modCode = (Aya::ModCode)lParam;

        shared_ptr<Aya::InputObject> keyInput =
            Aya::Creatable<Aya::Instance>::create<Aya::InputObject>(eventType, eventState, keyCode, modCode, extraParam, dataModel.get());
        sendEvent(keyInput, processed);

        if (wParam < NKEYSTATES)
            if (!processed)
                setKeyState(keyCode, modCode, extraParam, (eventState == Aya::InputObject::INPUT_STATE_BEGIN));
        break;
    }
    case Aya::InputObject::TYPE_MOUSEBUTTON1:
    {
        G3D::Vector2 pos((float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam));

        if (eventState == Aya::InputObject::INPUT_STATE_BEGIN) // button down
        {
            leftMouseButtonDown = true;
            leftMouseUp = false;
            leftMouseDown = true;
            isMouseCaptured = true;
        }
        else if (eventState == Aya::InputObject::INPUT_STATE_END) // button up
        {
            leftMouseButtonDown = false;
            leftMouseUp = true;
            leftMouseDown = false;
            autoMouseMove = true;
            isMouseCaptured = false;
        }

        if (StudioDeviceEmulator::Instance().isEmulatingTouch() && StudioUtilities::isAvatarMode())
        {
            G3D::Vector2 pos;
            wnd->getCursorPos(&pos);
            Aya::Vector3 position = Aya::Vector3(pos, 0);

            if (eventState == Aya::InputObject::INPUT_STATE_BEGIN)
            {
                touchInput = Aya::Creatable<Aya::Instance>::create<Aya::InputObject>(
                    Aya::InputObject::TYPE_TOUCH, eventState, position, Aya::Vector3::zero(), dataModel.get());
            }
            else
            {
                touchInput->setInputState(Aya::InputObject::INPUT_STATE_END);
                touchInput->setPosition(position);
            }

            if (touchInput)
                sendEvent(touchInput);

            if (touchInput && eventState == Aya::InputObject::INPUT_STATE_END)
                touchInput.reset();
        }
        else
        {
            sendMouseEvent(eventType, eventState, cursorPosition, deltaVec);
        }

        wrapMousePosition = pos - getWindowRect().center();

        break;
    }
    case Aya::InputObject::TYPE_MOUSEBUTTON2:
    {
        G3D::Vector2 pos((float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam));
        sendMouseEvent(eventType, eventState, cursorPosition, deltaVec);

        rightMouseDown = (eventState == Aya::InputObject::INPUT_STATE_BEGIN) ? true : false;

        wrapMousePosition = pos - getWindowRect().center();

        break;
    }
    case Aya::InputObject::TYPE_MOUSEBUTTON3:
    {
        G3D::Vector2 pos((float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam));
        sendMouseEvent(eventType, eventState, cursorPosition, deltaVec);

        middleMouseButtonDown = (eventState == Aya::InputObject::INPUT_STATE_BEGIN) ? true : false;

        wrapMousePosition = pos - getWindowRect().center();

        break;
    }
    case Aya::InputObject::TYPE_MOUSEMOVEMENT:
    {
        Aya::UserInputService* userInputService = Aya::ServiceProvider::find<Aya::UserInputService>(dataModel.get());

        G3D::Vector2 windowCursorPos = getWindowsCursorPositionInternal() - getWindowRect().center();

        if (windowCursorPos == wrapMousePosition && (userInputService->getMouseWrapMode() == Aya::UserInputService::WRAP_CENTER ||
                                                        userInputService->getMouseWrapMode() == Aya::UserInputService::WRAP_NONEANDCENTER))
            return;

        G3D::Vector2 pos;
        wnd->getCursorPos(&pos);
        Aya::Vector3 position = Aya::Vector3(pos, 0);

        if (StudioDeviceEmulator::Instance().isEmulatingTouch() && StudioUtilities::isAvatarMode() && touchInput)
        {
            touchInput->setInputState(Aya::InputObject::INPUT_STATE_CHANGE);
            touchInput->setPosition(position);

            if (leftMouseDown)
                sendEvent(touchInput);
        }

        // if application doesn't have focus then just send mouse move events, do not do any mouse wrapping
        if (!wnd->hasApplicationFocus())
        {
            sendMouseEvent(Aya::InputObject::TYPE_MOUSEMOVEMENT, Aya::InputObject::INPUT_STATE_CHANGE, position, deltaVec);
            return;
        }

        mouseDelta = G3D::Vector2((float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam));

        autoMouseMove = false;
        cursorMoved = true;

        if (userInputService->getMouseWrapMode() != Aya::UserInputService::WRAP_HYBRID)
            doWrapMouse(mouseDelta, wrapMouseDelta);

        break;
    }
    case Aya::InputObject::TYPE_MOUSEWHEEL:
    {
        int zDelta = (int)wParam; // wheel rotation
        G3D::Vector2 pos((float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam));
        Aya::Vector3 wheelPos = Aya::Vector3(pos, zDelta);

        sendMouseEvent(Aya::InputObject::TYPE_MOUSEWHEEL, Aya::InputObject::INPUT_STATE_CHANGE, wheelPos, deltaVec);

        break;
    }
    case Aya::InputObject::TYPE_FOCUS:
    {
        shared_ptr<Aya::InputObject> event = Aya::Creatable<Aya::Instance>::create<Aya::InputObject>(
            Aya::InputObject::TYPE_FOCUS, eventState, Aya::Vector3(getCursorPositionInternal(), 0), Aya::Vector3::zero(), dataModel.get());

        sendEvent(event);
        break;
    }
    default:
    {
        break;
    }
    }

    /////////// DONE PROCESSING ////////////////
    postProcessUserInput(cursorMoved, leftMouseUp, wrapMouseDelta, mouseDelta);
}

void UserInput::postProcessUserInput(bool cursorMoved, bool leftMouseUp, Aya::Vector2 wrapMouseDelta, Aya::Vector2 mouseDelta)
{
    Aya::Vector3 deltaVec;

    if (DFFlag::MouseDeltaWhenNotMouseLocked)
    {
        deltaVec = Aya::Vector3(mouseDelta.x, mouseDelta.y, 0);
    }
    else
    {
        deltaVec = Aya::Vector3(wrapMouseDelta.x, wrapMouseDelta.y, 0);
    }

    Aya::UserInputService* userInputService = Aya::ServiceProvider::find<Aya::UserInputService>(dataModel.get());

    // kind of a hack to allows us to pan when mouse isn't moving
    if (userInputService->getMouseWrapMode() == Aya::UserInputService::WRAP_HYBRID && !dataModel->getMouseOverGui())
    {
        doWrapHybrid(cursorMoved, leftMouseUp, wrapMouseDelta, wrapMousePosition, posToWrapTo);
    }
    else if (Aya::GameBasicSettings::singleton().inMousepanMode())
    {
        if (leftMouseButtonDown && !rightMouseDown && !middleMouseButtonDown && Aya::GameBasicSettings::singleton().getCanMousePan())
            userInputService->setMouseWrapMode(Aya::UserInputService::WRAP_CENTER);
        else if (!rightMouseDown && !middleMouseButtonDown)
            userInputService->setMouseWrapMode(Aya::UserInputService::WRAP_AUTO);
    }

    if (userInputService->getMouseWrapMode() == Aya::UserInputService::WRAP_HYBRID)
        doWrapMouse(mouseDelta, wrapMouseDelta);

    Aya::Vector3 cursorPos = Aya::Vector3(getCursorPositionInternal(), 0);

    if (wrapMouseDelta != Aya::Vector2::zero())
    {
        if (userInputService->getMouseWrapMode() != Aya::UserInputService::WRAP_NONEANDCENTER &&
            userInputService->getMouseWrapMode() != Aya::UserInputService::WRAP_NONE)
        {
            if (dataModel->getWorkspace())
            {
                if (userInputService)
                {
                    userInputService->wrapCamera(wrapMouseDelta);
                }
            }
        }

        sendMouseEvent(Aya::InputObject::TYPE_MOUSEDELTA, Aya::InputObject::INPUT_STATE_CHANGE, cursorPos, deltaVec);
        sendMouseEvent(Aya::InputObject::TYPE_MOUSEMOVEMENT, Aya::InputObject::INPUT_STATE_CHANGE, cursorPos, deltaVec);
    }
    else if (cursorMoved)
    {
        sendMouseEvent(Aya::InputObject::TYPE_MOUSEMOVEMENT, Aya::InputObject::INPUT_STATE_CHANGE, cursorPos, deltaVec);
    }
}

void UserInput::sendMouseEvent(
    Aya::InputObject::UserInputType mouseEventType, Aya::InputObject::UserInputState mouseEventState, Aya::Vector3& position, Aya::Vector3& delta)
{
    FASTLOG1(FLog::UserInputProfile, "Processing mouse event from the queue, event: %u", mouseEventType);

    shared_ptr<Aya::InputObject> mouseEventObject;
    if (inputObjectMap.find(mouseEventType) == inputObjectMap.end())
    {
        mouseEventObject = Aya::Creatable<Aya::Instance>::create<Aya::InputObject>(mouseEventType, mouseEventState, position, delta, dataModel.get());
        inputObjectMap[mouseEventType] = mouseEventObject;
    }
    else
    {
        mouseEventObject = inputObjectMap[mouseEventType];

        // make sure we aren't sending redundant move events
        // only send if we have new position and a delta from last position
        if (mouseEventType == Aya::InputObject::TYPE_MOUSEMOVEMENT)
        {
            Aya::Vector3 currentMouseMovePos = mouseEventObject->getRawPosition();
            if (currentMouseMovePos == position && delta == Aya::Vector3::zero())
            {
                return;
            }
        }

        mouseEventObject->setInputState(mouseEventState);
        mouseEventObject->setPosition(position);
        mouseEventObject->setDelta(delta);
        inputObjectMap[mouseEventType] = mouseEventObject;
    }


    if (Aya::UserInputService* userInputService = Aya::ServiceProvider::find<Aya::UserInputService>(dataModel.get()))
        userInputService->fireInputEvent(mouseEventObject, NULL);
}

void UserInput::sendEvent(shared_ptr<Aya::InputObject> event, bool processed)
{
    if (Aya::UserInputService* userInputService = Aya::ServiceProvider::find<Aya::UserInputService>(dataModel.get()))
        userInputService->fireInputEvent(event, NULL, processed);
}

bool UserInput::isFullScreenMode() const
{
    return false;
}

G3D::Rect2D UserInput::getWindowRect() const
{
    float width = wnd->getWidth();
    float height = wnd->getHeight();
    if (DFFlag::UserInputViewportSizeFixStudio)
    {
        Aya::Camera* cam = this->dataModel.get()->getWorkspace()->getCamera();
        width = cam->getViewportWidth();
        height = cam->getViewportHeight();
    }
    G3D::Rect2D answer(G3D::Vector2(width, height));
    return answer;
}

G3D::Vector2int16 UserInput::getWindowSize() const
{
    G3D::Rect2D windowRect = getWindowRect();
    return G3D::Vector2int16((G3D::int16)windowRect.width(), (G3D::int16)windowRect.height());
}

Aya::TextureProxyBaseRef UserInput::getGameCursor(Aya::Adorn* adorn)
{
    if (isMouseInside)
    {
        if (Aya::UserInputService* inputService = Aya::ServiceProvider::find<Aya::UserInputService>(dataModel.get()))
        {
            Aya::TextureId mouseTexture = inputService->getCurrentMouseIcon();

            if (!mouseTexture.isNull())
            {
                bool waitingCurrentCursor;
                return adorn->createTextureProxy(mouseTexture, waitingCurrentCursor, false, "Mouse Cursor");
            }
        }
        return UserInputBase::getGameCursor(adorn);
    }

    return Aya::TextureProxyBaseRef();
}

void UserInput::doWrapHybrid(bool, bool leftMouseUp, G3D::Vector2& wrapMouseDelta, G3D::Vector2& wrapMousePosition, G3D::Vector2& posToWrapTo) {}

bool UserInput::movementKeysDown()
{
    return (keyDownInternal(Aya::AYA_SDLK_w) || keyDownInternal(Aya::AYA_SDLK_s) || keyDownInternal(Aya::AYA_SDLK_a) || keyDownInternal(Aya::AYA_SDLK_d) ||
            keyDownInternal(Aya::AYA_SDLK_UP) || keyDownInternal(Aya::AYA_SDLK_DOWN) || keyDownInternal(Aya::AYA_SDLK_LEFT) || keyDownInternal(Aya::AYA_SDLK_RIGHT));
}

bool UserInput::areEditModeMovementKeysDown()
{
    return (movementKeysDown() || keyDownInternal(Aya::AYA_SDLK_q) || keyDownInternal(Aya::AYA_SDLK_e));
}

G3D::Vector2 UserInput::getGameCursorPositionInternal()
{
    return getWindowRect().center() + wrapMousePosition;
}

G3D::Vector2 UserInput::getGameCursorPositionExpandedInternal()
{
    return getWindowRect().center() + Aya::Math::expandVector2(wrapMousePosition, 10);
}


// will be called repeatedly. ignore repeated calls.
void UserInput::onMouseInside()
{
    G3D::Vector2 pos;
    wnd->getCursorPos(&pos);
    // wrapMousePosition = Aya::Math::expandVector2(pos - getWindowRect().center(), -10);	// i.e. - pull towards the origin - remove chatter
    wrapMousePosition = pos - getWindowRect().center();

    if (isMouseInside)
        return; // we know. ignore.

    if (Aya::UserInputService* inputService = Aya::ServiceProvider::find<Aya::UserInputService>(dataModel.get()))
    {
        if (inputService->getMouseWrapMode() == Aya::UserInputService::WRAP_NONEANDCENTER)
        {
            centerCursor();
        }
    }

    isMouseInside = true;
};

void UserInput::onMouseLeave()
{
    if (!isMouseInside)
        return;

    if (rightMouseDown)
        PostUserInputMessage(Aya::InputObject::TYPE_MOUSEBUTTON2, Aya::InputObject::INPUT_STATE_END, 0, 0);

    isMouseInside = false;
};

G3D::Vector2 UserInput::getWindowsCursorPositionInternal()
{
    G3D::Vector2 pt;
    wnd->getCursorPos(&pt);
    return pt;
}

G3D::Vector2 UserInput::getCursorPosition()
{
    return getCursorPositionInternal();
}

G3D::Vector2 UserInput::getCursorPositionInternal()
{
    return getGameCursorPositionInternal();
}

bool UserInput::keyDownInternal(Aya::KeyCode code) const
{
    if (Aya::UserInputService* userInputService = Aya::ServiceProvider::find<Aya::UserInputService>(dataModel.get()))
        return userInputService->isKeyDown(code);

    return false;
}

bool UserInput::keyDown(Aya::KeyCode code) const
{
    return keyDownInternal(code);
}

void UserInput::setKeyState(Aya::KeyCode code, Aya::ModCode modCode, char modifiedKey, bool isDown)
{
    if (Aya::UserInputService* userInputService = Aya::ServiceProvider::find<Aya::UserInputService>(dataModel.get()))
        userInputService->setKeyState(code, modCode, modifiedKey, isDown);
}

void UserInput::resetKeyState()
{
    if (Aya::UserInputService* userInputService = Aya::ServiceProvider::find<Aya::UserInputService>(dataModel.get()))
        userInputService->resetKeyState();
}

bool preventWrapMouse()
{
    // unlikely to get here
    return true;
}

void UserInput::doWrapMouse(const G3D::Vector2& delta, G3D::Vector2& wrapMouseDelta)
{
    Aya::Vector2 savedPos = getGameCursorPositionInternal();
    Aya::UserInputService* userInputService = Aya::ServiceProvider::find<Aya::UserInputService>(dataModel.get());

    switch (userInputService->getMouseWrapMode())
    {
    case Aya::UserInputService::WRAP_NONE:
    case Aya::UserInputService::WRAP_NONEANDCENTER:
    case Aya::UserInputService::WRAP_CENTER: // intentional fall-thru
    {
        UserInputUtil::wrapMouseCenter(delta, wrapMouseDelta, this->wrapMousePosition);
        break;
    }
    case Aya::UserInputService::WRAP_HYBRID:
    {
        UserInputUtil::wrapMousePos(delta, wrapMouseDelta, wrapMousePosition, getWindowSize(), posToWrapTo, autoMouseMove);
        break;
    }
    case Aya::UserInputService::WRAP_AUTO:
    {
        if (movementKeysDown() || isMouseCaptured)
        {                                             // 1. If movement keys are down - keep the mouse in the window
            UserInputUtil::wrapMouseBorderLock(delta, // 2. If the left mouse button is down (we are dragging) - keep in the window
                wrapMouseDelta, this->wrapMousePosition, getWindowSize());
        }
        else if (preventWrapMouse())
        { // 3. If using toolbox, prevent wrapping
            UserInputUtil::wrapMouseNone(delta, wrapMouseDelta, this->wrapMousePosition);
        }
        else if (isFullScreenMode())
        { // 4. OK - we're in PlayMode (i.e. character)
            UserInputUtil::wrapFullScreen(delta, wrapMouseDelta, this->wrapMousePosition, getWindowSize());
        }
        else
        {
            // We no longer want mouse wrap and camera auto-pan at the horizontal window extents.
            UserInputUtil::wrapMouseNone(delta, wrapMouseDelta, this->wrapMousePosition);
        }
        break;
    }
    }

    if (userInputService->getMouseWrapMode() != Aya::UserInputService::WRAP_HYBRID)
    {
        Aya::Vector2 pos = getGameCursorPositionInternal();
        wnd->setCursorPos(&pos, leftMouseButtonDown, pos == savedPos);
    }
}
