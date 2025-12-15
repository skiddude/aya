


#include "World/Edge.hpp"


namespace Aya
{

Edge::Edge(Primitive* prim0, Primitive* prim1)
    : edgeState(Sim::UNDEFINED)
    , throttleType(Sim::UNDEFINED_THROTTLE)
    , prim0(prim0)
    , prim1(prim1)
    , index0(-1)
    , index1(-1)
{
}


void Edge::setPrimitive(int i, Primitive* p)
{
    //	AYAASSERT(!this->inPipeline());
    AYAASSERT((i == 0) || (i == 1));
    if (i == 0)
    {
        prim0 = p;
    }
    else
    {
        prim1 = p;
    }
}


} // namespace Aya