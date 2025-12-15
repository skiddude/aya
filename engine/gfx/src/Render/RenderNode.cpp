
#include "Render/RenderNode.hpp"

#include "Render/Material.hpp"

#include "Base/FrameRateManager.hpp"
#include "Render/VisualEngine.hpp"
#include "Render/Util.hpp"
#include "Render/VertexStreamer.hpp"
#include "Render/GeometryGenerator.hpp"
#include "Render/SceneManager.hpp"

#include "API/GL/DeviceGL.hpp"
#include "DataModel/BasicPartInstance.hpp"
#include "World/Geometry.hpp"
#include "Render/FastCluster.hpp"

namespace Aya
{
namespace Graphics
{

RenderEntity::RenderEntity(
    RenderNode* node, const GeometryBatch& geometry, const shared_ptr<Material>& material, RenderQueue::Id renderQueueId, unsigned char lodMask)
    : node(node)
    , renderQueueId(renderQueueId)
    , lodMask(lodMask)
    , geometry(geometry)
    , material(material)
{
    Device* dev = node->getVisualEngine()->getDevice();
}

RenderEntity::~RenderEntity() {}

#define STENCIL_SHADOW_DISTANCE 10000.0f

struct Edge
{
    GeometryGenerator::Vertex a, b;      // point v1 and v2
    std::vector<unsigned int> triangles; // we need to know what triangle faces these edges belong to

    bool operator==(const Edge& edge) const
    {
        // Compare vertex positions
        return (a.pos == edge.a.pos && b.pos == edge.b.pos) || (a.pos == edge.b.pos && b.pos == edge.a.pos);
    }
};

struct EdgeHash
{
    size_t operator()(const Edge& edge) const
    {
        size_t h1 = boost::hash<float>{}(edge.a.pos.x) ^ boost::hash<float>{}(edge.a.pos.y) ^ boost::hash<float>{}(edge.a.pos.z);
        size_t h2 = boost::hash<float>{}(edge.b.pos.x) ^ boost::hash<float>{}(edge.b.pos.y) ^ boost::hash<float>{}(edge.b.pos.z);
        return h1 ^ h2;
    }
};

struct Triangle
{
    GeometryGenerator::Vertex a, b, c;
};

void RenderEntity::computeStencilGeometry(RenderOperation& ope)
{
    // Snatch current geometry & buffers
    Geometry* geometry = ope.geometry->getGeometry();
    std::vector<shared_ptr<VertexBuffer>> vertexBuffers = geometry->getVertexBuffers();
    shared_ptr<VertexBuffer> vertexBuffer = vertexBuffers[0];
    shared_ptr<IndexBuffer> indexBuffer = geometry->getIndexBuffer();

    // Copy it so we can unlock the current vertex buffer as fast as possible
    GeometryGenerator::Vertex* iter = static_cast<GeometryGenerator::Vertex*>(vertexBuffer->lock());
    unsigned int vertexCount = vertexBuffer->getElementCount();

    GeometryGenerator::Vertex* buffer = new GeometryGenerator::Vertex[vertexCount];

    memcpy(buffer, iter, (sizeof(GeometryGenerator::Vertex) * vertexCount));

    // Free the current buffer that's being rendered
    vertexBuffer->unlock();

    // Store face and edge data
    std::vector<Triangle> faces;
    boost::unordered_map<Edge, std::vector<size_t>, EdgeHash> edges;

    // Reserve face data for reduced allocation
    faces.reserve(vertexCount / 3);

    Color4uint8 test = Color4uint8(Color4(0, 0, 0, 0.25f));
    // We can now do our work here
    for (int i = 0; i < vertexCount; i += 3)
    {
        faces.emplace_back(Triangle{buffer[i], buffer[i + 1], buffer[i + 2]});
    }

    // Reserve edge data
    // Every triangle has three edges
    edges.reserve(faces.size() * 3);

    // Fetch edges
    for (int i = 0; i < faces.size(); i++)
    {
        faces[i].a.color = test;
        faces[i].b.color = test;
        faces[i].c.color = test;
        Edge triEdges[3] = {{faces[i].a, faces[i].b}, {faces[i].b, faces[i].c}, {faces[i].c, faces[i].a}};

        for (int e = 0; e < 3; e++)
        {
            Edge& edge = triEdges[e];
            auto it = edges.find(edge);
            if (it != edges.end())
            {
                // We've already seen this edge, let's add to it's list of triangles
                it->second.emplace_back(static_cast<unsigned int>(i));
            }
            else
            {
                // New edge, add to the list.
                edges[edge] = {static_cast<unsigned int>(i)}; // why do I need to cast this?
            }
        }
    }

    // Silhouette edge detection

    GlobalShaderData shaderData = getNode()->getVisualEngine()->getSceneManager()->readGlobalShaderData();
    Vector3 lightDir = shaderData.Lamp0Dir.xyz() / shaderData.Lamp0Dir.xyz().length();

    std::vector<GeometryGenerator::Vertex> silhouetteVertices;

    for (const auto& pair : edges)
    {
        // Possible silhouette edges are shared between two triangle faces, one facing the light, and one that isn't
        const Edge& edge = pair.first;
        const std::vector<size_t>& triangles = pair.second;

        if (triangles.size() == 2)
        {
            // Get the first triangle face.
            Triangle firstFace = faces[triangles[0]];
            Triangle secondFace = faces[triangles[1]];

            // Calculate the normals of both faces.
            Vector3 fNormal = (firstFace.b.pos - firstFace.a.pos).cross(firstFace.c.pos - firstFace.a.pos);
            Vector3 sNormal = (secondFace.b.pos - secondFace.a.pos).cross(secondFace.c.pos - secondFace.a.pos);

            // Normalize
            fNormal = fNormal / fNormal.length();
            sNormal = sNormal / fNormal.length();

            bool isFrontFacing1 = fNormal.dot(lightDir) >= 0;
            bool isFrontFacing2 = sNormal.dot(lightDir) >= 0;

            if (isFrontFacing1 != isFrontFacing2)
            {
                // This is a silhouette edge.
                // Add the vertices to our vector.
                // Let's emit a quad
                GeometryGenerator::Vertex extrudedA, extrudedB;
                extrudedA = edge.a;
                extrudedB = edge.b;
                extrudedA.pos += (lightDir * STENCIL_SHADOW_DISTANCE);
                extrudedB.pos += (lightDir * STENCIL_SHADOW_DISTANCE);

                silhouetteVertices.push_back(edge.a);
                silhouetteVertices.push_back(edge.b);
                silhouetteVertices.push_back(extrudedA);
                silhouetteVertices.push_back(extrudedB);
            }
        }
    }

    // Create our own buffers
    Device* device = node->getVisualEngine()->getDevice();

    // Note: we can probably optimize this if we create a single buffer on initialization, and we keep on reusing it
    shared_ptr<VertexLayout>& layout = getNode()->getVisualEngine()->getFastClusterLayout();
    shared_ptr<VertexBuffer> vb =
        device->createVertexBuffer(sizeof(GeometryGenerator::Vertex), silhouetteVertices.size(), GeometryBuffer::Usage_Dynamic);

    vb->upload(0, silhouetteVertices.data(), (sizeof(GeometryGenerator::Vertex) * silhouetteVertices.size()));

    shared_ptr<Geometry> shadowGeometry = device->createGeometry(layout, vb, shared_ptr<IndexBuffer>());
    ope.stencil_geometry = new GeometryBatch(shadowGeometry, Geometry::Primitive_TriangleStrip, silhouetteVertices.size(), silhouetteVertices.size());

    // In theory, this isn't needed since vectors call their respective dTor functions
    faces.clear();
    edges.clear();

    delete[] buffer;
}

void RenderEntity::updateRenderQueue(RenderQueue& queue, const RenderCamera& camera, unsigned int lodIndex, RenderQueue::Pass pass)
{
    const shared_ptr<Material>& m = material;

    if (const Technique* technique = m->getBestTechnique(lodIndex, pass))
    {
        float distanceKey = (renderQueueId == RenderQueue::Id_Transparent) ? getViewDepth(camera) : 0.f;

        RenderOperation rop = {this, distanceKey, technique, &geometry};

        queue.getGroup(renderQueueId).push(rop);
    }
    else
    {
        float distanceKey = (renderQueueId == RenderQueue::Id_Transparent) ? getViewDepth(camera) : 0.f;

        RenderOperation rop = {this, distanceKey, NULL, &geometry};

        queue.getGroup(renderQueueId).push(rop);
    }
}

unsigned int RenderEntity::getWorldTransforms4x3(float* buffer, unsigned int maxTransforms, const void** cacheKey) const
{
    if (useCache(cacheKey, node))
        return 0;

    AYAASSERT(maxTransforms >= 1);

    const CoordinateFrame& cframe = node->getCoordinateFrame();

    memcpy(&buffer[0], cframe.rotation[0], sizeof(float) * 3);
    buffer[3] = cframe.translation.x;

    memcpy(&buffer[4], cframe.rotation[1], sizeof(float) * 3);
    buffer[7] = cframe.translation.y;

    memcpy(&buffer[8], cframe.rotation[2], sizeof(float) * 3);
    buffer[11] = cframe.translation.z;

    return 1;
}

float RenderEntity::getViewDepth(const RenderCamera& camera) const
{
    return computeViewDepth(camera, node->getCoordinateFrame().translation);
}

float RenderEntity::computeViewDepth(const RenderCamera& camera, const Vector3& position, float offset)
{
    float result = (position - camera.getPosition()).dot(camera.getDirection()) + offset;

    return Math::isNan(result) ? 0 : result;
}

RenderNode::RenderNode(VisualEngine* visualEngine, CullMode cullMode, unsigned int flags)
    : CullableSceneNode(visualEngine, cullMode, flags)
{
}

RenderNode::~RenderNode()
{
    // delete all entities
    for (size_t i = 0; i < entities.size(); ++i)
    {
        delete entities[i];
    }
}

void RenderNode::addEntity(RenderEntity* entity)
{
    AYAASSERT(entity);
    entities.push_back(entity);
}

void RenderNode::removeEntity(RenderEntity* entity)
{
    std::vector<RenderEntity*>::iterator it = std::find(entities.begin(), entities.end(), entity);
    AYAASSERT(it != entities.end());

    entities.erase(it);
}

void RenderNode::updateRenderQueue(RenderQueue& queue, const RenderCamera& camera, RenderQueue::Pass pass)
{
    if (updateIsCulledByFRM())
        return;

    FrameRateManager* frm = getVisualEngine()->getFrameRateManager();

    unsigned int lodIndex = frm->getGBufferSetting() ? 0 : getSqDistanceToFocus() < frm->getShadingSqDistance() ? 1 : 2;

    unsigned int lodMask = 1 << lodIndex;

    // Add all entities
    for (size_t i = 0; i < entities.size(); ++i)
    {
        if (entities[i]->getLodMask() & lodMask)
            entities[i]->updateRenderQueue(queue, camera, lodIndex, pass);
    }

    // Render bounding box
    if (getVisualEngine()->getSettings()->getDebugShowBoundingBoxes())
    {
        debugRenderBoundingBox();
    }
}

void RenderNode::debugRenderBoundingBox()
{
    VertexStreamer* vs = getVisualEngine()->getVertexStreamer();
    const Extents& b = getWorldBounds();

    if (!b.isNull())
    {
        static const int edges[12][2] = {{0, 2}, {2, 6}, {6, 4}, {4, 0}, {1, 3}, {3, 7}, {7, 5}, {5, 1}, {0, 1}, {2, 3}, {4, 5}, {6, 7}};

        for (int edge = 0; edge < 12; ++edge)
        {
            Vector3 e0 = b.getCorner(edges[edge][0]);
            Vector3 e1 = b.getCorner(edges[edge][1]);

            vs->line3d(e0.x, e0.y, e0.z, e1.x, e1.y, e1.z, Color3::white());
        }
    }
}

} // namespace Graphics
} // namespace Aya
