

#pragma once

#include "DataModel/HandlesBase.hpp"
#include "DataModel/EventReplicator.hpp"
#include "Base/IAdornable.hpp"
#include "Utility/Axes.hpp"

#include "HandleType.hpp"

namespace Aya
{
extern const char* const sArcHandles;

class ArcHandles : public DescribedCreatable<ArcHandles, HandlesBase, sArcHandles>
{
private:
    typedef DescribedCreatable<ArcHandles, HandlesBase, sArcHandles> Super;

public:
    ArcHandles();

    Aya::remote_signal<void(Aya::Vector3::Axis)> mouseEnterSignal;
    Aya::remote_signal<void(Aya::Vector3::Axis)> mouseLeaveSignal;
    Aya::remote_signal<void(Aya::Vector3::Axis, float, float)> mouseDragSignal;
    Aya::remote_signal<void(Aya::Vector3::Axis)> mouseButton1DownSignal;
    Aya::remote_signal<void(Aya::Vector3::Axis)> mouseButton1UpSignal;

    DECLARE_EVENT_REPLICATOR_SIG(ArcHandles, MouseEnter, void(Aya::Vector3::Axis));
    DECLARE_EVENT_REPLICATOR_SIG(ArcHandles, MouseLeave, void(Aya::Vector3::Axis));
    DECLARE_EVENT_REPLICATOR_SIG(ArcHandles, MouseDrag, void(Aya::Vector3::Axis, float, float));
    DECLARE_EVENT_REPLICATOR_SIG(ArcHandles, MouseButton1Down, void(Aya::Vector3::Axis));
    DECLARE_EVENT_REPLICATOR_SIG(ArcHandles, MouseButton1Up, void(Aya::Vector3::Axis));

    void setAxes(Axes value);
    Axes getAxes() const
    {
        return axes;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Instance
    /*override*/ void onPropertyChanged(const Reflection::PropertyDescriptor& descriptor);


    ////////////////////////////////////////////////////////////////////////////////////
    //
    // GuiBase
    /*override*/ GuiResponse process(const shared_ptr<InputObject>& event);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // HandlesBase
    /*override*/ Aya::HandleType getHandleType() const;

protected:
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // HandlesBase
    /*override*/ int getHandlesNormalIdMask() const;
    /*override*/ void setServerGuiObject();

private:
    Axes axes;
};

} // namespace Aya
