


#include "Tool/NullTool.hpp"
#include "Humanoid/Humanoid.hpp"
#include "DataModel/Filters.hpp"
#include "DataModel/Workspace.hpp"
#include "DataModel/PartInstance.hpp"
#include "DataModel/UserController.hpp"
#include "Utility/UserInputBase.hpp"
#include "Utility/NavKeys.hpp"
#include "DataModel/ClickDetector.hpp"
#include "DataModel/UserInputService.hpp"
#include "Players.hpp"

FASTFLAGVARIABLE(UseFixedRightMouseClickBehaviour, true)

namespace Aya
{

const char* const sNewNullTool = "NewNullTool";

NewNullTool::NewNullTool(Workspace* workspace)
    : Named<MouseCommand, sNewNullTool>(workspace)
    , cursor("ArrowCursor")
    , hasWaypoint(false)
{
    if (Aya::ServiceProvider::find<UserInputService>(workspace))
    {
        shared_ptr<InputObject> mousePos = Creatable<Instance>::create<InputObject>(
            InputObject::TYPE_MOUSEIDLE, InputObject::INPUT_STATE_CHANGE, Vector3::zero(), Vector3::zero(), DataModel::get(workspace));
        onMouseIdle(mousePos);
    }
}



NewNullTool::~NewNullTool() {}

bool NewNullTool::isInFirstPerson()
{
    // check to see if we're in first person or not
    if (ModelInstance* character = Aya::Network::Players::findLocalCharacter(workspace))
    {
        if (Humanoid* humanoid = Humanoid::modelIsCharacter(character))
        {
            if (PartInstance* head = humanoid->getHeadSlow())
            {
                if (head->getLocalTransparencyModifier() >= 0.99f)
                {
                    // when the character's head is transparency = 1, means he's transparent due to close camera
                    // = in fps
                    return true;
                }
            }
        }
    }
    return false;
}

void NewNullTool::getIndicatedPart(const shared_ptr<InputObject>& inputObject, const bool& raiseClickEvent, PartInstance** instance, bool* clickable,
    Vector3* waypoint, TextureId* cursorIcon, bool isRightClick)
{
    FilterInvisibleNonColliding invisibleNonColliding;
    Aya::Network::Player* localPlayer = Aya::Network::Players::findLocalPlayer(DataModel::get(workspace));
    *instance = getPartByLocalCharacter(workspace, inputObject, &invisibleNonColliding, *waypoint);
    *clickable =
        ClickDetector::isClickable(shared_from(*instance), distanceToCharacter(*waypoint), raiseClickEvent, localPlayer, &cursorIcon, isRightClick);
}

void NewNullTool::onMouseIdle(const shared_ptr<InputObject>& inputObject)
{
    AYAASSERT(inputObject->getUserInputType() == InputObject::TYPE_MOUSEIDLE);

    PartInstance* foundPart;
    bool clickable;
    TextureId cursorIcon;
    getIndicatedPart(inputObject, false /*click event*/, &foundPart, &clickable, &waypoint, &cursorIcon);

    UserInputService* userInputService = Aya::ServiceProvider::find<UserInputService>(workspace);

    if (foundPart && clickable)
    {
        cursor = cursorIcon.toString();
        if (cursor.empty())
        {
            cursor = "DragCursor";
        }
    }
    else if (!MouseCommand::isAdvArrowToolEnabled())
    {
        if (userInputService)
        {
            cursor = userInputService->getDefaultMouseCursor(false, true).toString();
        }
    }
    else
    {
        shared_ptr<Network::Player> player = Network::Players::findAncestorPlayer(foundPart);
        Aya::Network::Player* localPlayer = Aya::Network::Players::findLocalPlayer(DataModel::get(workspace));

        if (localPlayer || player || Aya::Network::Players::serverIsPresent(DataModel::get(workspace)))
        {
            if (userInputService)
            {
                cursor = userInputService->getDefaultMouseCursor(false, true).toString();
            }
        }
        else
        {
            if (userInputService)
            {
                cursor = userInputService->getDefaultMouseCursor(true, true).toString();
            }
        }
    }
}

void NewNullTool::updateClickDetectorHover(const shared_ptr<InputObject>& inputObject)
{
    if (Aya::Network::Player* player = Aya::Network::Players::findLocalPlayer(DataModel::get(workspace)))
    {
        if (PartInstance* newHoverPart = getPartByLocalCharacter(workspace, inputObject))
        {
            if (newHoverPart != lastHoverPart.get()) // we were hovering on another part, tell that part we stopped hovering
            {
                ClickDetector::stopHover(lastHoverPart, player);
                ClickDetector::isHovered(newHoverPart, distanceToCharacter(newHoverPart->getTranslationUi()), true, player);

                lastHoverPart = shared_from(newHoverPart);
            }
        }
        else if (lastHoverPart) // we were hovering, but now we can't find a part, stop hovering
        {
            ClickDetector::stopHover(lastHoverPart, player);
            lastHoverPart = shared_ptr<PartInstance>();
        }
    }
}

// i.e. - mouse move
void NewNullTool::onMouseHover(const shared_ptr<InputObject>& inputObject)
{
    AYAASSERT(inputObject->getUserInputType() == InputObject::TYPE_MOUSEMOVEMENT);

    if (!FFlag::UseFixedRightMouseClickBehaviour)
    {
        if (rightMouseClickPart)
        {
            PartInstance* potentialNewRightMousePart = getPartByLocalCharacter(workspace, inputObject);
            if (potentialNewRightMousePart !=
                rightMouseClickPart.get()) // if we move the right mouse over another object in the middle of click, we reset rightmouse object
                rightMouseClickPart = shared_ptr<PartInstance>();
        }
    }

    updateClickDetectorHover(inputObject);

    PartInstance* foundPart;
    bool clickable;
    TextureId cursorIcon;
    getIndicatedPart(inputObject, false /*click event*/, &foundPart, &clickable, &waypoint, &cursorIcon);

    UserInputService* userInputService = Aya::ServiceProvider::find<UserInputService>(workspace);

    if (foundPart && clickable)
    {
        cursor = cursorIcon.toString();
        if (cursor.empty())
        {
            cursor = "DragCursor";
        }
    }
    else if (!MouseCommand::isAdvArrowToolEnabled())
    {
        if (userInputService)
        {
            cursor = userInputService->getDefaultMouseCursor(false, true).toString();
        }
    }
    else
    {
        Aya::Network::Player* localPlayer = Aya::Network::Players::findLocalPlayer(DataModel::get(workspace));
        shared_ptr<Network::Player> player = Network::Players::findAncestorPlayer(foundPart);

        if (localPlayer || player || Aya::Network::Players::serverIsPresent(DataModel::get(workspace)))
        {
            if (userInputService)
            {
                cursor = userInputService->getDefaultMouseCursor(false, true).toString();
                ;
            }
        }
        else if (userInputService)
        {
            cursor = userInputService->getDefaultMouseCursor(true, true).toString();
            ;
        }
    }

    hasWaypoint = false;
}

shared_ptr<MouseCommand> NewNullTool::onRightMouseDown(const shared_ptr<InputObject>& inputObject)
{
    PartInstance* foundPart;
    bool clickable;
    Vector3 hitPoint;
    TextureId cursorIcon;

    getIndicatedPart(inputObject, true, &foundPart, &clickable, &waypoint, &cursorIcon, true);

    return shared_from(this);
}

shared_ptr<MouseCommand> NewNullTool::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
    PartInstance* foundPart;
    bool clickable;
    Vector3 hitPoint;
    TextureId cursorIcon;

    getIndicatedPart(inputObject, true, &foundPart, &clickable, &waypoint, &cursorIcon);

    return shared_from(this);
}

shared_ptr<MouseCommand> NewNullTool::onRightMouseUp(const shared_ptr<InputObject>& inputObject)
{
    return Super::onRightMouseUp(inputObject);
}

void NewNullTool::render3dAdorn(Adorn* adorn)
{
    Super::render3dAdorn(adorn);
}

} // namespace Aya
