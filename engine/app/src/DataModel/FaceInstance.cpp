

#include "DataModel/FaceInstance.hpp"
#include "DataModel/PartInstance.hpp"
#include "DrawAdorn.hpp"

using namespace Aya;

const Reflection::EnumPropDescriptor<FaceInstance, NormalId> FaceInstance::prop_Face(
    "Face", category_Data, &FaceInstance::getFace, &FaceInstance::setFace);
const char* const Aya::sFaceInstance = "FaceInstance";

FaceInstance::FaceInstance(void)
    : face(NORM_Z_NEG)
{
}

bool FaceInstance::askSetParent(const Instance* instance) const
{
    return Instance::fastDynamicCast<PartInstance>(instance) != NULL;
}

void FaceInstance::setFace(NormalId value)
{
    if (this->face != value)
    {
        this->face = value;
        this->raisePropertyChanged(prop_Face);
    }
}

// IAdornable
void FaceInstance::render3dSelect(Adorn* adorn, SelectState selectState)
{
    IAdornable::render3dAdorn(adorn);

    if (PartInstance* parentPart = Instance::fastDynamicCast<PartInstance>(getParent()))
    {
        DrawAdorn::partSurface(parentPart->getPart(), face, adorn);
    }
}
