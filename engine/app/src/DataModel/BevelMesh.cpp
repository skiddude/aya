


#include "DataModel/BevelMesh.hpp"

using namespace Aya;

const char* const Aya::sBevelMesh = "BevelMesh";


static Reflection::PropDescriptor<BevelMesh, float> desc_bevel(
    "Bevel", category_Data, &BevelMesh::getBevel, &BevelMesh::setBevel, Aya::Reflection::PropertyDescriptor::STREAMING);
static Reflection::PropDescriptor<BevelMesh, float> desc_roundness(
    "Bevel Roundness", category_Data, &BevelMesh::getRoundness, &BevelMesh::setRoundness, Aya::Reflection::PropertyDescriptor::STREAMING);
static Reflection::PropDescriptor<BevelMesh, float> desc_bulge(
    "Bulge", category_Data, &BevelMesh::getBulge, &BevelMesh::setBulge, Aya::Reflection::PropertyDescriptor::STREAMING);
REFLECTION_END();

BevelMesh::BevelMesh()
    : bevel(0.0)
    , roundness(0.0)
    , bulge(0.0)
{
}

const float BevelMesh::getBevel() const
{
    return bevel;
}

void BevelMesh::setBevel(const float bev)
{
    bevel = bev;
    raisePropertyChanged(desc_bevel);
}

const float BevelMesh::getRoundness() const
{
    return roundness;
}
void BevelMesh::setRoundness(const float rnd)
{
    roundness = rnd;
    raisePropertyChanged(desc_roundness);
}

const float BevelMesh::getBulge() const
{
    return bulge;
}

void BevelMesh::setBulge(const float blg)
{
    bulge = blg;
    raisePropertyChanged(desc_bulge);
}