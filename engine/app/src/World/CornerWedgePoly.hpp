#pragma once

#include "World/Poly.hpp"
#include "World/GeometryPool.hpp"
#include "World/CornerWedgeMesh.hpp"
#include "World/BlockMesh.hpp"
#include "World/BulletGeometryPoolObjects.hpp"

namespace Aya
{

class CornerWedgePoly : public Poly
{
public:
    typedef GeometryPool<Vector3, POLY::CornerWedgeMesh, Vector3Comparer> CornerWedgeMeshPool;
    typedef GeometryPool<Vector3, BulletCornerWedgeShapeWrapper, Vector3Comparer> BulletCornerWedgeShapePool;

    /*override*/ Matrix3 getMoment(float mass) const;
    /*override*/ Vector3 getCofmOffset() const;
    /*override*/ CoordinateFrame getSurfaceCoordInBody(const size_t surfaceId) const;
    /*override*/ bool isGeometryOrthogonal(void) const
    {
        return false;
    }
    /*override*/ bool setUpBulletCollisionData(void);
    /*override*/ void setSize(const G3D::Vector3& _size);

private:
    typedef Poly Super;

    CornerWedgeMeshPool::Token aCornerWedgeMesh;
    BulletCornerWedgeShapePool::Token bulletCornerWedgeShape;

    /*override*/ virtual Vector3 getCenterToCorner(const Matrix3& rotation) const;

    void updateBulletCollisionData();

protected:
    // Geometry Overrides
    /*override*/ virtual GeometryType getGeometryType() const
    {
        return GEOMETRY_CORNERWEDGE;
    }

    // Poly Overrides
    /*override*/ void buildMesh();
    /*override*/ size_t getFaceFromLegacyNormalId(const NormalId nId) const;
};

} // namespace Aya
