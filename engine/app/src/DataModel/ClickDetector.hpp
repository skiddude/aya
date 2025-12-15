

#pragma once

#include "Tree/Instance.hpp"
#include "Base/IAdornable.hpp"
#include "DataModel/PartInstance.hpp"
#include "DataModel/ModelInstance.hpp"
#include "Utility/TextureId.hpp"

namespace Aya
{
namespace Network
{
class Players;
class Player;
} // namespace Network

class PartInstance;
class ModelInstance;

extern const char* const sClickDetector;
class ClickDetector
    : public DescribedCreatable<ClickDetector, Instance, sClickDetector>
    , public IAdornable
{
private:
    int cycle;
    float maxActivationDistance; // max distance a character can be from the button and still raise events
    TextureId cursorIcon;        // cursor icon that shows when the player hovers over this clickdetector
    shared_ptr<Instance> lastHoverPart;

    // IAdornable
    /*override*/ bool shouldRender3dAdorn() const
    {
        return true;
    }
    /*override*/ void render3dAdorn(Adorn* adorn);

public:
    ClickDetector();
    virtual ~ClickDetector() {}

    void fireMouseClick(float distance, Aya::Network::Player* player);
    void fireRightMouseClick(float distance, Aya::Network::Player* player);
    void fireMouseHover(Aya::Network::Player* player);
    void fireMouseHoverLeave(Aya::Network::Player* player);
    static Reflection::BoundProp<float> propMaxActivationDistance;
    static Reflection::BoundProp<TextureId> propCursorIcon;

    shared_ptr<Instance> getLastHoverPart()
    {
        return lastHoverPart;
    }
    bool updateLastHoverPart(shared_ptr<Instance> newHover, Aya::Network::Player* player);

    Aya::remote_signal<void(shared_ptr<Instance>)> mouseClickSignal;
    Aya::remote_signal<void(shared_ptr<Instance>)> rightMouseClickSignal;
    Aya::remote_signal<void(shared_ptr<Instance>)> mouseHoverSignal;
    Aya::remote_signal<void(shared_ptr<Instance>)> mouseHoverLeaveSignal;

    static int cycles()
    {
        return 30;
    }

    float getMaxActivationDistance()
    {
        return maxActivationDistance;
    }
    TextureId getCursorIcon()
    {
        return cursorIcon;
    }


    static bool isClickable(shared_ptr<PartInstance> part, float distanceToCharacter, bool raiseClickedEvent, Aya::Network::Player* player,
        TextureId** cursorIcon = NULL, bool isRightClick = false);
    static bool isHovered(PartInstance* part, float distanceToCharacter, bool raiseHoveredEvent, Aya::Network::Player* player);
    static void stopHover(shared_ptr<PartInstance> part, Aya::Network::Player* player);
    /* override */ bool askSetParent(const Instance* parent) const
    {
        return (Instance::fastDynamicCast<PartInstance>(parent) != NULL) || (Instance::fastDynamicCast<ModelInstance>(parent) != NULL);
    }
    /* override */ bool askAddChild(const Instance* instance) const
    {
        return true;
    }
};
} // namespace Aya