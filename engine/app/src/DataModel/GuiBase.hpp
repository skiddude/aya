

#pragma once

#include "GUI/GuiEvent.hpp"
#include "Utility/G3DCore.hpp"
#include "DataModel/InputObject.hpp"
#include "DataModel/UserInputService.hpp"
#include "Base/IAdornable.hpp"

namespace Aya
{

enum GuiQueue
{
    GUIQUEUE_GENERAL = 0,
    GUIQUEUE_TEXT,

    GUIQUEUE_COUNT
};

extern const char* const sGuiBase;

class GuiBase
    : public DescribedNonCreatable<GuiBase, Instance, sGuiBase>
    , public IAdornable
{
public:
    GuiBase(const char* name);

    static int minZIndex()
    {
        return 0;
    }
    static int maxZIndex()
    {
        return 10;
    }

    static int minZIndex2d()
    {
        return minZIndex() + 1;
    }
    static int maxZIndex2d()
    {
        return maxZIndex();
    }

    virtual GuiResponse process(const shared_ptr<InputObject>& event)
    {
        return GuiResponse::notSunk();
    }
    virtual bool canProcessMeAndDescendants() const = 0;

    virtual GuiResponse processGesture(const UserInputService::Gesture& gesture, const shared_ptr<const Aya::Reflection::ValueArray>& touchPositions,
        const shared_ptr<const Reflection::Tuple>& args)
    {
        return GuiResponse::notSunk();
    }

    virtual int getZIndex() const = 0;
    virtual GuiQueue getGuiQueue() const = 0;

private:
    typedef DescribedNonCreatable<GuiBase, Instance, sGuiBase> Super;
};
} // namespace Aya
