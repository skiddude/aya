#include "Utility/Extents.hpp"

#include <CGAL/Aff_transformation_3.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>

#include <string>
#include <vector>

#include "DataModel/CSGMesh.hpp"

namespace Aya
{

class CSGMeshCGAL : public CSGMesh
{
public:
    using Kernel = CGAL::Exact_predicates_inexact_constructions_kernel;
    using SurfaceMesh = CGAL::Surface_mesh<Kernel::Point_3>;
    using Transform = CGAL::Aff_transformation_3<Kernel>;

    CSGMeshCGAL();
    CSGMeshCGAL(const CSGMeshCGAL& mesh);
    ~CSGMeshCGAL() override;

    CSGMesh* clone() const override;
    CSGMeshCGAL& operator=(const CSGMeshCGAL& mesh);

    bool isValid() const override;

    void translate(const G3D::Vector3& translation) override;
    void applyCoordinateFrame(G3D::CoordinateFrame cFrame) override;
    void applyTranslation(const G3D::Vector3& trans) override;
    void applyScale(const G3D::Vector3& size) override;
    void applyColor(const G3D::Vector3& color) override;

    void triangulate() override;
    bool newTriangulate() override;
    void weldMesh(bool positionOnly = false) override;

    std::string getBRepBinaryString() const override;
    void setBRepFromBinaryString(const std::string& str) override;

    void buildBRep() override;

    bool unionMesh(const CSGMesh* a, const CSGMesh* b) override;
    bool intersectMesh(const CSGMesh* a, const CSGMesh* b) override;
    bool subractMesh(const CSGMesh* a, const CSGMesh* b) override;

    size_t clusterVertices(float resolution) override;
    bool makeHalfEdges(std::vector<int>& vertexEdges) override;

    G3D::Vector3 extentsCenter() override;
    G3D::Vector3 extentsSize() override;

private:
    enum class BooleanOperation
    {
        Union,
        Intersection,
        Difference
    };

    SurfaceMesh surfaceMesh;
    Aya::Extents extents;
    bool meshValid;
    bool dirtyTriangulation;
    G3D::Color4uint8 uniformColor;
    std::vector<G3D::Color4uint8> vertexColorBuffer;

    void invalidateTriangulation();
    bool ensureTriangulation();
    void updateTriangulation();
    void computeExtentsFromVertices();
    void applyTransform(const Transform& transform);
    void resetColorBuffer(const G3D::Color4uint8& color);
    bool performBooleanOperation(const CSGMeshCGAL& lhs, const CSGMeshCGAL& rhs, BooleanOperation operation);
    static unsigned determineUVProjection(const G3D::Vector3& normal);
    static G3D::Vector3 computeFlatTangent(const CSGVertex& a, const CSGVertex& b, const CSGVertex& c);
    static bool isFiniteVector(const G3D::Vector3& vec);
};

class CSGMeshFactoryCGAL : public CSGMeshFactory
{
public:
    CSGMesh* createMesh() const;
};

} // namespace Aya
