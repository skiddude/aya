

#include "DataModel/legacy.hpp"
#include "Utility/SurfaceType.hpp"
#include "Reflection/EnumConverter.hpp"

namespace Aya
{
namespace Reflection
{
template<>
EnumDesc<Legacy::SurfaceConstraint>::EnumDesc()
    : EnumDescriptor("SurfaceConstraint")
{
    addPair(Legacy::NO_CONSTRAINT, "None");
    addPair(Legacy::ROTATE_LEGACY, "Hinge");
    addPair(Legacy::ROTATE_P_LEGACY, "SteppingMotor");
    addPair(Legacy::ROTATE_V_LEGACY, "Motor");
}
} // namespace Reflection
} // namespace Aya
