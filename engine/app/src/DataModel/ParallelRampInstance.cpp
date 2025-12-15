


#include "DataModel/ParallelRampInstance.hpp"
#include "World/ParallelRampPoly.hpp"
#include "World/Primitive.hpp"

namespace Aya
{
const char* const sParallelRamp = "ParallelRampPart";

using namespace Reflection;

const char* category_ParallelRamp = "Part ";
static const Vector3 InitialParallelRampPartSize = Vector3(4.0, 4.0, 2.0);

ParallelRampInstance::ParallelRampInstance()
    : DescribedNonCreatable<ParallelRampInstance, PartInstance, sParallelRamp>(InitialParallelRampPartSize)
{
    setName("ParallelRamp");
    Primitive* myPrim = this->getPartPrimitive();
    myPrim->setGeometryType(Geometry::GEOMETRY_PARALLELRAMP);
    myPrim->setSurfaceType(NORM_X, UNIVERSAL);
    myPrim->setSurfaceType(NORM_X_NEG, UNIVERSAL);
    myPrim->setSurfaceType(NORM_Y, NO_SURFACE);
    myPrim->setSurfaceType(NORM_Y_NEG, NO_SURFACE);

    shouldRenderSetDirty();
}

ParallelRampInstance::~ParallelRampInstance() {}


} // namespace Aya
