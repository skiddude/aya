


#include "DataModel/RightAngleRampInstance.hpp"
#include "World/RightAngleRampPoly.hpp"
#include "World/Primitive.hpp"

namespace Aya
{
const char* const sRightAngleRamp = "RightAngleRampPart";

using namespace Reflection;

const char* category_RightAngleRamp = "Part ";
static const Vector3 InitialRightAngleRampPartSize = Vector3(4.0, 4.0, 2.0);

RightAngleRampInstance::RightAngleRampInstance()
    : DescribedNonCreatable<RightAngleRampInstance, PartInstance, sRightAngleRamp>(InitialRightAngleRampPartSize)
{
    setName("RightAngleRamp");
    Primitive* myPrim = this->getPartPrimitive();
    myPrim->setGeometryType(Geometry::GEOMETRY_RIGHTANGLERAMP);
    myPrim->setSurfaceType(NORM_X_NEG, UNIVERSAL);
    myPrim->setSurfaceType(NORM_Y_NEG, UNIVERSAL);
    myPrim->setSurfaceType(NORM_Y, NO_SURFACE);

    shouldRenderSetDirty();
}

RightAngleRampInstance::~RightAngleRampInstance() {}


} // namespace Aya
