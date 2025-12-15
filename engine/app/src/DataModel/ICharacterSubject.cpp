


#include "DataModel/ICharacterSubject.hpp"

#include "boost/foreach.hpp"

#include "DataModel/Camera.hpp"
#include "DataModel/Workspace.hpp"
#include "DataModel/Filters.hpp"
#include "DataModel/GameBasicSettings.hpp"
#include "DataModel/PlayerGui.hpp"
#include "DataModel/TextButton.hpp"
#include "DataModel/ScreenGui.hpp"
#include "DataModel/UserInputService.hpp"
#include "World/ContactManager.hpp"
#include "World/World.hpp"
#include "Utility/UserInputBase.hpp"

#include "Humanoid/Humanoid.hpp"

FASTFLAG(UserAllCamerasInLua)

namespace Aya
{

const float ICharacterSubject::maxMouseLockOffset = 1.5f;

ICharacterSubject::ICharacterSubject()
    : requestedDistance(10.0f)
    , mouseLockOffset(maxMouseLockOffset)
    , cameraTransitioning(false)
    , cameraMode(Aya::Camera::CAMERAMODE_CLASSIC)
    , minDistance(0.0f)
    , maxDistance(Aya::Camera::distanceMaxCharacter())
{
}

int ICharacterSubject::getControlMode() const
{
    return Aya::GameBasicSettings::singleton().getControlMode();
}

int ICharacterSubject::getCustomCameraMode() const
{
    return Aya::GameBasicSettings::singleton().getCameraModeWithDefault();
}

bool ICharacterSubject::isFirstPerson() const
{
    return requestedDistance < firstPersonCutoff();
}

bool ICharacterSubject::isDistanceFirstPerson(float distance) const
{
    return distance < firstPersonCutoff();
}

void ICharacterSubject::stepRotationalVelocity(Vector3& cameraLocation, Vector3& focusLocation)
{
    if (Aya::GameBasicSettings::singleton().getRotationType() == Aya::GameBasicSettings::ROTATION_TYPE_CAMERA_RELATIVE)
    {
        setFirstPersonRotationalVelocity(focusLocation - cameraLocation, true);
    }
}

void ICharacterSubject::onCameraHeartbeat(const Vector3& cameraLocation, const Vector3& focusPoint)
{
    if (FFlag::UserAllCamerasInLua)
    {
        return;
    }
    Aya::GameBasicSettings& settings(Aya::GameBasicSettings::singleton());

    tellCameraNear((focusPoint - cameraLocation).magnitude());

    if (settings.mouseLockedInMouseLockMode())
        tellCursorOver(mouseLockOffset / maxMouseLockOffset);

    // Ugly
    Instance* i = dynamic_cast<Instance*>(this);
    if (Workspace* workspace = ServiceProvider::find<Workspace>(i))
    {
        workspace->requestFirstPersonCamera(settings.mouseLockedInMouseLockMode() || isFirstPerson(), isTransitioning(), getControlMode());
    }

    if ((isFirstPerson() && !cameraTransitioning) || settings.mouseLockedInMouseLockMode() || settings.camLockedInCamLockMode())
    {
        // Rotate guy on camera pan
        setFirstPersonRotationalVelocity(focusPoint - cameraLocation, true);
    }
    else if (!isFirstPerson() || cameraTransitioning)
    {
        // don't rotate guy
        setFirstPersonRotationalVelocity(Vector3::zero(), false);
    }
}

void ICharacterSubject::setCameraMode(Aya::Camera::CameraMode value)
{
    if (value != cameraMode)
        if (value == Aya::Camera::CAMERAMODE_CLASSIC)
            requestedDistance = 11.0f;

    cameraMode = value;
}

void ICharacterSubject::setMinDistance(float value)
{
    if (value != minDistance)
    {
        if (value > maxDistance)
            value = maxDistance;

        if (value < Camera::distanceMin())
            value = Camera::distanceMin();

        minDistance = value;
    }
}

void ICharacterSubject::setMaxDistance(float value)
{
    if (value != maxDistance)
    {
        if (value < minDistance)
            value = minDistance;

        if (value > Camera::distanceMaxCharacter())
            value = Camera::distanceMaxCharacter();

        maxDistance = value;
    }
}

} // namespace Aya
