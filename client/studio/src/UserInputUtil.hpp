

#pragma once

// 3rd Party Headers
#include "Vector2.hpp"

// Roblox Headers
#include "Utility/KeyCode.hpp"

#include "Utility/G3DCore.hpp"

#include "Debug.hpp"

#include "DataModel/InputObject.hpp"

#define GET_X_LPARAM(l) ((short)(l & 0x0000FFFF))
#define GET_Y_LPARAM(l) ((short)((l & 0xFFFF0000) >> 16))

#define MAKEXYLPARAM(x, y) (((unsigned short)x) | ((unsigned short)y) << 16)


class UserInputUtil
{
private:
    static void wrapMouseBorder(const G3D::Vector2& delta, G3D::Vector2& wrapMouseDelta, G3D::Vector2& wrapMousePosition,
        const G3D::Vector2& windowSize, const int borderWidth, const float creepFactor);

public:
    typedef uint8_t DiKeys[256];

    static const float HybridSensitivity;
    static const float MouseTug;

    static bool isCtrlDown(Aya::ModCode modCode);
    static Aya::InputObject::UserInputState msgToEventState(unsigned int uMsg);
    static Aya::InputObject::UserInputType msgToEventType(unsigned int uMsg);

    static Aya::ModCode createModCode(const DiKeys& diKeys);
    static DWORD keyCodeToDirectInput(Aya::KeyCode keyCode);
    static Aya::KeyCode directInputToKeyCode(DWORD diKey);
    static DWORD keyCodeToVK(Aya::KeyCode diKey);

    static void wrapMouseNone(const G3D::Vector2& delta, G3D::Vector2& wrapMouseDelta, G3D::Vector2& wrapMousePosition);

    static void wrapFullScreen(
        const G3D::Vector2& delta, G3D::Vector2& wrapMouseDelta, G3D::Vector2& wrapMousePosition, const G3D::Vector2& windowSize);

    static void wrapMouseHorizontalTransition(
        const G3D::Vector2& delta, G3D::Vector2& wrapMouseDelta, G3D::Vector2& wrapMousePosition, const G3D::Vector2& windowSize);

    static void wrapMouseBorderLock(
        const G3D::Vector2& delta, G3D::Vector2& wrapMouseDelta, G3D::Vector2& wrapMousePosition, const G3D::Vector2& windowSize);

    static void wrapMouseBorderTransition(
        const G3D::Vector2& delta, G3D::Vector2& wrapMouseDelta, G3D::Vector2& wrapMousePosition, const G3D::Vector2& windowSize);

    static void wrapMouseCenter(const G3D::Vector2& delta, G3D::Vector2& wrapMouseDelta, G3D::Vector2& wrapMousePosition);

    static void wrapMouseHorizontalCenter(const G3D::Vector2& delta, G3D::Vector2& wrapMouseDelta, G3D::Vector2& wrapMousePosition);

    static void wrapMousePos(const G3D::Vector2& delta, G3D::Vector2& wrapMouseDelta, G3D::Vector2& wrapMousePosition, const G3D::Vector2& windowSize,
        G3D::Vector2& posToWrapTo, bool autoMoveMouse);
};
