


#include "DataModel/ParametricPartInstance.hpp"
#include "World/Primitive.hpp"
#include "World/Poly.hpp"

namespace Aya
{
namespace PART
{

const char* const sWedge = "WedgePart";

ParametricPartInstance::ParametricPartInstance() {}

ParametricPartInstance::~ParametricPartInstance() {}

Wedge::Wedge()
{
    setName("Wedge");
    Primitive* myPrim = this->getPartPrimitive();
    myPrim->setGeometryType(Geometry::GEOMETRY_WEDGE);
    myPrim->setSurfaceType(NORM_Y, NO_SURFACE);
}

Wedge::~Wedge() {}


} // namespace PART
} // namespace Aya