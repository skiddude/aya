

#include "DataModel/BasicPartInstance.hpp"
#include "DataModel/ExtrudedPartInstance.hpp"
#include "DataModel/PrismInstance.hpp"
#include "DataModel/PyramidInstance.hpp"
#include "DataModel/Handles.hpp"
#include "DataModel/GuiObject.hpp"
#include "Reflection/EnumConverter.hpp"

namespace Aya
{
namespace Reflection
{
template<>
EnumDesc<Aya::BasicPartInstance::LegacyPartType>::EnumDesc()
    : EnumDescriptor("PartType")
{
    addPair(Aya::BasicPartInstance::BALL_LEGACY_PART, "Ball");
    addPair(Aya::BasicPartInstance::BLOCK_LEGACY_PART, "Block");
    addPair(Aya::BasicPartInstance::CYLINDER_LEGACY_PART, "Cylinder");
}

template<>
EnumDesc<Aya::ExtrudedPartInstance::VisualTrussStyle>::EnumDesc()
    : EnumDescriptor("Style")
{
    addPair(Aya::ExtrudedPartInstance::FULL_ALTERNATING_CROSS_BEAM, "AlternatingSupports");
    addPair(Aya::ExtrudedPartInstance::BRIDGE_STYLE_CROSS_BEAM, "BridgeStyleSupports");
    addPair(Aya::ExtrudedPartInstance::NO_CROSS_BEAM, "NoSupports");
    addLegacyName("Alternating Supports", Aya::ExtrudedPartInstance::FULL_ALTERNATING_CROSS_BEAM);
    addLegacyName("Bridge Style Supports", Aya::ExtrudedPartInstance::BRIDGE_STYLE_CROSS_BEAM);
    addLegacyName("No Supports", Aya::ExtrudedPartInstance::NO_CROSS_BEAM);
}

template<>
EnumDesc<Aya::PrismInstance::NumSidesEnum>::EnumDesc()
    : EnumDescriptor("PrismSides")
{
    addPair(Aya::PrismInstance::sides3, "3");
    // Don't allow a 4 sided prism - should use block.
    addPair(Aya::PrismInstance::sides5, "5");
    addPair(Aya::PrismInstance::sides6, "6");
    addPair(Aya::PrismInstance::sides8, "8");
    addPair(Aya::PrismInstance::sides10, "10");
    addPair(Aya::PrismInstance::sides20, "20");
}

template<>
EnumDesc<Aya::PyramidInstance::NumSidesEnum>::EnumDesc()
    : EnumDescriptor("PyramidSides")
{
    addPair(Aya::PyramidInstance::sides3, "3");
    addPair(Aya::PyramidInstance::sides4, "4");
    addPair(Aya::PyramidInstance::sides5, "5");
    addPair(Aya::PyramidInstance::sides6, "6");
    addPair(Aya::PyramidInstance::sides8, "8");
    addPair(Aya::PyramidInstance::sides10, "10");
    addPair(Aya::PyramidInstance::sides20, "20");
}

template<>
EnumDesc<Aya::Handles::VisualStyle>::EnumDesc()
    : EnumDescriptor("HandlesStyle")
{
    addPair(Aya::Handles::RESIZE_HANDLES, "Resize");
    addPair(Aya::Handles::MOVEMENT_HANDLES, "Movement");
}

template<>
EnumDesc<Aya::GuiObject::SizeConstraint>::EnumDesc()
    : EnumDescriptor("SizeConstraint")
{
    addPair(Aya::GuiObject::RELATIVE_XY, "RelativeXY");
    addPair(Aya::GuiObject::RELATIVE_XX, "RelativeXX");
    addPair(Aya::GuiObject::RELATIVE_YY, "RelativeYY");
}

template<>
EnumDesc<Aya::GuiObject::ImageScale>::EnumDesc()
    : EnumDescriptor("ScaleType")
{
    addPair(Aya::GuiObject::SCALE_STRETCH, "Stretch");
    addPair(Aya::GuiObject::SCALE_SLICED, "Slice");
}
} // namespace Reflection
} // namespace Aya