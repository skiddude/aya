#pragma once

#include "Tree/Service.hpp"
#include "Utility/G3DCore.hpp"
#include "Utility/Extents.hpp"
#include "Array.hpp"

#include "Utility/PartMaterial.hpp"

namespace Aya
{

class PartInstance;
class Primitive;
class ContactManager;
class World;
class Workspace;
class Instance;

extern const char* const sGeometryService;
class GeometryService
    : public DescribedNonCreatable<GeometryService, Instance, sGeometryService>
    , public Service
{
private:
    typedef DescribedNonCreatable<GeometryService, Instance, sGeometryService> Super;
    Workspace* workspace;
    G3D::Array<Primitive*> foundPrimitives;

public:
    GeometryService();

    Vector3 getHitLocationFilterStairs(Instance* ancestor, Aya::RbxRay ray, Primitive** hitPrim);

    Vector3 getHitLocationFilterDescendents(Instance* ancestor, Aya::RbxRay ray, Primitive** hitPrim, Vector3& surfaceNormal,
        PartMaterial& surfaceMaterial, bool terrainCellsAreCubes, bool ignoreWaterCells);
    Vector3 getHitLocationFilterDescendents(const Instances* ancestors, Aya::RbxRay ray, Primitive** hitPrim, Vector3& surfaceNormal,
        PartMaterial& surfaceMaterial, bool terrainCellsAreCubes, bool ignoreWaterCells);

    // we template this function to avoid heavy code duplication: IgnoreType is currently either Instance or Instances
    template<class IgnoreType>
    Vector3 getHitLocationPartFilterDescendents(IgnoreType* ancestor, Aya::RbxRay ray, shared_ptr<PartInstance>& result, Vector3& surfaceNormal,
        PartMaterial& surfaceMaterial, bool terrainCellsAreCubes, bool ignoreWaterCells);

    void getPartsTouchingExtents(const Extents& extents, const Primitive* ignore, int maxCount, G3D::Array<PartInstance*>& found);

    void getPartsTouchingExtentsWithIgnore(const Extents& extents, const Instances* ancestors, int maxCount, G3D::Array<PartInstance*>& found);

protected:
    virtual void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
};

} // namespace Aya
