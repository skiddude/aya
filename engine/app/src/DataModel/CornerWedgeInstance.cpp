


#include "DataModel/CornerWedgeInstance.hpp"
#include "World/CornerWedgePoly.hpp"
#include "World/Primitive.hpp"

namespace Aya
{
const char* const sCornerWedge = "CornerWedgePart";

using namespace Reflection;

const char* category_CornerWedge = "Part ";
static const Vector3 InitialCornerWedgePartSize = Vector3(2.0, 2.0, 2.0);

CornerWedgeInstance::CornerWedgeInstance()
    : DescribedCreatable<CornerWedgeInstance, PartInstance, sCornerWedge>(InitialCornerWedgePartSize)
{
    setName("CornerWedge");
    Primitive* myPrim = this->getPartPrimitive();
    myPrim->setGeometryType(Geometry::GEOMETRY_CORNERWEDGE);
    myPrim->setSurfaceType(NORM_X_NEG, NO_SURFACE);
    myPrim->setSurfaceType(NORM_Y_NEG, NO_SURFACE);
    myPrim->setSurfaceType(NORM_Y, NO_SURFACE);

    shouldRenderSetDirty();
}

CornerWedgeInstance::~CornerWedgeInstance() {}


} // namespace Aya