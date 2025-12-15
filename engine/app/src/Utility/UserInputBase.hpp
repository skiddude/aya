#pragma once

#include "Utility/KeyCode.hpp"
#include "Utility/G3DCore.hpp"
#include "Base/TextureProxyBase.hpp"
#include "Utility/ContentId.hpp"
#include "Utility/TextureId.hpp"
#include "Utility/Object.hpp"
#include "Base/Adorn.hpp"

LOGGROUP(UserInputProfile)

namespace Aya
{


class Adorn;
class NavKeys;

// TODO: Rename HardwareDevice or something
class AyaBaseClass UserInputBase
{
private:
    ContentId currentCursorId;
    TextureProxyBaseRef currentCursor;
    TextureProxyBaseRef fallbackCursor;
    Aya::signals::scoped_connection unbindResourceSignal;

    void onUnbindResourceSignal();

protected:
    virtual Vector2 getCursorPosition() = 0;
    virtual TextureProxyBaseRef getGameCursor(Adorn* adorn);
    TextureProxyBaseRef getCurrentCursor()
    {
        return currentCursor;
    }
    TextureProxyBaseRef getFallbackCursor()
    {
        return fallbackCursor;
    }

public:
    UserInputBase();
    ~UserInputBase() {}

    Aya::signal<void()> cursorIdChangedSignal;

    // This function is purely intended for debugging and diagnostics
    Vector2 getCursorPositionForDebugging()
    {
        return getCursorPosition();
    }

    virtual void removeJobs() {}

    /////////////////////////////////////////////////////////////////////
    // Mouse Wrapping
    //
    virtual void centerCursor() = 0;

    /////////////////////////////////////////////////////////////////////
    // Real-time Key Handling
    //
    virtual bool keyDown(KeyCode code) const = 0;

    void getNavKeys(NavKeys& navKeys, const bool shouldSuppressNavKeys) const;

    bool altKeyDown() const
    {
        return keyDown(AYA_SDLK_RALT) || keyDown(AYA_SDLK_LALT);
    }
    bool shiftKeyDown() const
    {
        return keyDown(AYA_SDLK_RSHIFT) || keyDown(AYA_SDLK_LSHIFT);
    }
    bool ctrlKeyDown() const
    {
        return keyDown(AYA_SDLK_RCTRL) || keyDown(AYA_SDLK_LCTRL);
    }

    // allows Gui Key buttons to "press" keys
    virtual void setKeyState(Aya::KeyCode code, Aya::ModCode modCode, char modifiedKey, bool isDown) = 0;

    /////////////////////////////////////////////////////////////////////
    // Cursor Handling
    //
    ContentId getCurrentCursorId()
    {
        return currentCursorId;
    }
    virtual bool setCursorId(Aya::Adorn* adorn, const Aya::TextureId& id);
    virtual void renderGameCursor(Adorn* adorn);
};
} // namespace Aya
