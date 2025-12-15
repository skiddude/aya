#pragma once

#include "intrusive_ptr_target.hpp"
#include "DataModel/GlobalSettings.hpp"
#include "DataModel/GameSettings.hpp"
#include "DataModel/RenderHooksService.hpp"

namespace Aya
{
extern const char* const sGameBasicSettings;
class GameBasicSettings : public GlobalBasicSettingsItem<GameBasicSettings, sGameBasicSettings>
{
    typedef GlobalBasicSettingsItem<GameBasicSettings, sGameBasicSettings> Super;

public:
    enum ControlMode
    {
        CONTROL_CLASSIC = 0,
        CONTROL_MOUSELOCK = 1,
        CONTROL_HYBRID = 2,
        CONTROL_CAMLOCK = 3,
        CONTROL_MOUSEPAN = 4
    };
    enum RenderQualitySetting
    {
        QUALITY_AUTO = 0,
        QUALITY_1 = 1,
        QUALITY_2 = 2,
        QUALITY_3 = 3,
        QUALITY_4 = 4,
        QUALITY_5 = 5,
        QUALITY_6 = 6,
        QUALITY_7 = 7,
        QUALITY_8 = 8,
        QUALITY_9 = 9,
        QUALITY_10 = 10
    };
    enum CameraMode
    {
        CAMERA_MODE_DEFAULT = 0,
        CAMERA_MODE_CLASSIC = 1,
        CAMERA_MODE_FOLLOW = 2
    };
    enum TouchCameraMovementMode
    {
        TOUCH_CAMERA_MOVEMENT_MODE_DEFAULT = 0,
        TOUCH_CAMERA_MOVEMENT_MODE_CLASSIC = 1,
        TOUCH_CAMERA_MOVEMENT_MODE_FOLLOW = 2
    };
    enum ComputerCameraMovementMode
    {
        COMPUTER_CAMERA_MOVEMENT_MODE_DEFAULT = 0,
        COMPUTER_CAMERA_MOVEMENT_MODE_CLASSIC = 1,
        COMPUTER_CAMERA_MOVEMENT_MODE_FOLLOW = 2
    };
    enum TouchMovementMode
    {
        TOUCH_MOVEMENT_MODE_DEFAULT = 0,
        TOUCH_MOVEMENT_MODE_THUMBSTICK = 1,
        TOUCH_MOVEMENT_MODE_DPAD = 2,
        TOUCH_MOVEMENT_MODE_THUMBPAD = 3,
        TOUCH_MOVEMENT_MODE_CLICK_TO_MOVE = 4
    };
    enum ComputerMovementMode
    {
        COMPUTER_MOVEMENT_MODE_DEFAULT = 0,
        COMPUTER_MOVEMENT_MODE_KBD_MOUSE = 1,
        COMPUTER_MOVEMENT_MODE_CLICK_TO_MOVE = 2
    };
    enum RotationType
    {
        ROTATION_TYPE_MOVEMENT_RELATIVE = 0,
        ROTATION_TYPE_CAMERA_RELATIVE = 1
    };
    enum VirtualVersion
    {
        VERSION_2016 = 0,
        VERSION_2015 = 1,
        VERSION_2014 = 2,
        VERSION_2013 = 3,
        VERSION_2012 = 4
    };
    enum MaxFramerate
    {
        FPS_30 = 0,
        FPS_60 = 1,
        FPS_75 = 2,
        FPS_120 = 3,
        FPS_144 = 4,
        FPS_200 = 5,
        FPS_240 = 6,
        FPS_360 = 7,
        FPS_UNCAPPED = 8
    };

    static Reflection::PropDescriptor<GameBasicSettings, float> prop_masterVolume;

    GameBasicSettings();

    ControlMode getControlMode() const
    {
        return controlMode;
    }
    void setControlMode(ControlMode setting);

    CameraMode getCameraMode() const
    {
        return cameraMode;
    }
    GameBasicSettings::CameraMode getCameraModeWithDefault() const;
    void setCameraMode(CameraMode setting);

    TouchCameraMovementMode getTouchCameraMovementMode() const
    {
        return touchCameraMovementMode;
    }
    void setTouchCameraMovementMode(TouchCameraMovementMode setting);
    bool getTouchCameraMovementModeModified() const
    {
        return touchCameraMovementModeModified;
    }
    void setTouchCameraMovementModeModified(bool setting);

    ComputerCameraMovementMode getComputerCameraMovementMode() const
    {
        return computerCameraMovementMode;
    }
    void setComputerCameraMovementMode(ComputerCameraMovementMode setting);
    bool getComputerCameraMovementModeModified() const
    {
        return computerCameraMovementModeModified;
    }
    void setComputerCameraMovementModeModified(bool setting);

    TouchMovementMode getTouchMovementMode() const
    {
        return touchMoveMode;
    }
    void setTouchMovementMode(TouchMovementMode setting);
    bool getTouchMovementModeModified() const
    {
        return touchMoveModeModeModified;
    }
    void setTouchMovementModeModified(bool setting);

    ComputerMovementMode getComputerMovementMode() const
    {
        return computerMoveMode;
    }
    void setComputerMovementMode(ComputerMovementMode setting);
    bool getComputerMovementModeModified() const
    {
        return computerMoveModeModeModified;
    }
    void setComputerMovementModeModified(bool setting);

    RotationType getRotationType() const;
    void setRotationType(RotationType setting);

    bool getDiscordRichPresenceEnabled() const
    {
        return discordRichPresenceEnabled;
    }
    void setDiscordRichPresenceEnabled(bool value);

    bool getMicroProfilerEnabled() const
    {
        return microProfilerEnabled;
    }
    void setMicroProfilerEnabled(bool value);

    bool getWireframeRendering() const
    {
        return wireframeRenderingEnabled;
    }
    void setWireframeRendering(bool value)
    {
        wireframeRenderingEnabled = value;
    }

    bool getFreakyModeEnabled() const
    {
        return freakyModeEnabled;
    }
    void setFreakyModeEnabled(bool value);

    VirtualVersion getVirtualVersion() const
    {
        return virtualVersion;
    }
    void setVirtualVersion(VirtualVersion value);

    MaxFramerate getMaxFramerate() const
    {
        return maxFramerate;
    }
    void setMaxFramerate(MaxFramerate value);
    void setVirtualVersionInternal(VirtualVersion value); // one w/o the check, sorry

    Aya::signal<void(VirtualVersion)> virtualVersionChangedSignal;
    Aya::signal<void(MaxFramerate)> maxFramerateChangedSignal;
    Aya::signal<void(bool)> toggleClassicRenderingSignal;
    void toggleClassicRendering(bool on)
    {
        toggleClassicRenderingSignal(on);
    }

    void setMouseLock(bool isLocked);
    bool isMouseLocked() const
    {
        return mouseLocked;
    }

    void setCanMousePan(bool canPan)
    {
        canMousePan = canPan;
    }
    bool getCanMousePan()
    {
        return canMousePan;
    }

    void setFreeLook(bool canLook)
    {
        freeLook = canLook;
    }
    bool getFreeLook()
    {
        return freeLook;
    }

    bool inClassicMode()
    {
        return controlMode == CONTROL_CLASSIC;
    }
    bool inMouseLockMode()
    {
        return controlMode == CONTROL_MOUSELOCK;
    }
    bool inHybridMode()
    {
        return controlMode == CONTROL_HYBRID;
    }
    bool inCamlockMode()
    {
        return controlMode == CONTROL_CAMLOCK;
    }
    bool inMousepanMode()
    {
        return controlMode == CONTROL_MOUSEPAN;
    }

    bool mouseLockedInMouseLockMode()
    {
        return inMouseLockMode() && isMouseLocked();
    }
    bool camLockedInCamLockMode()
    {
        return inCamlockMode() && !getFreeLook();
    }

    bool getTutorialState(std::string tutorialId);
    void setTutorialState(std::string tutorialId, bool value);

    std::string getCompletedTutorials() const;
    void setCompletedTutorials(std::string value);

    RenderQualitySetting getRenderQuality() const
    {
        return renderQualitySetting;
    }
    void setRenderQuality(RenderQualitySetting value);
    void setRenderQualityInternal(RenderQualitySetting value);

    bool getAllTutorialsDisabled() const
    {
        return allTutorialsDisabled;
    }
    void setAllTutorialsDisabled(bool value);

    bool getFullScreenConst() const
    {
        return fullscreen;
    }
    bool getFullScreen()
    {
        return fullscreen;
    }
    void setFullScreen(bool value)
    {
        if (value != fullscreen)
        {
            fullscreen = value;
            fullscreenChangedSignal(value);
        }
    }

    Vector2 getStartScreenPos() const
    {
        return startScreenPos;
    }
    void setStartScreenPos(Vector2 value);

    Vector2 getStartScreenSize() const
    {
        return startScreenSize;
    }
    void setStartScreenSize(Vector2 value);

    bool getStartMaximized() const
    {
        return startMaximized;
    }
    void setStartMaximized(bool value);

    float getMasterVolume() const
    {
        return masterVolume;
    }
    void setMasterVolume(float value);

    float getMouseSensitivity() const;
    void setMouseSensitivity(float value);

    bool inStudioMode()
    {
        return studio;
    }
    void setStudioMode(bool value)
    {
        if (value != studio)
        {
            studioModeChangedSignal(value);
            studio = value;
        }
    }

    bool getUsedHideHudShortcut() const
    {
        return usedHideHudShortcut;
    }
    void setUsedHideHudShortcut(bool value)
    {
        usedHideHudShortcut = value;
    }

    /*override*/ void reset();
    /*override*/ void verifySetParent(const Instance* instance) const;

    Aya::signal<void(bool)> fullscreenChangedSignal;
    Aya::signal<void(bool)> studioModeChangedSignal;

private:
    ControlMode controlMode;
    RenderQualitySetting renderQualitySetting;
    CameraMode cameraMode;
    TouchCameraMovementMode touchCameraMovementMode;
    bool touchCameraMovementModeModified;
    ComputerCameraMovementMode computerCameraMovementMode;
    bool computerCameraMovementModeModified;
    TouchMovementMode touchMoveMode;
    bool touchMoveModeModeModified;
    ComputerMovementMode computerMoveMode;
    bool computerMoveModeModeModified;
    RotationType rotationType;
    VirtualVersion virtualVersion;
    MaxFramerate maxFramerate;
    bool freakyModeEnabled;

    bool mouseLocked;
    bool canMousePan;
    bool freeLook;
    bool usedHideHudShortcut;

    bool fullscreen;
    bool discordRichPresenceEnabled;
    bool microProfilerEnabled;
    bool wireframeRenderingEnabled = false;
    bool studio;

    Vector2 startScreenPos;
    Vector2 startScreenSize;

    bool startMaximized;

    float masterVolume;
    float mouseSensitivity;

    std::map<std::string, bool> tutorialState;
    bool allTutorialsDisabled;
};

} // namespace Aya
