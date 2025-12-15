

#include "Tree/Instance.hpp"
#include "Tree/Service.hpp"
#include "DataModel/StarterPlayerService.hpp"
#include "DataModel/PlayerScripts.hpp"
#include "DataModel/GameBasicSettings.hpp"
#include "DataModel/Folder.hpp"
#include "Humanoid/Humanoid.hpp"
#include "Player.hpp"
#include "Players.hpp"

namespace Aya
{

const char* const sStarterPlayerService = "StarterPlayer";

static Reflection::EnumPropDescriptor<StarterPlayerService, GameBasicSettings::VirtualVersion> prop_minVirtualVersion(
    "MinVirtualVersion", "Virtual Version", &StarterPlayerService::getMinVirtualVersion, &StarterPlayerService::setMinVirtualVersion);
static Reflection::EnumPropDescriptor<StarterPlayerService, GameBasicSettings::VirtualVersion> prop_maxVirtualVersion(
    "MaxVirtualVersion", "Virtual Version", &StarterPlayerService::getMaxVirtualVersion, &StarterPlayerService::setMaxVirtualVersion);

static Reflection::EnumPropDescriptor<StarterPlayerService, StarterPlayerService::DeveloperTouchCameraMovementMode> prop_devTouchCameraMovementMode(
    "DevTouchCameraMovementMode", "Camera", &StarterPlayerService::getDevTouchCameraMovementMode,
    &StarterPlayerService::setDevTouchCameraMovementMode);
static Reflection::EnumPropDescriptor<StarterPlayerService, StarterPlayerService::DeveloperComputerCameraMovementMode>
    prop_devComputerCameraMovementMode("DevComputerCameraMovementMode", "Camera", &StarterPlayerService::getDevComputerCameraMovementMode,
        &StarterPlayerService::setDevComputerCameraMovementMode);
static Reflection::EnumPropDescriptor<StarterPlayerService, StarterPlayerService::DeveloperCameraOcclusionMode> prop_devCameraOcclusionMode(
    "DevCameraOcclusionMode", "Camera", &StarterPlayerService::getDevCameraOcclusionMode, &StarterPlayerService::setDevCameraOcclusionMode);
static Reflection::EnumPropDescriptor<StarterPlayerService, StarterPlayerService::DeveloperTouchMovementMode> prop_devTouchMovementMode(
    "DevTouchMovementMode", "Controls", &StarterPlayerService::getDevTouchMovementMode, &StarterPlayerService::setDevTouchMovementMode);
static Reflection::EnumPropDescriptor<StarterPlayerService, StarterPlayerService::DeveloperComputerMovementMode> prop_devComputerMovementMode(
    "DevComputerMovementMode", "Controls", &StarterPlayerService::getDevComputerMovementMode, &StarterPlayerService::setDevComputerMovementMode);
static Reflection::PropDescriptor<StarterPlayerService, bool> prop_enableMouseLockOption(
    "EnableMouseLockOption", "Controls", &StarterPlayerService::getEnableMouseLockOption, &StarterPlayerService::setEnableMouseLockOption);
static Reflection::PropDescriptor<StarterPlayerService, bool> prop_autoJumpEnabled(
    "AutoJumpEnabled", "Mobile", &StarterPlayerService::getAutoJumpEnabled, &StarterPlayerService::setAutoJumpEnabled);
static Reflection::PropDescriptor<StarterPlayerService, bool> prop_loadCharacterAppearance(
    "LoadCharacterAppearance", "Character", &StarterPlayerService::getLoadCharacterAppearance, &StarterPlayerService::setLoadCharacterAppearance);

static Reflection::EnumPropDescriptor<StarterPlayerService, Camera::CameraMode> prop_cameraMode(
    "CameraMode", "Camera", &StarterPlayerService::getCameraMode, &StarterPlayerService::setCameraMode);
static Reflection::PropDescriptor<StarterPlayerService, float> prop_cameraMaxZoomDistance(
    "CameraMaxZoomDistance", "Camera", &StarterPlayerService::getCameraMaxZoomDistance, &StarterPlayerService::setCameraMaxZoomDistance);
static Reflection::PropDescriptor<StarterPlayerService, float> prop_cameraMinZoomDistance(
    "CameraMinZoomDistance", "Camera", &StarterPlayerService::getCameraMinZoomDistance, &StarterPlayerService::setCameraMinZoomDistance);

static Reflection::PropDescriptor<StarterPlayerService, float> prop_nameDisplayDistance(
    "NameDisplayDistance", category_Data, &StarterPlayerService::getNameDisplayDistance, &StarterPlayerService::setNameDisplayDistance);
static Reflection::PropDescriptor<StarterPlayerService, float> prop_healthDisplayDistance(
    "HealthDisplayDistance", category_Data, &StarterPlayerService::getHealthDisplayDistance, &StarterPlayerService::setHealthDisplayDistance);
REFLECTION_END();

namespace Reflection
{
template<>
EnumDesc<StarterPlayerService::DeveloperTouchCameraMovementMode>::EnumDesc()
    : EnumDescriptor("DevTouchCameraMovementMode")
{
    addPair(StarterPlayerService::DEV_TOUCH_CAMERA_MOVEMENT_MODE_USER, "UserChoice");
    addPair(StarterPlayerService::DEV_TOUCH_CAMERA_MOVEMENT_MODE_CLASSIC, "Classic");
    addPair(StarterPlayerService::DEV_TOUCH_CAMERA_MOVEMENT_MODE_FOLLOW, "Follow");
}

template<>
EnumDesc<StarterPlayerService::DeveloperComputerCameraMovementMode>::EnumDesc()
    : EnumDescriptor("DevComputerCameraMovementMode")
{
    addPair(StarterPlayerService::DEV_COMPUTER_CAMERA_MOVEMENT_MODE_USER, "UserChoice");
    addPair(StarterPlayerService::DEV_COMPUTER_CAMERA_MOVEMENT_MODE_CLASSIC, "Classic");
    addPair(StarterPlayerService::DEV_COMPUTER_CAMERA_MOVEMENT_MODE_FOLLOW, "Follow");
}

template<>
EnumDesc<StarterPlayerService::DeveloperCameraOcclusionMode>::EnumDesc()
    : EnumDescriptor("DevCameraOcclusionMode")
{
    addPair(StarterPlayerService::DEV_CAMERA_OCCLUSION_MODE_ZOOM, "Zoom");
    addPair(StarterPlayerService::DEV_CAMERA_OCCLUSION_MODE_INVISI, "Invisicam");
}

template<>
EnumDesc<StarterPlayerService::DeveloperTouchMovementMode>::EnumDesc()
    : EnumDescriptor("DevTouchMovementMode")
{
    addPair(StarterPlayerService::DEV_TOUCH_MOVEMENT_MODE_USER, "UserChoice");
    addPair(StarterPlayerService::DEV_TOUCH_MOVEMENT_MODE_THUMBSTICK, "Thumbstick");
    addPair(StarterPlayerService::DEV_TOUCH_MOVEMENT_MODE_DPAD, "DPad");
    addPair(StarterPlayerService::DEV_TOUCH_MOVEMENT_MODE_THUMBPAD, "Thumbpad");
    addPair(StarterPlayerService::DEV_TOUCH_MOVEMENT_MODE_CLICK_TO_MOVE, "ClickToMove");
    addPair(StarterPlayerService::DEV_TOUCH_MOVEMENT_MODE_SCRIPTABLE, "Scriptable");
}

template<>
EnumDesc<StarterPlayerService::DeveloperComputerMovementMode>::EnumDesc()
    : EnumDescriptor("DevComputerMovementMode")
{
    addPair(StarterPlayerService::DEV_COMPUTER_MOVEMENT_MODE_USER, "UserChoice");
    addPair(StarterPlayerService::DEV_COMPUTER_MOVEMENT_MODE_KBD_MOUSE, "KeyboardMouse");
    addPair(StarterPlayerService::DEV_COMPUTER_MOVEMENT_MODE_CLICK_TO_MOVE, "ClickToMove");
    addPair(StarterPlayerService::DEV_COMPUTER_MOVEMENT_MODE_SCRIPTABLE, "Scriptable");
}


// TODO: Add rest of enums here and in factoryregistration.cpp
} // namespace Reflection


static float defaultDisplayDistance(100.0f);

StarterPlayerService::StarterPlayerService()
    : cameraMode(Aya::Camera::CAMERAMODE_CLASSIC)
    , minVirtualVersion(Aya::GameBasicSettings::VERSION_2012)
    , maxVirtualVersion(Aya::GameBasicSettings::VERSION_2016)
    , nameDisplayDistance(defaultDisplayDistance)
    , healthDisplayDistance(defaultDisplayDistance)
    , cameraMaxZoomDistance(Aya::Camera::distanceMaxCharacter())
    , cameraMinZoomDistance(Aya::Camera::distanceMin())
    , enableMouseLockOption(true)
    , touchCameraMovementMode(StarterPlayerService::DEV_TOUCH_CAMERA_MOVEMENT_MODE_USER)
    , computerCameraMovementMode(StarterPlayerService::DEV_COMPUTER_CAMERA_MOVEMENT_MODE_USER)
    , cameraOcclusionMode(StarterPlayerService::DEV_CAMERA_OCCLUSION_MODE_ZOOM)
    , touchMovementMode(StarterPlayerService::DEV_TOUCH_MOVEMENT_MODE_USER)
    , computerMovementMode(StarterPlayerService::DEV_COMPUTER_MOVEMENT_MODE_USER)
    , autoJumpEnabled(true)
    , loadCharacterAppearance(true)
{
    Instance::setName(sStarterPlayerService);
}

bool StarterPlayerService::askForbidChild(const Instance* instance) const
{
    if (Instance::fastDynamicCast<StarterPlayerScripts>(instance) != NULL)
        return false;

    bool isHumanoid = Instance::fastDynamicCast<Humanoid>(instance) != NULL;
    if (isHumanoid)
        return false;

    bool isCharacterScripts = (Instance::fastDynamicCast<StarterCharacterScripts>(instance) != NULL);
    if (isCharacterScripts)
        return false;

    bool isModel = Instance::fastDynamicCast<ModelInstance>(instance) != NULL;
    if (isModel)
        return false;

    return true;
}

bool StarterPlayerService::askAddChild(const Instance* instance) const
{
    if (Instance::fastDynamicCast<StarterPlayerScripts>(instance) != NULL)
        return true;

    bool isHumanoid = Instance::fastDynamicCast<Humanoid>(instance) != NULL;
    if (isHumanoid)
        return true;

    bool isCharacterScripts = (Instance::fastDynamicCast<StarterCharacterScripts>(instance) != NULL);
    if (isCharacterScripts)
        return true;

    bool isModel = Instance::fastDynamicCast<ModelInstance>(instance) != NULL;
    if (isModel)
        return true;

    return false;
}

void StarterPlayerService::setMinVirtualVersion(Aya::GameBasicSettings::VirtualVersion value)
{
    if (minVirtualVersion != value)
    {
        if (value > maxVirtualVersion)
            throw std::runtime_error("MinVirtualVersion cannot be greater than MaxVirtualVersion");

        minVirtualVersion = value;
        raisePropertyChanged(prop_minVirtualVersion);
    }
}

void StarterPlayerService::setMaxVirtualVersion(Aya::GameBasicSettings::VirtualVersion value)
{
    if (maxVirtualVersion != value)
    {
        if (value < minVirtualVersion)
            throw std::runtime_error("MaxVirtualVersion cannot be less than MinVirtualVersion");

        maxVirtualVersion = value;
        raisePropertyChanged(prop_maxVirtualVersion);
    }
}

void StarterPlayerService::setCameraMode(Aya::Camera::CameraMode value)
{
    if (cameraMode != value)
    {
        cameraMode = value;
        raisePropertyChanged(prop_cameraMode);
    }
}

void StarterPlayerService::setEnableMouseLockOption(bool setting)
{
    if (enableMouseLockOption != setting)
    {
        enableMouseLockOption = setting;
        raisePropertyChanged(prop_enableMouseLockOption);
    }
}

void StarterPlayerService::setAutoJumpEnabled(bool value)
{
    if (autoJumpEnabled != value)
    {
        autoJumpEnabled = value;
        raisePropertyChanged(prop_autoJumpEnabled);
    }
}

void StarterPlayerService::setLoadCharacterAppearance(bool value)
{
    if (loadCharacterAppearance != value)
    {
        loadCharacterAppearance = value;
        raisePropertyChanged(prop_loadCharacterAppearance);
    }
}

void StarterPlayerService::setDevTouchCameraMovementMode(DeveloperTouchCameraMovementMode setting)
{
    if (touchCameraMovementMode != setting)
    {
        touchCameraMovementMode = setting;
        raisePropertyChanged(prop_devTouchCameraMovementMode);
    }
}

void StarterPlayerService::setDevComputerCameraMovementMode(DeveloperComputerCameraMovementMode setting)
{
    if (computerCameraMovementMode != setting)
    {
        computerCameraMovementMode = setting;
        raisePropertyChanged(prop_devComputerCameraMovementMode);
    }
}

void StarterPlayerService::setDevCameraOcclusionMode(DeveloperCameraOcclusionMode setting)
{
    if (cameraOcclusionMode != setting)
    {
        cameraOcclusionMode = setting;
        raisePropertyChanged(prop_devCameraOcclusionMode);
    }
}

void StarterPlayerService::setDevTouchMovementMode(DeveloperTouchMovementMode setting)
{
    if (touchMovementMode != setting)
    {
        touchMovementMode = setting;
        raisePropertyChanged(prop_devTouchMovementMode);
    }
}

void StarterPlayerService::setDevComputerMovementMode(DeveloperComputerMovementMode setting)
{
    if (computerMovementMode != setting)
    {
        computerMovementMode = setting;
        raisePropertyChanged(prop_devComputerMovementMode);
    }
}



void StarterPlayerService::setNameDisplayDistance(float value)
{
    float legalValue = G3D::max(value, 0.0f);
    if (nameDisplayDistance != legalValue)
    {
        nameDisplayDistance = legalValue;
        raisePropertyChanged(prop_nameDisplayDistance);
    }
}

void StarterPlayerService::setHealthDisplayDistance(float value)
{
    float legalValue = G3D::max(value, 0.0f);
    if (healthDisplayDistance != legalValue)
    {
        healthDisplayDistance = legalValue;
        raisePropertyChanged(prop_healthDisplayDistance);
    }
}

void StarterPlayerService::setCameraMaxZoomDistance(float value)
{
    value = G3D::clamp(value, cameraMinZoomDistance, Camera::distanceMaxCharacter());
    if (cameraMaxZoomDistance != value)
    {
        cameraMaxZoomDistance = value;
        raisePropertyChanged(prop_cameraMaxZoomDistance);
    }
}

void StarterPlayerService::setCameraMinZoomDistance(float value)
{
    value = G3D::clamp(value, Camera::distanceMin(), cameraMaxZoomDistance);
    if (cameraMinZoomDistance != value)
    {
        cameraMinZoomDistance = value;
        raisePropertyChanged(prop_cameraMinZoomDistance);
    }
}

void StarterPlayerService::setupPlayer(shared_ptr<Instance> instance)
{
    if (Network::Player* player = Instance::fastDynamicCast<Network::Player>(instance.get()))
    {
        player->setCameraMode(cameraMode);
        player->setCameraMaxZoomDistance(cameraMaxZoomDistance);
        player->setCameraMinZoomDistance(cameraMinZoomDistance);
        player->setHealthDisplayDistance(healthDisplayDistance);
        player->setNameDisplayDistance(nameDisplayDistance);
        player->setAutoJumpEnabled(autoJumpEnabled);
    }
}

void StarterPlayerService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
    Super::onServiceProvider(oldProvider, newProvider);

    if (newProvider && Aya::Network::Players::backendProcessing(newProvider))
    {
        Network::Players* players = ServiceProvider::find<Network::Players>(newProvider);
        if (players)
            players->visitDescendants(boost::bind(&StarterPlayerService::setupPlayer, this, _1));

        if (DataModel* dataModel = DataModel::get(newProvider))
            workspaceLoadedConnection = dataModel->workspaceLoadedSignal.connect(boost::bind(&StarterPlayerService::setupPlayerScripts, this));
    }
}



void StarterPlayerService::setupPlayerScripts()
{
    Aya::StarterPlayerScripts* currPlayerScripts = findFirstChildOfType<Aya::StarterPlayerScripts>();
    if (!currPlayerScripts)
    {
        shared_ptr<Aya::StarterPlayerScripts> playerScripts = Creatable<Instance>::create<Aya::StarterPlayerScripts>();
        currPlayerScripts = playerScripts.get();
        if (currPlayerScripts)
        {
            playerScripts->setParent(this);
        }
    }

    Aya::StarterPlayerScripts* pStarterCharacterScripts = findFirstChildOfType<Aya::StarterCharacterScripts>();
    if (!pStarterCharacterScripts)
    {
        shared_ptr<Aya::StarterCharacterScripts> characterScripts = Creatable<Instance>::create<Aya::StarterCharacterScripts>();
        currPlayerScripts = characterScripts.get();
        if (currPlayerScripts)
        {
            currPlayerScripts->setParent(this);
        }
    }
}

void StarterPlayerService::recordSettingsInGA() const
{
    const char* CameraMovement = NULL;
    const char* CameraOcclusion = NULL;
    const char* CharacterMovement = NULL;

    switch (touchCameraMovementMode)
    {
    case DEV_TOUCH_CAMERA_MOVEMENT_MODE_CLASSIC:
        CameraMovement = "TouchCameraMoveModeClassic";
        break;
    case DEV_TOUCH_CAMERA_MOVEMENT_MODE_FOLLOW:
        CameraMovement = "TouchCameraMoveModeFollow";
        break;
    case DEV_TOUCH_CAMERA_MOVEMENT_MODE_USER:
    default:
        CameraMovement = "TouchCameraMoveModeUser";
        break;
    }

    switch (touchMovementMode)
    {
    case DEV_TOUCH_MOVEMENT_MODE_THUMBSTICK:
        CharacterMovement = "TouchMovementModeThumbStick";
        break;
    case DEV_TOUCH_MOVEMENT_MODE_DPAD:
        CharacterMovement = "TouchMovementModeDPad";
        break;
    case DEV_TOUCH_MOVEMENT_MODE_THUMBPAD:
        CharacterMovement = "TouchMovementModeThumbpad";
        break;
    case DEV_TOUCH_MOVEMENT_MODE_CLICK_TO_MOVE:
        CharacterMovement = "TouchMovementModeClickToMove";
        break;
    case DEV_TOUCH_MOVEMENT_MODE_SCRIPTABLE:
        CharacterMovement = "TouchMovementModeScriptable";
        break;
    case DEV_TOUCH_MOVEMENT_MODE_USER:
    default:
        CharacterMovement = "TouchMovementModeUser";
        break;
    }

    switch (computerCameraMovementMode)
    {
    case DEV_COMPUTER_CAMERA_MOVEMENT_MODE_CLASSIC:
        CameraMovement = "ComputerCameraMoveModeClassic";
        break;
    case DEV_COMPUTER_CAMERA_MOVEMENT_MODE_FOLLOW:
        CameraMovement = "ComputerCameraMoveModeFollow";
        break;
    case DEV_COMPUTER_CAMERA_MOVEMENT_MODE_USER:
    default:
        CameraMovement = "ComputerCameraMoveModeUser";
        break;
    }

    switch (computerMovementMode)
    {
    case DEV_COMPUTER_MOVEMENT_MODE_KBD_MOUSE:
        CharacterMovement = "ComputerMovementModeKbdMouse";
        break;
    case DEV_COMPUTER_MOVEMENT_MODE_CLICK_TO_MOVE:
        CharacterMovement = "ComputerMovementModeClickToMove";
        break;
    case DEV_COMPUTER_MOVEMENT_MODE_SCRIPTABLE:
        CharacterMovement = "ComputerMovementModeScriptable";
        break;
    case DEV_COMPUTER_MOVEMENT_MODE_USER:
    default:
        CharacterMovement = "ComputerMovementModeUser";
        break;
    }

    switch (cameraOcclusionMode)
    {
    case DEV_CAMERA_OCCLUSION_MODE_INVISI:
        CameraOcclusion = "DevCameraOcclusionInvisi";
        break;
    case DEV_CAMERA_OCCLUSION_MODE_ZOOM:
    default:
        CameraOcclusion = "DevCameraOcclusionZoom";
        break;
    }
}




} // namespace Aya
