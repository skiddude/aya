#include "Utility/CSGKernel.hpp"

#include "DataModel/CSGMesh.hpp"

#include "Color4uint8.hpp"
#include "Vector2.hpp"
#include "Vector3.hpp"

#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Polygon_mesh_processing/repair.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/boost/graph/helpers.h>

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <sstream>
#include <utility>

namespace Aya
{
namespace
{
namespace PMP = CGAL::Polygon_mesh_processing;

inline G3D::Color4uint8 convertColor(const G3D::Vector3& color)
{
    const auto clamp = [](float value) -> unsigned char {
        float scaled = std::max(0.0f, std::min(1.0f, value));
        return static_cast<unsigned char>(std::round(scaled * 255.0f));
    };

    return G3D::Color4uint8(clamp(color.x), clamp(color.y), clamp(color.z), 255);
}

inline bool isTriangleDegenerate(const G3D::Vector3& a, const G3D::Vector3& b, const G3D::Vector3& c)
{
    const G3D::Vector3 normal = (b - a).cross(c - a);
    return normal.squaredLength() <= 1e-10f;
}

} // namespace

CSGMeshCGAL::CSGMeshCGAL()
    : meshValid(false)
    , dirtyTriangulation(true)
    , uniformColor(255, 255, 255, 255)
{
    extents = Extents::negativeMaxExtents();
}

CSGMeshCGAL::CSGMeshCGAL(const CSGMeshCGAL& mesh)
    : CSGMesh(mesh)
    , surfaceMesh(mesh.surfaceMesh)
    , extents(mesh.extents)
    , meshValid(mesh.meshValid)
    , dirtyTriangulation(mesh.dirtyTriangulation)
    , uniformColor(mesh.uniformColor)
    , vertexColorBuffer(mesh.vertexColorBuffer)
{
}

CSGMeshCGAL::~CSGMeshCGAL() = default;

CSGMesh* CSGMeshCGAL::clone() const
{
    return new CSGMeshCGAL(*this);
}

CSGMeshCGAL& CSGMeshCGAL::operator=(const CSGMeshCGAL& mesh)
{
    if (this != &mesh)
    {
        CSGMesh::operator=(mesh);
        surfaceMesh = mesh.surfaceMesh;
        extents = mesh.extents;
        meshValid = mesh.meshValid;
        dirtyTriangulation = mesh.dirtyTriangulation;
        uniformColor = mesh.uniformColor;
        vertexColorBuffer = mesh.vertexColorBuffer;
    }
    return *this;
}

bool CSGMeshCGAL::isValid() const
{
    return meshValid && !surfaceMesh.is_empty();
}

void CSGMeshCGAL::invalidateTriangulation()
{
    dirtyTriangulation = true;
    extents = Extents::negativeMaxExtents();
}

bool CSGMeshCGAL::isFiniteVector(const G3D::Vector3& vec)
{
    return std::isfinite(vec.x) && std::isfinite(vec.y) && std::isfinite(vec.z);
}

unsigned CSGMeshCGAL::determineUVProjection(const G3D::Vector3& normal)
{
    const G3D::Vector3 absNormal = G3D::Vector3(std::abs(normal.x), std::abs(normal.y), std::abs(normal.z));
    if (absNormal.x >= absNormal.y && absNormal.x >= absNormal.z)
        return normal.x >= 0.0f ? CSGVertex::UV_BOX_X : CSGVertex::UV_BOX_X_NEG;
    if (absNormal.y >= absNormal.x && absNormal.y >= absNormal.z)
        return normal.y >= 0.0f ? CSGVertex::UV_BOX_Y : CSGVertex::UV_BOX_Y_NEG;
    return normal.z >= 0.0f ? CSGVertex::UV_BOX_Z : CSGVertex::UV_BOX_Z_NEG;
}

G3D::Vector3 CSGMeshCGAL::computeFlatTangent(const CSGVertex& a, const CSGVertex& b, const CSGVertex& c)
{
    const G3D::Vector3& v1 = a.pos;
    const G3D::Vector3& v2 = b.pos;
    const G3D::Vector3& v3 = c.pos;
    const G3D::Vector2& w1 = a.uv;
    const G3D::Vector2& w2 = b.uv;
    const G3D::Vector2& w3 = c.uv;

    const float x1 = v2.x - v1.x;
    const float x2 = v3.x - v1.x;
    const float y1 = v2.y - v1.y;
    const float y2 = v3.y - v1.y;
    const float z1 = v2.z - v1.z;
    const float z2 = v3.z - v1.z;
    const float s1 = w2.x - w1.x;
    const float s2 = w3.x - w1.x;
    const float t1 = w2.y - w1.y;
    const float t2 = w3.y - w1.y;

    float r = s1 * t2 - s2 * t1;
    if (std::abs(r) < 1e-10f)
        r = 1.0f;
    else
        r = 1.0f / r;

    return G3D::Vector3((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
}

void CSGMeshCGAL::resetColorBuffer(const G3D::Color4uint8& color)
{
    uniformColor = color;
    vertexColorBuffer.assign(surfaceMesh.number_of_vertices(), uniformColor);
}

void CSGMeshCGAL::computeExtentsFromVertices()
{
    extents = Extents::negativeMaxExtents();
    for (const CSGVertex& vertex : vertices)
    {
        if (isFiniteVector(vertex.pos))
            extents.expandToContain(vertex.pos);
    }
}

void CSGMeshCGAL::applyTransform(const Transform& transform)
{
    const bool hadSyncedVertices = !dirtyTriangulation;

    if (meshValid && !surfaceMesh.is_empty())
    {
        for (SurfaceMesh::Vertex_index vertex : surfaceMesh.vertices())
        {
            surfaceMesh.point(vertex) = transform(surfaceMesh.point(vertex));
        }
    }

    if (hadSyncedVertices)
    {
        for (CSGVertex& vertex : vertices)
        {
            Kernel::Point_3 point(vertex.pos.x, vertex.pos.y, vertex.pos.z);
            point = transform(point);
            vertex.pos = G3D::Vector3(static_cast<float>(point.x()), static_cast<float>(point.y()), static_cast<float>(point.z()));
        }
    }

    invalidateTriangulation();
}

void CSGMeshCGAL::translate(const G3D::Vector3& translation)
{
    applyTranslation(translation);
}

void CSGMeshCGAL::applyCoordinateFrame(G3D::CoordinateFrame cFrame)
{
    const Transform transform(
        Kernel::FT(cFrame.rotation[0][0]), Kernel::FT(cFrame.rotation[0][1]), Kernel::FT(cFrame.rotation[0][2]), Kernel::FT(cFrame.translation.x),
        Kernel::FT(cFrame.rotation[1][0]), Kernel::FT(cFrame.rotation[1][1]), Kernel::FT(cFrame.rotation[1][2]), Kernel::FT(cFrame.translation.y),
        Kernel::FT(cFrame.rotation[2][0]), Kernel::FT(cFrame.rotation[2][1]), Kernel::FT(cFrame.rotation[2][2]), Kernel::FT(cFrame.translation.z));
    applyTransform(transform);
}

void CSGMeshCGAL::applyTranslation(const G3D::Vector3& trans)
{
    const Transform transform(CGAL::TRANSLATION, Kernel::Vector_3(trans.x, trans.y, trans.z));
    applyTransform(transform);
}

void CSGMeshCGAL::applyScale(const G3D::Vector3& size)
{
    const Transform transform(Kernel::FT(size.x), Kernel::FT(0.0f), Kernel::FT(0.0f), Kernel::FT(0.0f),
        Kernel::FT(0.0f), Kernel::FT(size.y), Kernel::FT(0.0f), Kernel::FT(0.0f), Kernel::FT(0.0f), Kernel::FT(0.0f), Kernel::FT(size.z), Kernel::FT(0.0f));
    applyTransform(transform);
}

void CSGMeshCGAL::applyColor(const G3D::Vector3& color)
{
    const G3D::Color4uint8 converted = convertColor(color);
    uniformColor = converted;

    if (!surfaceMesh.is_empty())
        resetColorBuffer(converted);

    if (!dirtyTriangulation)
    {
        for (CSGVertex& vertex : vertices)
            vertex.color = converted;
    }
}

void CSGMeshCGAL::triangulate()
{
    newTriangulate();
}

bool CSGMeshCGAL::ensureTriangulation()
{
    if (!meshValid || surfaceMesh.is_empty())
    {
        vertices.clear();
        indices.clear();
        return false;
    }

    if (dirtyTriangulation)
        updateTriangulation();

    return meshValid && !vertices.empty() && !indices.empty();
}

void CSGMeshCGAL::updateTriangulation()
{
    vertices.clear();
    indices.clear();

    if (!meshValid || surfaceMesh.is_empty())
    {
        dirtyTriangulation = false;
        meshValid = false;
        return;
    }

    SurfaceMesh workingMesh = surfaceMesh;
    PMP::triangulate_faces(workingMesh);
    PMP::remove_degenerate_faces(workingMesh);
    PMP::remove_isolated_vertices(workingMesh);
    workingMesh.collect_garbage();

    const size_t vertexCount = workingMesh.number_of_vertices();
    const size_t faceCount = workingMesh.number_of_faces();

    if (vertexCount == 0 || faceCount == 0)
    {
        dirtyTriangulation = false;
        meshValid = false;
        return;
    }

    std::vector<G3D::Vector3> accumulatedNormals(vertexCount, G3D::Vector3::zero());
    std::vector<G3D::Vector3> accumulatedTangents(vertexCount, G3D::Vector3::zero());

    vertices.resize(vertexCount);
    size_t index = 0;
    for (SurfaceMesh::Vertex_index vertex : workingMesh.vertices())
    {
        const Kernel::Point_3& point = workingMesh.point(vertex);
        CSGVertex csgVertex;
        csgVertex.pos = G3D::Vector3(static_cast<float>(point.x()), static_cast<float>(point.y()), static_cast<float>(point.z()));
        csgVertex.normal = G3D::Vector3::zero();
        csgVertex.tangent = G3D::Vector3::zero();
        csgVertex.edgeDistances = G3D::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
        csgVertex.color = (vertex.idx() < vertexColorBuffer.size()) ? vertexColorBuffer[vertex.idx()] : uniformColor;
        csgVertex.extra = G3D::Color4uint8(CSGVertex::UV_BOX_Z, 0, 0, 0);
        csgVertex.uv = G3D::Vector2::zero();
        csgVertex.uvStuds = G3D::Vector2::zero();
        csgVertex.uvDecal = G3D::Vector2::zero();
        vertices[index++] = csgVertex;
    }

    indices.reserve(faceCount * 3);

    for (SurfaceMesh::Face_index face : workingMesh.faces())
    {
        std::array<SurfaceMesh::Vertex_index, 3> faceVertices;
        size_t local = 0;
        for (SurfaceMesh::Vertex_index vertex : CGAL::vertices_around_face(workingMesh.halfedge(face), workingMesh))
        {
            if (local < 3)
                faceVertices[local++] = vertex;
        }
        if (local != 3)
            continue;

        const unsigned int i0 = faceVertices[0].idx();
        const unsigned int i1 = faceVertices[1].idx();
        const unsigned int i2 = faceVertices[2].idx();

        CSGVertex& v0 = vertices[i0];
        CSGVertex& v1 = vertices[i1];
        CSGVertex& v2 = vertices[i2];

        if (!isFiniteVector(v0.pos) || !isFiniteVector(v1.pos) || !isFiniteVector(v2.pos))
            continue;

        if (isTriangleDegenerate(v0.pos, v1.pos, v2.pos))
            continue;

        const G3D::Vector3 faceNormal = (v1.pos - v0.pos).cross(v2.pos - v0.pos);
        const unsigned projection = determineUVProjection(faceNormal);
        v0.extra.r = projection;
        v1.extra.r = projection;
        v2.extra.r = projection;
        v0.uv = v0.generateUv(v0.pos);
        v1.uv = v1.generateUv(v1.pos);
        v2.uv = v2.generateUv(v2.pos);

        const G3D::Vector3 tangent = computeFlatTangent(v0, v1, v2);

        accumulatedNormals[i0] += faceNormal;
        accumulatedNormals[i1] += faceNormal;
        accumulatedNormals[i2] += faceNormal;
        accumulatedTangents[i0] += tangent;
        accumulatedTangents[i1] += tangent;
        accumulatedTangents[i2] += tangent;

        indices.push_back(i0);
        indices.push_back(i1);
        indices.push_back(i2);
    }

    if (indices.empty())
    {
        vertices.clear();
        dirtyTriangulation = false;
        meshValid = false;
        return;
    }

    for (size_t i = 0; i < vertices.size(); ++i)
    {
        vertices[i].normal = accumulatedNormals[i].directionOrZero();
        vertices[i].tangent = accumulatedTangents[i].directionOrZero();
        vertices[i].color = (i < vertexColorBuffer.size()) ? vertexColorBuffer[i] : uniformColor;
    }

    computeExtentsFromVertices();
    dirtyTriangulation = false;
    meshValid = true;
}

bool CSGMeshCGAL::newTriangulate()
{
    if (!ensureTriangulation())
        return false;

    computeDecalRemap();
    return true;
}

void CSGMeshCGAL::weldMesh(bool positionOnly)
{
    if (vertices.empty())
        return;

    std::vector<CSGVertex> uniqueVertices;
    uniqueVertices.reserve(vertices.size());
    std::vector<unsigned int> remap(vertices.size(), 0);

    const float epsilon = 1e-4f;

    for (size_t i = 0; i < vertices.size(); ++i)
    {
        const CSGVertex& current = vertices[i];
        size_t foundIndex = uniqueVertices.size();
        for (size_t candidate = 0; candidate < uniqueVertices.size(); ++candidate)
        {
            const CSGVertex& existing = uniqueVertices[candidate];
            if ((current.pos - existing.pos).squaredLength() > epsilon * epsilon)
                continue;

            if (positionOnly || (current.normal - existing.normal).squaredLength() <= epsilon * epsilon)
            {
                if (positionOnly || (current.uv - existing.uv).squaredLength() <= epsilon * epsilon)
                {
                    if (positionOnly || current.color == existing.color)
                    {
                        foundIndex = candidate;
                        break;
                    }
                }
            }
        }

        if (foundIndex == uniqueVertices.size())
        {
            uniqueVertices.push_back(current);
        }
        remap[i] = static_cast<unsigned int>(foundIndex);
    }

    for (unsigned int& index : indices)
    {
        if (index < remap.size())
            index = remap[index];
    }

    vertices.swap(uniqueVertices);
    computeExtentsFromVertices();
}

std::string CSGMeshCGAL::getBRepBinaryString() const
{
    CSGMeshCGAL& self = const_cast<CSGMeshCGAL&>(*this);
    if (!self.ensureTriangulation())
        return std::string();

    const uint32_t version = 1;
    const uint32_t vertexCount = static_cast<uint32_t>(surfaceMesh.number_of_vertices());
    const uint32_t indexCount = static_cast<uint32_t>(indices.size());

    std::ostringstream stream(std::ios::binary);
    const char magic[4] = { 'C', 'G', 'S', 'M' };
    stream.write(magic, sizeof(magic));
    stream.write(reinterpret_cast<const char*>(&version), sizeof(version));
    stream.write(reinterpret_cast<const char*>(&vertexCount), sizeof(vertexCount));
    stream.write(reinterpret_cast<const char*>(&indexCount), sizeof(indexCount));

    for (SurfaceMesh::Vertex_index vertex : surfaceMesh.vertices())
    {
        const Kernel::Point_3& point = surfaceMesh.point(vertex);
        const float x = static_cast<float>(point.x());
        const float y = static_cast<float>(point.y());
        const float z = static_cast<float>(point.z());
        stream.write(reinterpret_cast<const char*>(&x), sizeof(float));
        stream.write(reinterpret_cast<const char*>(&y), sizeof(float));
        stream.write(reinterpret_cast<const char*>(&z), sizeof(float));
        const G3D::Color4uint8 color = (vertex.idx() < vertexColorBuffer.size()) ? vertexColorBuffer[vertex.idx()] : uniformColor;
        stream.write(reinterpret_cast<const char*>(&color), sizeof(G3D::Color4uint8));
    }

    if (!indices.empty())
        stream.write(reinterpret_cast<const char*>(&indices[0]), sizeof(unsigned int) * indices.size());

    return stream.str();
}

void CSGMeshCGAL::setBRepFromBinaryString(const std::string& str)
{
    vertices.clear();
    indices.clear();
    surfaceMesh.clear();
    vertexColorBuffer.clear();
    meshValid = false;
    dirtyTriangulation = true;

    if (str.empty())
        return;

    std::istringstream stream(str, std::ios::binary);

    char magic[4];
    stream.read(magic, sizeof(magic));
    if (!stream || std::string(magic, sizeof(magic)) != std::string("CGSM", 4))
        return;

    uint32_t version = 0;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;

    stream.read(reinterpret_cast<char*>(&version), sizeof(version));
    stream.read(reinterpret_cast<char*>(&vertexCount), sizeof(vertexCount));
    stream.read(reinterpret_cast<char*>(&indexCount), sizeof(indexCount));

    if (!stream || version != 1)
        return;

    std::vector<Kernel::Point_3> points;
    std::vector<G3D::Color4uint8> colors;
    points.reserve(vertexCount);
    colors.reserve(vertexCount);

    for (uint32_t i = 0; i < vertexCount; ++i)
    {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        stream.read(reinterpret_cast<char*>(&x), sizeof(float));
        stream.read(reinterpret_cast<char*>(&y), sizeof(float));
        stream.read(reinterpret_cast<char*>(&z), sizeof(float));
        G3D::Color4uint8 color(255, 255, 255, 255);
        stream.read(reinterpret_cast<char*>(&color), sizeof(G3D::Color4uint8));
        if (!stream)
            return;
        points.push_back(Kernel::Point_3(x, y, z));
        colors.push_back(color);
    }

    std::vector<unsigned int> localIndices(indexCount, 0);
    if (indexCount > 0)
        stream.read(reinterpret_cast<char*>(&localIndices[0]), sizeof(unsigned int) * indexCount);
    if (!stream)
        return;

    surfaceMesh.clear();
    vertexColorBuffer.clear();

    std::vector<SurfaceMesh::Vertex_index> handles(points.size(), SurfaceMesh::null_vertex());
    for (size_t i = 0; i < points.size(); ++i)
    {
        handles[i] = surfaceMesh.add_vertex(points[i]);
    }

    for (size_t i = 0; i + 2 < localIndices.size(); i += 3)
    {
        const unsigned int ia = localIndices[i];
        const unsigned int ib = localIndices[i + 1];
        const unsigned int ic = localIndices[i + 2];
        if (ia >= handles.size() || ib >= handles.size() || ic >= handles.size())
            continue;
        surfaceMesh.add_face(handles[ia], handles[ib], handles[ic]);
    }

    surfaceMesh.collect_garbage();

    vertexColorBuffer.assign(surfaceMesh.number_of_vertices(), uniformColor);
    for (size_t i = 0; i < handles.size(); ++i)
    {
        const SurfaceMesh::Vertex_index handle = handles[i];
        if (handle != SurfaceMesh::null_vertex() && i < colors.size())
            vertexColorBuffer[handle.idx()] = colors[i];
    }
    if (!vertexColorBuffer.empty())
        uniformColor = vertexColorBuffer.front();

    meshValid = surfaceMesh.number_of_faces() > 0;
    dirtyTriangulation = true;

    newTriangulate();
}

void CSGMeshCGAL::buildBRep()
{
    surfaceMesh.clear();
    meshValid = false;

    if (vertices.empty() || indices.size() < 3)
    {
        invalidateTriangulation();
        return;
    }

    std::vector<SurfaceMesh::Vertex_index> handles(vertices.size(), SurfaceMesh::null_vertex());
    for (size_t i = 0; i < vertices.size(); ++i)
    {
        if (!isFiniteVector(vertices[i].pos))
            continue;
        handles[i] = surfaceMesh.add_vertex(Kernel::Point_3(vertices[i].pos.x, vertices[i].pos.y, vertices[i].pos.z));
    }

    for (size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        const unsigned int ia = indices[i];
        const unsigned int ib = indices[i + 1];
        const unsigned int ic = indices[i + 2];

        if (ia >= handles.size() || ib >= handles.size() || ic >= handles.size())
            continue;

        const SurfaceMesh::Vertex_index ha = handles[ia];
        const SurfaceMesh::Vertex_index hb = handles[ib];
        const SurfaceMesh::Vertex_index hc = handles[ic];

        if (ha == SurfaceMesh::null_vertex() || hb == SurfaceMesh::null_vertex() || hc == SurfaceMesh::null_vertex())
            continue;

        surfaceMesh.add_face(ha, hb, hc);
    }

    PMP::remove_degenerate_faces(surfaceMesh);
    PMP::remove_isolated_vertices(surfaceMesh);
    PMP::orient_to_bound_a_volume(surfaceMesh);
    surfaceMesh.collect_garbage();

    meshValid = surfaceMesh.number_of_faces() > 0;
    vertexColorBuffer.assign(surfaceMesh.number_of_vertices(), uniformColor);
    for (size_t i = 0; i < handles.size(); ++i)
    {
        const SurfaceMesh::Vertex_index handle = handles[i];
        if (handle != SurfaceMesh::null_vertex() && i < vertices.size())
            vertexColorBuffer[handle.idx()] = vertices[i].color;
    }
    if (!vertexColorBuffer.empty())
        uniformColor = vertexColorBuffer.front();

    invalidateTriangulation();
}

bool CSGMeshCGAL::performBooleanOperation(const CSGMeshCGAL& lhs, const CSGMeshCGAL& rhs, BooleanOperation operation)
{
    if (!lhs.isValid() || !rhs.isValid())
        return false;

    SurfaceMesh meshA = lhs.surfaceMesh;
    SurfaceMesh meshB = rhs.surfaceMesh;

    PMP::triangulate_faces(meshA);
    PMP::triangulate_faces(meshB);
    PMP::remove_degenerate_faces(meshA);
    PMP::remove_degenerate_faces(meshB);
    PMP::remove_isolated_vertices(meshA);
    PMP::remove_isolated_vertices(meshB);
    PMP::orient_to_bound_a_volume(meshA);
    PMP::orient_to_bound_a_volume(meshB);

    SurfaceMesh result;
    bool success = false;

    try
    {
        switch (operation)
        {
        case BooleanOperation::Union:
            success = PMP::corefine_and_compute_union(meshA, meshB, result);
            break;
        case BooleanOperation::Intersection:
            success = PMP::corefine_and_compute_intersection(meshA, meshB, result);
            break;
        case BooleanOperation::Difference:
            success = PMP::corefine_and_compute_difference(meshA, meshB, result);
            break;
        }
    }
    catch (...)
    {
        success = false;
    }

    if (!success)
        return false;

    PMP::remove_degenerate_faces(result);
    PMP::remove_isolated_vertices(result);
    PMP::triangulate_faces(result);
    PMP::orient_to_bound_a_volume(result);
    result.collect_garbage();

    if (result.is_empty() || result.number_of_faces() == 0)
        return false;

    surfaceMesh = std::move(result);
    meshValid = true;
    uniformColor = lhs.uniformColor;
    resetColorBuffer(uniformColor);
    invalidateTriangulation();
    return true;
}

bool CSGMeshCGAL::unionMesh(const CSGMesh* a, const CSGMesh* b)
{
    const CSGMeshCGAL* meshA = dynamic_cast<const CSGMeshCGAL*>(a);
    const CSGMeshCGAL* meshB = dynamic_cast<const CSGMeshCGAL*>(b);

    if (!meshA || !meshB)
        return false;

    return performBooleanOperation(*meshA, *meshB, BooleanOperation::Union);
}

bool CSGMeshCGAL::intersectMesh(const CSGMesh* a, const CSGMesh* b)
{
    const CSGMeshCGAL* meshA = dynamic_cast<const CSGMeshCGAL*>(a);
    const CSGMeshCGAL* meshB = dynamic_cast<const CSGMeshCGAL*>(b);

    if (!meshA || !meshB)
        return false;

    return performBooleanOperation(*meshA, *meshB, BooleanOperation::Intersection);
}

bool CSGMeshCGAL::subractMesh(const CSGMesh* a, const CSGMesh* b)
{
    const CSGMeshCGAL* meshA = dynamic_cast<const CSGMeshCGAL*>(a);
    const CSGMeshCGAL* meshB = dynamic_cast<const CSGMeshCGAL*>(b);

    if (!meshA || !meshB)
        return false;

    return performBooleanOperation(*meshA, *meshB, BooleanOperation::Difference);
}

size_t CSGMeshCGAL::clusterVertices(float resolution)
{
    (void)resolution;
    weldMesh(true);
    return vertices.size();
}

bool CSGMeshCGAL::makeHalfEdges(std::vector<int>& vertexEdges)
{
    vertexEdges.assign(vertices.size(), -1);
    return true;
}

G3D::Vector3 CSGMeshCGAL::extentsCenter()
{
    if (!ensureTriangulation())
        return G3D::Vector3::zero();
    return extents.center();
}

G3D::Vector3 CSGMeshCGAL::extentsSize()
{
    if (!ensureTriangulation())
        return G3D::Vector3::zero();
    return extents.size();
}

CSGMesh* CSGMeshFactoryCGAL::createMesh() const
{
    return new CSGMeshCGAL();
}

} // namespace Aya
