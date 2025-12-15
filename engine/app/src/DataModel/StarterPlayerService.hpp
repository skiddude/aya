#pragma once
#include "DataModel/GameBasicSettings.hpp"
#include "DataModel/Camera.hpp"
#include "Tree/Service.hpp"
namespace Aya
{

namespace Network
{
class Player;
}

//////////////////////////////////////////////////////////////////////////////

extern const char* const sStarterPlayerService;
class StarterPlayerService
    : public DescribedCreatable<StarterPlayerService, Instance, sStarterPlayerService, Reflection::ClassDescriptor::PERSISTENT_LOCAL>
    , public Service
{
public:
    enum DeveloperTouchCameraMovementMode
    {
        DEV_TOUCH_CAMERA_MOVEMENT_MODE_USER = 0,
        DEV_TOUCH_CAMERA_MOVEMENT_MODE_CLASSIC = 1,
        DEV_TOUCH_CAMERA_MOVEMENT_MODE_FOLLOW = 2
    };
    enum DeveloperComputerCameraMovementMode
    {
        DEV_COMPUTER_CAMERA_MOVEMENT_MODE_USER = 0,
        DEV_COMPUTER_CAMERA_MOVEMENT_MODE_CLASSIC = 1,
        DEV_COMPUTER_CAMERA_MOVEMENT_MODE_FOLLOW = 2
    };
    enum DeveloperCameraOcclusionMode
    {
        DEV_CAMERA_OCCLUSION_MODE_ZOOM = 0,
        DEV_CAMERA_OCCLUSION_MODE_INVISI = 1
    };
    enum DeveloperTouchMovementMode
    {
        DEV_TOUCH_MOVEMENT_MODE_USER = 0,
        DEV_TOUCH_MOVEMENT_MODE_THUMBSTICK = 1,
        DEV_TOUCH_MOVEMENT_MODE_DPAD = 2,
        DEV_TOUCH_MOVEMENT_MODE_THUMBPAD = 3,
        DEV_TOUCH_MOVEMENT_MODE_CLICK_TO_MOVE = 4,
        DEV_TOUCH_MOVEMENT_MODE_SCRIPTABLE = 5
    };
    enum DeveloperComputerMovementMode
    {
        DEV_COMPUTER_MOVEMENT_MODE_USER = 0,
        DEV_COMPUTER_MOVEMENT_MODE_KBD_MOUSE = 1,
        DEV_COMPUTER_MOVEMENT_MODE_CLICK_TO_MOVE = 2,
        DEV_COMPUTER_MOVEMENT_MODE_SCRIPTABLE = 3
    };

private:
    Aya::Camera::CameraMode cameraMode;

    float nameDisplayDistance;
    float healthDisplayDistance;
    float cameraMaxZoomDistance;
    float cameraMinZoomDistance;

    bool enableMouseLockOption;
    DeveloperTouchCameraMovementMode touchCameraMovementMode;
    DeveloperComputerCameraMovementMode computerCameraMovementMode;
    DeveloperCameraOcclusionMode cameraOcclusionMode;
    DeveloperTouchMovementMode touchMovementMode;
    DeveloperComputerMovementMode computerMovementMode;

    Aya::GameBasicSettings::VirtualVersion minVirtualVersion;
    Aya::GameBasicSettings::VirtualVersion maxVirtualVersion;

    void setupPlayer(shared_ptr<Instance> instance);

    Aya::signals::scoped_connection workspaceLoadedConnection;

    bool autoJumpEnabled;
    bool loadCharacterAppearance;

protected:
    typedef DescribedCreatable<StarterPlayerService, Instance, sStarterPlayerService, Reflection::ClassDescriptor::PERSISTENT_LOCAL> Super;

    ///////////////////////////////////////////////////////////////////////////
    // Instance
    /*override*/ bool askForbidChild(const Instance* instance) const;
    /*override*/ bool askAddChild(const Instance* instance) const;
    /*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

public:
    StarterPlayerService();
    void setupPlayerScripts();

    void recordSettingsInGA() const;

    void setMinVirtualVersion(Aya::GameBasicSettings::VirtualVersion value);
    Aya::GameBasicSettings::VirtualVersion getMinVirtualVersion() const
    {
        return minVirtualVersion;
    }

    void setMaxVirtualVersion(Aya::GameBasicSettings::VirtualVersion value);
    Aya::GameBasicSettings::VirtualVersion getMaxVirtualVersion() const
    {
        return maxVirtualVersion;
    }

    void setCameraMode(Aya::Camera::CameraMode value);
    Aya::Camera::CameraMode getCameraMode() const
    {
        return cameraMode;
    }

    bool getEnableMouseLockOption() const
    {
        return enableMouseLockOption;
    }
    void setEnableMouseLockOption(bool setting);

    DeveloperTouchCameraMovementMode getDevTouchCameraMovementMode() const
    {
        return touchCameraMovementMode;
    }
    void setDevTouchCameraMovementMode(DeveloperTouchCameraMovementMode setting);

    DeveloperComputerCameraMovementMode getDevComputerCameraMovementMode() const
    {
        return computerCameraMovementMode;
    }
    void setDevComputerCameraMovementMode(DeveloperComputerCameraMovementMode setting);

    DeveloperCameraOcclusionMode getDevCameraOcclusionMode() const
    {
        return cameraOcclusionMode;
    }
    void setDevCameraOcclusionMode(DeveloperCameraOcclusionMode setting);

    DeveloperTouchMovementMode getDevTouchMovementMode() const
    {
        return touchMovementMode;
    }
    void setDevTouchMovementMode(DeveloperTouchMovementMode setting);

    DeveloperComputerMovementMode getDevComputerMovementMode() const
    {
        return computerMovementMode;
    }
    void setDevComputerMovementMode(DeveloperComputerMovementMode setting);

    void setNameDisplayDistance(float value);
    float getNameDisplayDistance() const
    {
        return nameDisplayDistance;
    }

    void setHealthDisplayDistance(float value);
    float getHealthDisplayDistance() const
    {
        return healthDisplayDistance;
    }

    void setCameraMaxZoomDistance(float _cameraMaxZoomDistance);
    float getCameraMaxZoomDistance() const
    {
        return cameraMaxZoomDistance;
    }

    void setCameraMinZoomDistance(float _cameraMinZoomDistance);
    float getCameraMinZoomDistance() const
    {
        return cameraMinZoomDistance;
    }

    void setAutoJumpEnabled(bool value);
    bool getAutoJumpEnabled() const
    {
        return autoJumpEnabled;
    }

    void setLoadCharacterAppearance(bool value);
    bool getLoadCharacterAppearance() const
    {
        return loadCharacterAppearance;
    }
};

} // namespace Aya