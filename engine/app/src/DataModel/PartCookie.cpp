


#include "DataModel/PartCookie.hpp"
#include "DataModel/DataModelMesh.hpp"
#include "DataModel/Decal.hpp"
#include "DataModel/Workspace.hpp"
#include "DataModel/BasicPartInstance.hpp"
#include "DataModel/ExtrudedPartInstance.hpp"
#include "DataModel/PrismInstance.hpp"
#include "DataModel/PyramidInstance.hpp"


namespace Aya
{

unsigned int PartCookie::compute(Aya::PartInstance* part)
{
    unsigned int cookie = part->getCookie() & IS_HUMANOID_PART;

    if (const copy_on_write_ptr<Instances>& children = part->getChildren())
    {
        for (size_t i = 0; i < children->size(); ++i)
        {
            Instance* instance = (*children)[i].get();

            // Note: all data about the special shape refers to the last special shape!
            if (Instance::fastDynamicCast<DataModelMesh>(instance))
            {
                cookie |= HAS_SPECIALSHAPE;
                cookie &= ~(HAS_FILEMESH | HAS_HEADMESH); // clear any leftover flags from the previous special shape

                if (SpecialShape* shape = Instance::fastDynamicCast<SpecialShape>(instance))
                {
                    if (shape->getMeshType() == SpecialShape::FILE_MESH)
                        cookie |= HAS_FILEMESH;
                    else if (shape->getMeshType() == SpecialShape::HEAD_MESH)
                        cookie |= HAS_HEADMESH;
                }
                else if (Instance::fastDynamicCast<FileMesh>(instance))
                {
                    cookie |= HAS_FILEMESH;
                }
            }
            else if (Aya::Decal* decal = Instance::fastDynamicCast<Aya::Decal>(instance))
            {
                if (!decal->getTexture().isNull())
                {
                    cookie |= HAS_DECALS;

                    if (decal->getFace() == Aya::NORM_Z_NEG)
                    {
                        cookie |= HAS_DECALS_Z_NEG;
                    }
                }
            }
        }
    }
    return cookie;
}

} // namespace Aya