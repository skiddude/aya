


#include "DataModel/ClickDetector.hpp"
#include "DataModel/Workspace.hpp"
#include "DataModel/PartInstance.hpp"
#include "Players.hpp"
#include "Draw.hpp"


namespace Aya
{

const char* const sClickDetector = "ClickDetector";



static Reflection::RemoteEventDesc<ClickDetector, void(shared_ptr<Instance>)> desc_MouseClick(&ClickDetector::mouseClickSignal, "MouseClick",
    "playerWhoClicked", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<ClickDetector, void(shared_ptr<Instance>)> dep_MouseClick(&ClickDetector::mouseClickSignal, "mouseClick",
    "playerWhoClicked", Security::None,
    Reflection::RemoteEventCommon::Attributes::deprecated(Reflection::RemoteEventCommon::SCRIPTING, &desc_MouseClick),
    Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<ClickDetector, void(shared_ptr<Instance>)> desc_RightMouseClick(&ClickDetector::rightMouseClickSignal,
    "RightMouseClick", "playerWhoClicked", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<ClickDetector, void(shared_ptr<Instance>)> desc_MouseHoverEnter(&ClickDetector::mouseHoverSignal,
    "MouseHoverEnter", "playerWhoHovered", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<ClickDetector, void(shared_ptr<Instance>)> desc_MouseHoverLeave(&ClickDetector::mouseHoverLeaveSignal,
    "MouseHoverLeave", "playerWhoHovered", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);

Reflection::BoundProp<float> ClickDetector::propMaxActivationDistance("MaxActivationDistance", category_Data, &ClickDetector::maxActivationDistance);
Reflection::BoundProp<TextureId> ClickDetector::propCursorIcon("CursorIcon", category_Image, &ClickDetector::cursorIcon);
REFLECTION_END();


ClickDetector::ClickDetector()
    : cycle(0)
    , maxActivationDistance(32.0)
{
    setName("ClickDetector");
    lastHoverPart = shared_ptr<PartInstance>();
}


bool containsClickDetector(Instance* instance)
{
    for (size_t i = 0; i < instance->numChildren(); ++i)
    {
        Instance* child = instance->getChild(i);
        if (Instance::fastDynamicCast<ClickDetector>(child))
        {
            return true;
        }
    }
    return false;
}

bool ancestorContainsClickDetector(Instance* instance)
{
    AYAASSERT(instance);
    if (containsClickDetector(instance))
    {
        return true;
    }
    else
    {
        if (Instance* parent = instance->getParent())
        {
            if (!Instance::fastDynamicCast<Workspace>(parent))
            { // bail out at workspace level to prevent going through all workspace children
                return ancestorContainsClickDetector(parent);
            }
        }
        return false;
    }
}



void renderClickDetector(shared_ptr<Aya::Instance> descendant, Adorn* adorn, int cycle)
{
    PartInstance* part = Instance::fastDynamicCast<PartInstance>(descendant.get());
    if (part)
    {
        //		Vector3 size = 0.5f * (part->getPartSizeXml() + 0.5 * Vector3::one());	// one stud bigger
        //		AABox fieldBox(-size, size);
        //		adorn->setObjectToWorldMatrix(part->getCoordinateFrame());
        //		adorn->box(fieldBox, Color4(0.0, 0.0, 1.0, 0.5) );

        int max = ClickDetector::cycles();
        int half = max / 2;

        int value = cycle < half ? cycle : max - cycle;
        float percent = static_cast<float>(value) / static_cast<float>(half);

        Color4 color(1.0f - percent, percent, 0.5f);

        Draw::selectionBox(part->getPart(), adorn, color);
    }
}


void ClickDetector::render3dAdorn(Adorn* adorn)
{
    /*
    Instance* parent = this->getParent();

    AYAASSERT(!dynamic_cast<Workspace*>(parent));

    cycle = (cycle + 1) % cycles();

    parent->visitDescendants(boost::bind(&renderClickDetector, _1, adorn, cycle));
    renderClickDetector(shared_from(parent), adorn, cycle);
    */
}

void ClickDetector::fireMouseClick(float distance, Aya::Network::Player* player)
{
    if (distance < maxActivationDistance)
        desc_MouseClick.fireAndReplicateEvent(this, shared_from(player));
}

void ClickDetector::fireRightMouseClick(float distance, Aya::Network::Player* player)
{
    if (distance < maxActivationDistance)
        desc_RightMouseClick.fireAndReplicateEvent(this, shared_from(player));
}

bool ClickDetector::isClickable(shared_ptr<PartInstance> part, float distanceToCharacter, bool raiseClickedEvent, Aya::Network::Player* player,
    TextureId** cursorIcon, bool isRightClick)
{
    shared_ptr<Instance> clickDetectorParent = part;
    while (clickDetectorParent && fastSharedDynamicCast<Workspace>(clickDetectorParent) == NULL)
    {
        if (shared_ptr<ClickDetector> cd = shared_from(clickDetectorParent->findFirstChildOfType<ClickDetector>()))
        {
            **cursorIcon = cd->getCursorIcon();

            if (distanceToCharacter < cd->getMaxActivationDistance())
            {
                if (raiseClickedEvent)
                {
                    if (isRightClick)
                        cd->fireRightMouseClick(distanceToCharacter, player);
                    else
                        cd->fireMouseClick(distanceToCharacter, player);
                }
                return true;
            }
        }
        clickDetectorParent = shared_from(clickDetectorParent->getParent());
    }
    return false;
}

bool ClickDetector::updateLastHoverPart(shared_ptr<Instance> newHover, Aya::Network::Player* player)
{
    if (newHover != lastHoverPart)
    {
        if (newHover)
            fireMouseHover(player);
        lastHoverPart = newHover;

        return true;
    }
    return false;
}

void ClickDetector::fireMouseHover(Aya::Network::Player* player)
{
    desc_MouseHoverEnter.fireAndReplicateEvent(this, shared_from(player));
}

void ClickDetector::fireMouseHoverLeave(Aya::Network::Player* player)
{
    desc_MouseHoverLeave.fireAndReplicateEvent(this, shared_from(player));
    lastHoverPart = shared_ptr<PartInstance>();
}

void ClickDetector::stopHover(shared_ptr<PartInstance> part, Aya::Network::Player* player)
{
    if (part && part.get())
    {
        if (Instance* instance = fastDynamicCast<Instance>(part.get()))
        {
            do
            {
                if (ClickDetector* cd = instance->findFirstChildOfType<ClickDetector>())
                    cd->fireMouseHoverLeave(player);
                instance = instance->getParent();
            } while (instance != NULL && Instance::fastDynamicCast<Workspace>(instance) == NULL);
        }
    }
}
bool ClickDetector::isHovered(PartInstance* part, float distanceToCharacter, bool raiseHoveredEvent, Aya::Network::Player* player)
{
    if (part)
    {
        shared_ptr<Instance> i = shared_from(part);
        do
        {
            if (shared_ptr<ClickDetector> cd = shared_from(i->findFirstChildOfType<ClickDetector>()))
            {
                if ((distanceToCharacter < cd->getMaxActivationDistance()) && cd)
                {
                    return cd->updateLastHoverPart(i, player);
                }
            }
            i = shared_from(i->getParent());
        } while (i != NULL && Instance::fastDynamicCast<Workspace>(i.get()) == NULL);
    }
    return false;
}

} // namespace Aya