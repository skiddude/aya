#include "DataModel/GameBasicSettings.hpp"
#include "CoordinateFrame.hpp"
#include "Utility/StandardOut.hpp"

#include "Render/SceneManager.hpp"

#include "Render/VisualEngine.hpp"
#include "Render/RenderQueue.hpp"
#include "Render/Material.hpp"
#include "Render/ShaderManager.hpp"

#include "Render/SpatialHashedScene.hpp"
#include "Render/AdornRender.hpp"
#include "Render/VertexStreamer.hpp"

#include "Render/FastCluster.hpp"
#include "Render/Sky.hpp"
#include "Render/SSAO.hpp"
#include "Render/EnvMap.hpp"
#include "Render/ScreenSpaceEffect.hpp"
#include "Render/TextureManager.hpp"

#include "Core/Device.hpp"
#include "Core/Framebuffer.hpp"

#include "Base/FrameRateManager.hpp"
#include "Base/RenderStats.hpp"

#include "Render/EmitterShared.hpp"

#include "Profiler.hpp"

LOGGROUP(Graphics)

FASTFLAGVARIABLE(EnableFXAA, false)

namespace Aya
{
namespace Graphics
{

static void renderObjectsImpl(DeviceContext* context, const RenderQueueGroup& group, RenderPassStats& stats, const char* dbgname)
{
    float matrixData[256 * 12];

    const Technique* cachedTechnique = NULL;
    const void* cachedMatrixData = NULL;

    PIX_SCOPE(context, "Render: %s", dbgname);

    for (size_t i = 0; i < group.size(); ++i)
    {
        const RenderOperation& rop = group[i];

        if (cachedTechnique != rop.technique && (rop.technique != NULL))
        {
            cachedTechnique = rop.technique;
            cachedMatrixData = NULL;

            rop.technique->apply(context);

            stats.passChanges++;
        }

        if (rop.renderable)
        {
            unsigned int matrixCount = rop.renderable->getWorldTransforms4x3(matrixData, ARRAYSIZE(matrixData) / 12, &cachedMatrixData);

            if (matrixCount)
                context->setWorldTransforms4x3(matrixData, matrixCount);
        }

        context->draw(*rop.geometry);

        stats.batches++;
        stats.vertices += rop.geometry->getIndexRangeEnd() - rop.geometry->getIndexRangeBegin();
        stats.faces += rop.geometry->getCount() / 3;
    }
}

static void renderObjects(
    DeviceContext* context, RenderQueueGroup& group, RenderQueueGroup::SortMode sortMode, RenderPassStats& stats, const char* dbgname)
{
    if (group.size() == 0)
        return;

    AYAPROFILER_SCOPE("Render", "$Pass");
    AYAPROFILER_LABELF("Render", "%s (%d)", dbgname, int(group.size()));

    AYAPROFILER_SCOPE("GPU", "$Pass");
    AYAPROFILER_LABELF("GPU", "%s (%d)", dbgname, int(group.size()));

    {
        AYAPROFILER_SCOPE("Render", "sort");

        group.sort(sortMode);
    }

    {
        AYAPROFILER_SCOPE("Render", "draw");

        renderObjectsImpl(context, group, stats, dbgname);
    }
}

static shared_ptr<Geometry> createStencilShadowTriangle(Device* device)
{
    std::vector<VertexLayout::Element> elements;
    elements.push_back(VertexLayout::Element(0, 0, VertexLayout::Format_Float3, VertexLayout::Semantic_Position));

    shared_ptr<VertexLayout> layout = device->createVertexLayout(elements);

    shared_ptr<VertexBuffer> vb = device->createVertexBuffer(sizeof(Vector3), 3, GeometryBuffer::Usage_Static);

    Vector3 vertices[] = {
        Vector3(-1, -1, 0),
        Vector3(-1, +3, 0),
        Vector3(+3, -1, 0),
    };

    vb->upload(0, vertices, sizeof(vertices));

    return device->createGeometry(layout, vb, shared_ptr<IndexBuffer>());
}

static shared_ptr<Geometry> createFullscreenTriangle(Device* device)
{
    std::vector<VertexLayout::Element> elements;
    elements.push_back(VertexLayout::Element(0, 0, VertexLayout::Format_Float3, VertexLayout::Semantic_Position));

    shared_ptr<VertexLayout> layout = device->createVertexLayout(elements);

    shared_ptr<VertexBuffer> vb = device->createVertexBuffer(sizeof(Vector3), 3, GeometryBuffer::Usage_Static);

    Vector3 vertices[] = {
        Vector3(-1, -1, 0),
        Vector3(-1, +3, 0),
        Vector3(+3, -1, 0),
    };

    vb->upload(0, vertices, sizeof(vertices));

    return device->createGeometry(layout, vb, shared_ptr<IndexBuffer>());
}

class Glow
{
public:
    Glow(VisualEngine* visualEngine)
        : visualEngine(visualEngine)
        , blurError(false)
        , glowStrength(2.2f)
    {
    }

    bool valid()
    {
        return data.get() != NULL;
    }
    float getGlowIntensity()
    {
        return glowStrength;
    }

    void update(unsigned width, unsigned height, bool gBufferExists)
    {
        if (data && (!gBufferExists))
        {
            data.reset();
        }
        else
        {
            unsigned widthOver4 = width / 4;
            unsigned heightOver4 = height / 4;

            if (!blurError && gBufferExists && (!data.get() || data->width != widthOver4 || data->height != heightOver4))
            {
                try
                {
                    data.reset(createData(widthOver4, heightOver4));
                }
                catch (const Aya::base_exception&)
                {
                    blurError = true;
                    data.reset();
                }
            }
        }
    }

    void computeBlur(DeviceContext* context, Texture* glowIntensity)
    {
        if (!data)
            return;

        PIX_SCOPE(context, "GlowComputeBlur");
        AYAPROFILER_SCOPE("Render", "glowCompute");
        AYAPROFILER_SCOPE("GPU", "glowCompute");

        context->bindFramebuffer(data->quarterFB[0].get());

        if (ScreenSpaceEffect::renderFullscreenBegin(context, visualEngine, "DownSample4x4VS", "DownSample4x4GlowFS", BlendState::Mode_None,
                data->quarterFB[0]->getWidth(), data->quarterFB[0]->getHeight()))
        {
            context->bindTexture(0, glowIntensity, SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));
            ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
        }

        ScreenSpaceEffect::renderBlur(context, visualEngine, data->quarterFB[0].get(), data->quarterFB[1].get(), data->quarterBuffers[0].get(),
            data->quarterBuffers[1].get(), 3);
    }

    void applyBlur(DeviceContext* context, Framebuffer* target)
    {
        if (!data)
            return;

        PIX_SCOPE(context, "ApplyBlur");
        AYAPROFILER_SCOPE("Render", "glowApply");
        AYAPROFILER_SCOPE("GPU", "glowApply");

        if (ShaderProgram* program = ScreenSpaceEffect::renderFullscreenBegin(context, visualEngine, "PassThroughVS", "GlowApplyFS",
                BlendState::Mode_PremultipliedAlphaBlend, target->getWidth(), target->getHeight()))
        {
            float params[4] = {glowStrength, 0, 0, 0};
            context->setConstant(program->getConstantHandle("Params1"), &params[0], 1);
            context->bindTexture(0, data->quarterBuffers[0].get(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));

            ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
        }
    }

private:
    struct Data
    {
        shared_ptr<Texture> quarterBuffers[2];
        shared_ptr<Framebuffer> quarterFB[2];
        unsigned width;
        unsigned height;
    };

    Data* createData(unsigned width, unsigned height)
    {
        Data* data = new Data();

        Device* device = visualEngine->getDevice();

        data->quarterBuffers[0] = device->createTexture(Texture::Type_2D, Texture::Format_RGBA8, width, height, 1, 1, Texture::Usage_Renderbuffer);
        data->quarterFB[0] = device->createFramebuffer(data->quarterBuffers[0]->getRenderbuffer(0, 0));

        data->quarterBuffers[1] = device->createTexture(Texture::Type_2D, Texture::Format_RGBA8, width, height, 1, 1, Texture::Usage_Renderbuffer);
        data->quarterFB[1] = device->createFramebuffer(data->quarterBuffers[1]->getRenderbuffer(0, 0));

        data->width = width;
        data->height = height;

        return data;
    }

    float glowStrength;
    bool blurError;
    scoped_ptr<Data> data;
    VisualEngine* visualEngine;
};


class FXAA
{
public:
    FXAA(VisualEngine* visualEngine)
        : visualEngine(visualEngine)
        , fxaaError(false)
    {
    }

    bool valid()
    {
        return data != NULL;
    }

    void update(unsigned width, unsigned height)
    {
        if (!fxaaError && (!data.get() || data->width != width || data->height != height))
        {
            try
            {
                data.reset(createData(width, height));
            }
            catch (const Aya::base_exception&)
            {
                fxaaError = true;
                data.reset();
            }
        }
    }

    void render(DeviceContext* context, Framebuffer* bufferToFXAA, Texture* inTexture)
    {
        if (!data)
            return;

        ScreenSpaceEffect::renderFXAA(context, visualEngine, bufferToFXAA, data->intermediateFB[0].get(), inTexture, data->intermediateTex[0].get());
    }

    void render(DeviceContext* context, Framebuffer* bufferToFXAA)
    {
        if (!data)
            return;
        context->copyFramebuffer(bufferToFXAA, data->intermediateTex[0].get(), 0, 0);
        ScreenSpaceEffect::renderFXAA(
            context, visualEngine, bufferToFXAA, data->intermediateFB[1].get(), data->intermediateTex[0].get(), data->intermediateTex[1].get());
    }

private:
    struct Data
    {
        shared_ptr<Texture> intermediateTex[2];
        shared_ptr<Framebuffer> intermediateFB[2];
        unsigned width;
        unsigned height;
    };

    Data* createData(unsigned width, unsigned height)
    {
        Data* data = new Data();

        Device* device = visualEngine->getDevice();

        data->intermediateTex[0] = device->createTexture(Texture::Type_2D, Texture::Format_RGBA8, width, height, 1, 1, Texture::Usage_Renderbuffer);
        data->intermediateFB[0] = device->createFramebuffer(data->intermediateTex[0]->getRenderbuffer(0, 0));

        data->intermediateTex[1] = device->createTexture(Texture::Type_2D, Texture::Format_RGBA8, width, height, 1, 1, Texture::Usage_Renderbuffer);
        data->intermediateFB[1] = device->createFramebuffer(data->intermediateTex[1]->getRenderbuffer(0, 0));

        data->width = width;
        data->height = height;

        return data;
    }

    scoped_ptr<Data> data;
    bool fxaaError;
    VisualEngine* visualEngine;
};

class Blur
{
public:
    Blur(VisualEngine* visualEngine)
        : visualEngine(visualEngine)
        , blurError(false)
        , blurStrentgh(0)
    {
    }

    bool valid()
    {
        return data != NULL;
    }

    void update(unsigned width, unsigned height, const SceneManager::PostProcessSettings& pps)
    {
        blurStrentgh = pps.blurIntensity;

        if (data && !blurNeeded())
        {
            data.reset();
        }
        else
        {
            if (!blurError && blurNeeded() && (!data.get() || data->width != width || data->height != height))
            {
                try
                {
                    data.reset(createData(width, height));
                }
                catch (const Aya::base_exception&)
                {
                    blurError = true;
                    data.reset();
                }
            }
        }
    }

    void render(DeviceContext* context, Framebuffer* bufferToBlur, Texture* inTexture)
    {
        if (!data)
            return;

        ScreenSpaceEffect::renderBlur(
            context, visualEngine, bufferToBlur, data->intermediateFB[0].get(), inTexture, data->intermediateTex[0].get(), blurStrentgh);
    }

    void render(DeviceContext* context, Framebuffer* bufferToBlur)
    {
        if (!data)
            return;
        context->copyFramebuffer(bufferToBlur, data->intermediateTex[0].get(), 0, 0);
        ScreenSpaceEffect::renderBlur(context, visualEngine, bufferToBlur, data->intermediateFB[1].get(), data->intermediateTex[0].get(),
            data->intermediateTex[1].get(), blurStrentgh);
    }

private:
    struct Data
    {
        shared_ptr<Texture> intermediateTex[2];
        shared_ptr<Framebuffer> intermediateFB[2];
        unsigned width;
        unsigned height;
    };

    Data* createData(unsigned width, unsigned height)
    {
        Data* data = new Data();

        Device* device = visualEngine->getDevice();

        data->intermediateTex[0] = device->createTexture(Texture::Type_2D, Texture::Format_RGBA8, width, height, 1, 1, Texture::Usage_Renderbuffer);
        data->intermediateFB[0] = device->createFramebuffer(data->intermediateTex[0]->getRenderbuffer(0, 0));

        data->intermediateTex[1] = device->createTexture(Texture::Type_2D, Texture::Format_RGBA8, width, height, 1, 1, Texture::Usage_Renderbuffer);
        data->intermediateFB[1] = device->createFramebuffer(data->intermediateTex[1]->getRenderbuffer(0, 0));

        data->width = width;
        data->height = height;

        return data;
    }

    bool blurNeeded()
    {
        return blurStrentgh > FLT_EPSILON;
    }

    scoped_ptr<Data> data;
    bool blurError;
    VisualEngine* visualEngine;

    float blurStrentgh;
};

class ImageProcess
{
public:
    ImageProcess(VisualEngine* visualEngine)
        : visualEngine(visualEngine)
        , grayscaleLevel(0)
        , brightness(0)
        , contrast(0)
        , tintColor(Color3(1, 1, 1))
        , error(false)
    {
    }

    bool valid()
    {
        return data != NULL;
    }

    void update(unsigned width, unsigned height, const SceneManager::PostProcessSettings& pps)
    {
        brightness = pps.brightness;
        contrast = pps.contrast;
        grayscaleLevel = pps.grayscaleLevel;
        tintColor = pps.tintColor;

        if (data && !processingNeeded())
        {
            data.reset();
        }
        else
        {
            if (!error && processingNeeded() && (!data.get() || data->width != width || data->height != height))
            {
                try
                {
                    data.reset(createData(width, height));
                }
                catch (const Aya::base_exception&)
                {
                    error = true;
                    data.reset();
                }
            }
        }
    }

    void render(DeviceContext* context, Framebuffer* target)
    {
        if (!data)
            return;

        AYAPROFILER_SCOPE("Render", "imgProcRender");
        AYAPROFILER_SCOPE("GPU", "imgProcRender");

        context->copyFramebuffer(target, data->intermediateTex.get(), 0, 0);

        if (ShaderProgram* program = ScreenSpaceEffect::renderFullscreenBegin(context, visualEngine, "PassThroughVS", "ImageProcessFS",
                BlendState::Mode_PremultipliedAlphaBlend, target->getWidth(), target->getHeight()))
        {
            float params1[4] = {brightness, contrast + 1, grayscaleLevel, 0};
            float params2[4] = {tintColor.r, tintColor.g, tintColor.b, 0};
            context->setConstant(program->getConstantHandle("Params1"), &params1[0], 1);
            context->setConstant(program->getConstantHandle("Params2"), &params2[0], 1);
            context->bindTexture(0, data->intermediateTex.get(), SamplerState(SamplerState::Filter_Point, SamplerState::Address_Clamp));

            ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
        }
    }

private:
    struct Data
    {
        shared_ptr<Texture> intermediateTex;
        unsigned width;
        unsigned height;
    };

    Data* createData(unsigned width, unsigned height)
    {
        Data* data = new Data();

        Device* device = visualEngine->getDevice();

        data->intermediateTex = device->createTexture(Texture::Type_2D, Texture::Format_RGBA8, width, height, 1, 1, Texture::Usage_Static);

        data->width = width;
        data->height = height;

        return data;
    }

    bool processingNeeded()
    {
        static const Color3 noTint = Color3(1, 1, 1);
        return tintColor != noTint || brightness != 0 || contrast != 0 || grayscaleLevel != 0;
    }

    scoped_ptr<Data> data;
    bool error;
    VisualEngine* visualEngine;

    Color3 tintColor;
    float brightness;
    float contrast;
    float grayscaleLevel;
};


MSAA::MSAA(VisualEngine* visualEngine, unsigned int width, unsigned int height, unsigned int samples)
    : visualEngine(visualEngine)
    , samples(samples)
{
    Device* device = visualEngine->getDevice();

    shared_ptr<Renderbuffer> msaaColor = device->createRenderbuffer(Texture::Format_RGBA8, width, height, samples);
    shared_ptr<Renderbuffer> msaaDepth = device->createRenderbuffer(Texture::Format_D24S8, width, height, samples);

    msaaFB = device->createFramebuffer(msaaColor, msaaDepth);

    msaaResolved = device->createTexture(Texture::Type_2D, Texture::Format_RGBA8, width, height, 1, 1, Texture::Usage_Renderbuffer);
    msaaResolvedFB = device->createFramebuffer(msaaResolved->getRenderbuffer(0, 0));
}

Texture* MSAA::getResolved() const
{
    return msaaResolved.get();
}

void MSAA::renderResolve(DeviceContext* context)
{
    context->resolveFramebuffer(msaaFB.get(), msaaResolvedFB.get(), DeviceContext::Buffer_Color);
}

void MSAA::renderComposit(DeviceContext* context)
{
    const float clearColor[] = {0, 0, 0, 1};

    context->clearFramebuffer(DeviceContext::Buffer_Color | DeviceContext::Buffer_Depth | DeviceContext::Buffer_Stencil, clearColor, 1.f, 0);

    if (ScreenSpaceEffect::renderFullscreenBegin(
            context, visualEngine, "PassThroughVS", "PassThroughFS", BlendState::Mode_None, msaaFB->getWidth(), msaaFB->getHeight()))
    {
        context->bindTexture(0, msaaResolved.get(), SamplerState(SamplerState::Filter_Point, SamplerState::Address_Clamp));

        ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
    }
}

Framebuffer* MSAA::getFramebuffer() const
{
    return msaaFB.get();
}

unsigned int MSAA::getWidth() const
{
    return msaaFB->getWidth();
}

unsigned int MSAA::getHeight() const
{
    return msaaFB->getHeight();
}

unsigned int MSAA::getSamples() const
{
    return samples;
}
class ShadowMap
{
public:
    ShadowMap(VisualEngine* visualEngine, Texture::Format format, int size)
    {
        Device* device = visualEngine->getDevice();

        shadowMap[0] = device->createTexture(Texture::Type_2D, format, size, size, 1, 1, Texture::Usage_Renderbuffer);
        shadowMap[1] = device->createTexture(Texture::Type_2D, format, size, size, 1, 1, Texture::Usage_Renderbuffer);

        shadowMapFB[0] = device->createFramebuffer(shadowMap[0]->getRenderbuffer(0, 0));
        shadowMapFB[1] = device->createFramebuffer(shadowMap[1]->getRenderbuffer(0, 0));

        shared_ptr<Renderbuffer> shadowDepth = device->createRenderbuffer(Texture::Format_D24S8, size, size, 1);

        shadowFB = device->createFramebuffer(shadowMap[0]->getRenderbuffer(0, 0), shadowDepth);
    }

    const shared_ptr<Texture>& getTexture() const
    {
        return shadowMap[0];
    }

    shared_ptr<Framebuffer> shadowFB;

    shared_ptr<Texture> shadowMap[2];
    shared_ptr<Framebuffer> shadowMapFB[2];
};

SceneManager::SceneManager(VisualEngine* visualEngine)
    : Resource(visualEngine->getDevice())
    , visualEngine(visualEngine)
    , sqMinPartDistance(FLT_MAX)
    , skyEnabled(true)
    , gbufferError(false)
    , msaaError(false)
    , postProcessSettings(PostProcessSettings())
{
    Device* device = visualEngine->getDevice();
    Framebuffer* framebuffer = device->getMainFramebuffer();

    spatialHashedScene.reset(new SpatialHashedScene());
    renderQueue.reset(new RenderQueue());

    fullscreenTriangle.reset(new GeometryBatch(createFullscreenTriangle(device), Geometry::Primitive_Triangles, 3, 3));

    sky.reset(new Sky(visualEngine));
    ssao.reset(new SSAO(visualEngine));
    glow.reset(new Glow(visualEngine));
    blur.reset(new Blur(visualEngine));

    if (FFlag::EnableFXAA)
        fxaa.reset(new FXAA(visualEngine));

    envMap.reset(new EnvMap(visualEngine));
    imageProcess.reset(new ImageProcess(visualEngine));
#ifndef AYA_SERVER
    msaa.reset(new MSAA(visualEngine, framebuffer->getWidth() * 2, framebuffer->getHeight() * 2, 4));
#endif

    shadowRenderQueue.reset(new RenderQueue());

    shadowMask = visualEngine->getTextureManager()->load(ContentId("ayaasset://textures/shadowmask.png"), TextureManager::Fallback_Black);

    try
    {
#if !defined(AYA_PLATFORM_IOS) && !defined(__ANDROID__)
        if (Aya::GameBasicSettings::singleton().getVirtualVersion() < GameBasicSettings::VERSION_2013)
        {
            shadowMapTexelSize = 0.05f; // Setting this too low breaks things
        }
        else
        {
            shadowMapTexelSize = 0.2f; // Setting this too low breaks things
        }

        shadowMapTexelSize = 0.2f;

        shadowMaps[0].reset(new ShadowMap(visualEngine, Texture::Format_RGBA8, 256));
        shadowMaps[1].reset(new ShadowMap(visualEngine, Texture::Format_RGBA8, 512));
        shadowMaps[2].reset(new ShadowMap(visualEngine, Texture::Format_RGBA8, 1024));
#else
        shadowMapTexelSize = 0.3f;

        shadowMaps[0].reset(new ShadowMap(visualEngine, Texture::Format_RGBA8, 128));
        shadowMaps[1].reset(new ShadowMap(visualEngine, Texture::Format_RGBA8, 256));
#endif
    }
    catch (Aya::base_exception& e)
    {
        FASTLOGS(FLog::Graphics, "Error creating shadow maps: %s", e.what());
    }

    // Create dummy GBuffer textures so that materials can have passes that reference them
    gbufferColor = shared_ptr<Texture>();
    gbufferDepth = shared_ptr<Texture>();
    shadowMapTexture = shared_ptr<Texture>();
}

SceneManager::~SceneManager() {}

void SceneManager::resetShadowMap()
{
    shadowRenderQueue.reset(new RenderQueue());

    try
    {
#if !defined(AYA_PLATFORM_IOS) && !defined(__ANDROID__)
        if (Aya::GameBasicSettings::singleton().getVirtualVersion() < GameBasicSettings::VERSION_2013)
        {
            shadowMapTexelSize = 0.05f; // Setting this too low breaks things
        }
        else
        {
            shadowMapTexelSize = 0.2f; // Setting this too low breaks things
        }

        shadowMaps[0].reset(new ShadowMap(visualEngine, Texture::Format_RGBA8, 256));
        shadowMaps[1].reset(new ShadowMap(visualEngine, Texture::Format_RGBA8, 512));
        shadowMaps[2].reset(new ShadowMap(visualEngine, Texture::Format_RGBA8, 1024));
#else
        shadowMapTexelSize = 0.3f;

        shadowMaps[0].reset(new ShadowMap(visualEngine, Texture::Format_RGBA8, 128));
        shadowMaps[1].reset(new ShadowMap(visualEngine, Texture::Format_RGBA8, 256));
#endif
    }
    catch (Aya::base_exception& e)
    {
        FASTLOGS(FLog::Graphics, "Error creating shadow maps: %s", e.what());
    }

    // Create dummy GBuffer textures so that materials can have passes that reference them
    gbufferColor = shared_ptr<Texture>();
    gbufferDepth = shared_ptr<Texture>();
    shadowMapTexture = shared_ptr<Texture>();
}

void SceneManager::computeMinimumSqDistance(const RenderCamera& camera)
{
    sqMinDistanceCenter = pointOfInterest;
    sqMinPartDistance = FLT_MAX;

    std::vector<CullableSceneNode*> nodes;
    spatialHashedScene->queryFrustumOrdered(nodes, camera, pointOfInterest, visualEngine->getFrameRateManager());

    // Required to call SceneManager::processSqPartDistance
    for (CullableSceneNode* node : nodes)
        node->updateIsCulledByFRM();
}

void SceneManager::renderScene(
    DeviceContext* context, Framebuffer* mainFramebuffer, const RenderCamera& camera, unsigned int viewWidth, unsigned int viewHeight)
{
    AYAPROFILER_SCOPE("GPU", "Scene");
    AYAPROFILER_SCOPE("Render", "Scene");

    renderBegin(context, camera, viewWidth, viewHeight);
    renderView(context, mainFramebuffer, camera, viewWidth, viewHeight);
    renderEnd(context);
}

void SceneManager::renderBegin(DeviceContext* context, const RenderCamera& camera, unsigned int viewWidth, unsigned int viewHeight)
{
    FrameRateManager* frm = visualEngine->getFrameRateManager();
    RenderStats* stats = visualEngine->getRenderStats();

    // updateMSAA(viewWidth, viewHeight);
    updateGBuffer(viewWidth, viewHeight);

    if (gbuffer)
    {
        gbufferColor.updateAllRefs(gbuffer->gbufferColor);
        gbufferDepth.updateAllRefs(gbuffer->gbufferDepth);
    }
    else
    {
        TextureManager* textureManager = visualEngine->getTextureManager();

        gbufferColor.updateAllRefs(textureManager->getFallbackTexture(TextureManager::Fallback_Black));
        gbufferDepth.updateAllRefs(textureManager->getFallbackTexture(TextureManager::Fallback_White));
    }

    // prepare UI
    visualEngine->getVertexStreamer()->renderPrepare();

    // prepare the sky
    // call this *before* rendering the envmap
    sky->prerender();

    // prepare the envmap
    envMap->update(context, curTimeOfDay);

    // Update SSAO
    ssao->update(frm->getSSAOLevel(), viewWidth, viewHeight);

    // Prepare for stats gathering
    stats->passTotal = stats->passScene = stats->passShadow = stats->passUI = stats->pass3DAdorns = RenderPassStats();

    sqMinDistanceCenter = pointOfInterest;
    sqMinPartDistance = FLT_MAX;

    // Get all visible ROPs
    renderNodes.clear();
    spatialHashedScene->queryFrustumOrdered(renderNodes, camera, pointOfInterest, frm);
    {
        AYAPROFILER_SCOPE("Render", "updateRenderQueue");

        for (CullableSceneNode* node : renderNodes)
            node->updateRenderQueue(*renderQueue, camera, RenderQueue::Pass_Default);
    }

    // Flush particle vertex buffer; it's being filled in particle emitter updateRenderQueue
    visualEngine->getEmitterSharedState()->flush();

    // Update glow targets if we need them
    {
        glow->update(viewWidth, viewHeight, true);
        globalShaderData.FadeDistance_GlowFactor.w = glow->valid() ? 1 : glow->getGlowIntensity();
    }

    if (FFlag::EnableFXAA)
        fxaa->update(viewWidth, viewHeight);

    blur->update(viewWidth, viewHeight, postProcessSettings);
    imageProcess->update(viewWidth, viewHeight, postProcessSettings);

    // Render shadow map
    ShadowMap* shadowMap = pickShadowMap(frm->GetQualityLevel());

    if (shadowMap)
    {
        renderShadowMap(context, shadowMap);

        shadowMapTexture.updateAllRefs(shadowMap->getTexture());
    }
    else
    {
        globalShaderData.OutlineBrightness_ShadowInfo.w = 0;

        shadowMapTexture.updateAllRefs(shared_ptr<Texture>());
    }
}

void SceneManager::renderView(
    DeviceContext* context, Framebuffer* mainFramebuffer, const RenderCamera& camera, unsigned int viewWidth, unsigned int viewHeight)
{
    RenderStats* stats = visualEngine->getRenderStats();

    // Set up main camera
    globalShaderData.setCamera(camera);

    context->updateGlobalConstants(&globalShaderData, sizeof(globalShaderData));

    if (gbuffer)
    {
        AYAPROFILER_SCOPE("Render", "Clear");
        AYAPROFILER_SCOPE("GPU", "Clear");

        const float clearDepthColor[] = {1, 1, 1, 1};

        context->bindFramebuffer(gbuffer->gbufferColorFB.get());
        context->clearFramebuffer(DeviceContext::Buffer_Color, &clearColor.r, 1.f, 0);

        context->bindFramebuffer(gbuffer->gbufferDepthFB.get());
        context->clearFramebuffer(DeviceContext::Buffer_Color, clearDepthColor, 1.f, 0);

        context->bindFramebuffer(gbuffer->gbufferFB.get());
        context->clearFramebuffer(DeviceContext::Buffer_Depth | DeviceContext::Buffer_Stencil, NULL, 1.f, 0);
    }


    {
        AYAPROFILER_SCOPE("Render", "Clear");
        AYAPROFILER_SCOPE("GPU", "Clear");

        context->bindFramebuffer(mainFramebuffer);
        context->clearFramebuffer(DeviceContext::Buffer_Color | DeviceContext::Buffer_Depth | DeviceContext::Buffer_Stencil, &clearColor.r, 1.f, 0);
    }

    renderObjects(context, renderQueue->getGroup(RenderQueue::Id_Opaque), RenderQueueGroup::Sort_Material, stats->passScene, "Id_Opaque");

    if (gbuffer)
        context->bindFramebuffer(gbuffer->gbufferColorFB.get());

    renderObjects(context, renderQueue->getGroup(RenderQueue::Id_OpaqueDecals), RenderQueueGroup::Sort_Material, stats->passScene, "Id_OpaqueDecals");

    if (gbuffer)
        context->bindFramebuffer(gbuffer->gbufferFB.get());

    ShadowValues shadowValues = unsetAndGetShadowValues(context);
    renderObjects(
        context, renderQueue->getGroup(RenderQueue::Id_OpaqueCasters), RenderQueueGroup::Sort_Material, stats->passScene, "Id_OpaqueCasters");
    restoreShadows(context, shadowValues);

    if (gbuffer)
        context->bindFramebuffer(gbuffer->gbufferColorFB.get());

    renderObjects(
        context, renderQueue->getGroup(RenderQueue::Id_TransparentDecals), RenderQueueGroup::Sort_Material, stats->passScene, "Id_TransparentDecals");

    if (gbuffer)
        context->bindFramebuffer(gbuffer->gbufferColorFB.get());

    if (gbuffer)
    {
        context->bindFramebuffer(gbuffer->mainFB.get());

        resolveGBuffer(context, gbuffer->gbufferColor.get());
    }

    // context->setRasterizerState(RasterizerState::Cull_None);

    context->setDepthState(DepthState(DepthState::Function_LessEqual, false, DepthState::Stencil_Increment));

    renderObjects(context, renderQueue->getGroup(RenderQueue::Id_Shadows), RenderQueueGroup::Sort_None, stats->passScene, "Id_Shadows");

    context->setDepthState(DepthState(DepthState::Function_LessEqual, false, DepthState::Stencil_Decrement));

    renderObjects(context, renderQueue->getGroup(RenderQueue::Id_Shadows), RenderQueueGroup::Sort_None, stats->passScene, "Id_Shadows");

    context->setDepthState(DepthState(DepthState::Function_LessEqual, false, DepthState::Stencil_IsNotZeroReplace));

    if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("GBufferResolveVS", "GBufferResolveFS"))
    {
        const float textureSize[] = {(float)mainFramebuffer->getWidth(), (float)mainFramebuffer->getHeight(), 1 / (float)mainFramebuffer->getWidth(),
            1 / (float)mainFramebuffer->getHeight()};

        context->bindProgram(program.get());
        context->bindTexture(0, visualEngine->getTextureManager()->getFallbackTexture(TextureManager::Fallback_StencilShadows).get(),
            SamplerState(SamplerState::Filter_Point, SamplerState::Address_Clamp));
        context->setConstant(program->getConstantHandle("TextureSize"), textureSize, 1);

        context->draw(getFullscreenTriangle());
    }

    context->setDepthState(DepthState(DepthState::Function_LessEqual, true, DepthState::Stencil_None));

    if (skyEnabled)
    {
        sky->render(context, camera);
    }

    renderObjects(context, renderQueue->getGroup(RenderQueue::Id_TransparentUnsorted), RenderQueueGroup::Sort_Material, stats->passScene,
        "Id_TransparentUnsorted");
    renderObjects(context, renderQueue->getGroup(RenderQueue::Id_Transparent), RenderQueueGroup::Sort_Distance, stats->passScene, "Id_Transparent");

    shadowValues = unsetAndGetShadowValues(context);
    renderObjects(context, renderQueue->getGroup(RenderQueue::Id_TransparentCasters), RenderQueueGroup::Sort_Distance, stats->passScene,
        "Id_TransparentCasters");
    restoreShadows(context, shadowValues);

    context->setDepthState(DepthState(DepthState::Function_None, true, DepthState::Stencil_None));

    if (glow->valid() && renderQueue->getFeatures() & RenderQueue::Features_Glow)
    {
        AYAPROFILER_SCOPE("Render", "Glow");
        AYAPROFILER_SCOPE("GPU", "Glow");
    }

    if (FFlag::EnableFXAA && fxaa->valid())
    {
        fxaa->render(context, gbuffer->mainFB.get(), gbuffer->mainColor.get());
    }

    if (blur->valid())
    {
        blur->render(context, mainFramebuffer);
    }

    if (imageProcess->valid())
    {
        imageProcess->render(context, mainFramebuffer);
    }

    visualEngine->getAdorn()->render(context, stats->pass3DAdorns);

    {
        AYAPROFILER_SCOPE("Render", "UI");
        AYAPROFILER_SCOPE("GPU", "UI");

        visualEngine->getVertexStreamer()->render3D(context, camera, stats->passUI);

        for (int renderIndex = 0; renderIndex <= Adorn::maximumZIndex; ++renderIndex)
        {
            visualEngine->getAdorn()->renderNoDepth(context, stats->pass3DAdorns, renderIndex);
            visualEngine->getVertexStreamer()->render3DNoDepth(context, camera, stats->pass3DAdorns, renderIndex);
        }
    }

    if (msaa)
    {
        AYAPROFILER_SCOPE("Render", "MSAA");
        AYAPROFILER_SCOPE("GPU", "MSAA");

        msaa->renderResolve(context);

        context->bindFramebuffer(mainFramebuffer);

        msaa->renderComposit(context);
    }

    {
        AYAPROFILER_SCOPE("Render", "UI");
        AYAPROFILER_SCOPE("GPU", "UI");

        visualEngine->getVertexStreamer()->render2D(context, viewWidth, viewHeight, stats->passUI);
    }

    if (ssao)
    {
        context->bindFramebuffer(mainFramebuffer);

        ssao->renderComposit(context);
    }
    else if (gbuffer)
    {
        context->bindFramebuffer(mainFramebuffer);

        resolveGBuffer(context, gbuffer->mainColor.get());
    }
}

void SceneManager::renderEnd(DeviceContext* context)
{
    RenderStats* stats = visualEngine->getRenderStats();

    renderQueue->clear();

    visualEngine->getVertexStreamer()->renderFinish();

    // Finalize stats
    stats->passTotal = stats->passScene + stats->passShadow + stats->passUI + stats->pass3DAdorns;
}

SceneManager::ShadowValues SceneManager::unsetAndGetShadowValues(DeviceContext* context)
{
    ShadowValues values = {globalShaderData.OutlineBrightness_ShadowInfo.w};

    globalShaderData.OutlineBrightness_ShadowInfo.w = 0;

    context->updateGlobalConstants(&globalShaderData, sizeof(globalShaderData));

    return values;
}

void SceneManager::restoreShadows(DeviceContext* context, const ShadowValues& shadowValues)
{
    globalShaderData.OutlineBrightness_ShadowInfo.w = shadowValues.intensity;

    context->updateGlobalConstants(&globalShaderData, sizeof(globalShaderData));
}

void SceneManager::setPointOfInterest(const Vector3& poi)
{
    pointOfInterest = poi;
}

void SceneManager::setSkyEnabled(bool value)
{
    skyEnabled = value;
}

void SceneManager::setClearColor(const Color4& value)
{
    clearColor = value;
}

void SceneManager::setFog(const Color3& color, float rangeBegin, float rangeEnd)
{
    globalShaderData.FogColor = Color4(color);
    globalShaderData.FogParams = Vector4(0, rangeBegin, rangeEnd, (rangeBegin != rangeEnd) ? 1 / (rangeEnd - rangeBegin) : 0);
}

void SceneManager::setLighting(const Color3& ambient, const Vector3& keyDirection, const Color3& keyColor, const Color3& fillColor)
{
    globalShaderData.AmbientColor = Color4(ambient);
    globalShaderData.Lamp0Color = Color4(keyColor);
    globalShaderData.Lamp0Dir = Vector4(keyDirection, 0);
    globalShaderData.Lamp1Color = Color4(fillColor);
}

void SceneManager::setBlurIntensity(float intensity)
{
    postProcessSettings.blurIntensity = intensity;
}

void SceneManager::setColorCorrection(float brightness, float contrast, float grayscaleLevel, const Color3& tintColor)
{
    postProcessSettings.brightness = brightness;
    postProcessSettings.contrast = contrast;
    postProcessSettings.grayscaleLevel = grayscaleLevel;
    postProcessSettings.tintColor = tintColor;
}

void SceneManager::setBloom(float intensity, float size, float threshold)
{
    postProcessSettings.bloomIntensity = intensity;
    postProcessSettings.bloomSize = size;
    postProcessSettings.bloomThreshold = threshold;
}

void SceneManager::updateMSAA(unsigned width, unsigned height)
{
    if (msaaError)
        return;
    if (device->getCaps().maxSamples <= 1)
        return;

    try
    {
        FrameRateManager* frm = visualEngine->getFrameRateManager();

        if (frm->getGBufferSetting())
        {
            unsigned int frmSamples = (frm->GetQualityLevel() >= 19) ? 8 : (frm->GetQualityLevel() >= 17) ? 4 : 2;
            unsigned int samples = std::min(device->getCaps().maxSamples, frmSamples);

            if (!msaa || msaa->getWidth() != width || msaa->getHeight() != height || msaa->getSamples() != samples)
            {
                msaa.reset();
                msaa.reset(new MSAA(visualEngine, width, height, samples));
            }
        }
        else
        {
            msaa.reset();
        }
    }
    catch (Aya::base_exception& e)
    {
        FASTLOGS(FLog::Graphics, "Error initializing MSAA: %s", e.what());

        msaa.reset();
        msaaError = true;
    }
}

void SceneManager::updateGBuffer(unsigned width, unsigned height)
{
    if (gbufferError)
    {
        gbuffer.reset();
        return;
    }

    FrameRateManager* frm = visualEngine->getFrameRateManager();
    if (!frm->getGBufferSetting())
    {
        // gbuffer is not required, kill it and bail out
        gbuffer.reset();
        return;
    }

    if (!gbuffer || (gbuffer->mainColor->getWidth() != width || gbuffer->mainColor->getHeight() != height))
    {
        gbuffer.reset();
        gbuffer.reset(new GBuffer());

        Device* device = visualEngine->getDevice();

        try
        {
            shared_ptr<Renderbuffer> mainDepth = device->createRenderbuffer(Texture::Format_D24S8, width, height, 4);

            gbuffer->mainColor = device->createTexture(Texture::Type_2D, Texture::Format_RGBA8, width, height, 1, 1, Texture::Usage_Renderbuffer);
            gbuffer->mainFB = device->createFramebuffer(gbuffer->mainColor->getRenderbuffer(0, 0), mainDepth);

            gbuffer->gbufferColor = device->createTexture(Texture::Type_2D, Texture::Format_RGBA8, width, height, 1, 1, Texture::Usage_Renderbuffer);
            gbuffer->gbufferColorFB = device->createFramebuffer(gbuffer->gbufferColor->getRenderbuffer(0, 0), mainDepth);

            gbuffer->gbufferDepth = device->createTexture(Texture::Type_2D, Texture::Format_RGBA8, width, height, 1, 1, Texture::Usage_Renderbuffer);
            gbuffer->gbufferDepthFB = device->createFramebuffer(gbuffer->gbufferDepth->getRenderbuffer(0, 0));

            std::vector<shared_ptr<Renderbuffer>> gbufferTextures;
            gbufferTextures.push_back(gbuffer->gbufferColor->getRenderbuffer(0, 0));
            gbufferTextures.push_back(gbuffer->gbufferDepth->getRenderbuffer(0, 0));

            gbuffer->gbufferFB = device->createFramebuffer(gbufferTextures, mainDepth);
        }
        catch (Aya::base_exception& e)
        {
            Aya::StandardOut::singleton()->printf(Aya::MESSAGE_INFO, "Error initializing GBuffer: %s", e.what());

            gbuffer.reset();
            gbufferError = true; // and do not try any further
        }
    }
}

void SceneManager::resolveGBuffer(DeviceContext* context, Texture* texture)
{
    AYAASSERT(gbuffer);
    PIX_SCOPE(context, "resolveGBuffer");
    AYAPROFILER_SCOPE("Render", "resolveGBuffer");
    AYAPROFILER_SCOPE("GPU", "resolveGBuffer");

    float width = texture->getWidth();
    float height = texture->getHeight();

    const float textureSize[] = {width, height, 1 / width, 1 / height};

    context->setRasterizerState(RasterizerState::Cull_None);
    context->setBlendState(BlendState::Mode_None);
    context->setDepthState(DepthState(DepthState::Function_Always, false));

    static const std::string vsName = "GBufferResolveVS";
    static const std::string fsName = "GBufferResolveFS";

    if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram(vsName, fsName))
    {
        context->bindProgram(program.get());
        context->bindTexture(0, texture, SamplerState(SamplerState::Filter_Point, SamplerState::Address_Clamp));
        context->setConstant(program->getConstantHandle("TextureSize"), textureSize, 1);

        context->draw(getFullscreenTriangle());
    }
}

void SceneManager::renderShadowMap(DeviceContext* context, ShadowMap* shadowMap)
{
    AYAPROFILER_SCOPE("Render", "renderShadowMap");
    AYAPROFILER_SCOPE("GPU", "ShadowMap");

    int shadowMapSize = shadowMap->getTexture()->getWidth();
    float shadowRadius = shadowMapSize * shadowMapTexelSize / 2.f;
    float shadowDepthRadius = 64.f;
    Vector3 shadowCenter = pointOfInterest;

    RenderCamera shadowCamera = getShadowCamera(shadowCenter, shadowMapSize, shadowRadius, shadowDepthRadius);

    renderNodes.clear();
    spatialHashedScene->querySphere(renderNodes, shadowCenter, std::max(shadowRadius, shadowDepthRadius), CullableSceneNode::Flags_ShadowCaster);

    {
        AYAPROFILER_SCOPE("Render", "updateRenderQueue");

        shadowRenderQueue->clear();

        for (CullableSceneNode* node : renderNodes)
            node->updateRenderQueue(*shadowRenderQueue, shadowCamera, RenderQueue::Pass_Shadows);
    }

    globalShaderData.setCamera(shadowCamera);

    float clipSign = visualEngine->getDevice()->getCaps().requiresRenderTargetFlipping ? 1 : -1;
    Matrix4 clipToUv = Matrix4::translation(0.5f, 0.5f, 0) * Matrix4::scale(0.5f, clipSign * 0.5f, 1);
    Matrix4 textureMatrix = clipToUv * shadowCamera.getViewProjectionMatrix();

    globalShaderData.ShadowMatrix0 = textureMatrix.row(0);
    globalShaderData.ShadowMatrix1 = textureMatrix.row(1);
    globalShaderData.ShadowMatrix2 = textureMatrix.row(2);

    context->updateGlobalConstants(&globalShaderData, sizeof(globalShaderData));

    const float clearColor[] = {1, 0, 0, 0};

    context->bindFramebuffer(shadowMap->shadowFB.get());
    context->clearFramebuffer(DeviceContext::Buffer_Color | DeviceContext::Buffer_Depth, clearColor, 1.f, 0);

    RenderQueueGroup& group = shadowRenderQueue->getGroup(RenderQueue::Id_OpaqueCasters);

    // don't render character shadows for under 2013.
    if (group.size() > 0 && Aya::GameBasicSettings::singleton().getVirtualVersion() < GameBasicSettings::VERSION_2013)
    {
        renderObjects(context, group, RenderQueueGroup::Sort_Material, visualEngine->getRenderStats()->passShadow, "Id_OpaqueCasters");
        blurShadowMap(context, shadowMap);
    }
}

RenderCamera SceneManager::getShadowCamera(const Vector3& center, int shadowMapSize, float shadowRadius, float shadowDepthRadius)
{
    RenderCamera shadowCamera;

    CoordinateFrame cframe;
    cframe.translation = pointOfInterest;
    cframe.lookAt(cframe.translation - globalShaderData.Lamp0Dir.xyz());

    Matrix4 proj = Matrix4::identity();

    proj[0][0] = 1.f / shadowRadius;
    proj[1][1] = 1.f / shadowRadius;
    proj[2][2] = 1.f / shadowDepthRadius;
    proj[2][3] = 0.5f;

    shadowCamera.setViewCFrame(cframe);
    shadowCamera.setProjectionMatrix(proj);

    // Round translation
    int roundThreshold = shadowMapSize / 2;

    Vector4 centerp = shadowCamera.getViewProjectionMatrix() * Vector4(0, 0, 0, 1);
    float centerX = floorf(centerp.x * roundThreshold) / roundThreshold;
    float centerY = floorf(centerp.y * roundThreshold) / roundThreshold;

    proj[0][3] = centerX - centerp.x;
    proj[1][3] = centerY - centerp.y;

    shadowCamera.setProjectionMatrix(proj);

    return shadowCamera;
}

void SceneManager::blurShadowMap(DeviceContext* context, ShadowMap* shadowMap)
{
    AYAPROFILER_SCOPE("Render", "Blur");
    AYAPROFILER_SCOPE("GPU", "Blur");

    int shadowMapSize = shadowMap->getTexture()->getWidth();

    context->bindFramebuffer(shadowMap->shadowMapFB[1].get());

    const shared_ptr<Texture>& dummyMask = visualEngine->getTextureManager()->getFallbackTexture(TextureManager::Fallback_White);

    if (ShaderProgram* program = ScreenSpaceEffect::renderFullscreenBegin(
            context, visualEngine, "PassThroughVS", "ShadowBlurFS", BlendState::Mode_None, shadowMapSize, shadowMapSize))
    {
        float params[] = {1.f / shadowMapSize, 0, 0, 0};
        context->setConstant(program->getConstantHandle("Params1"), params, 1);
        context->bindTexture(0, shadowMap->shadowMap[0].get(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));
        context->bindTexture(1, dummyMask.get(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));

        ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
    }

    context->bindFramebuffer(shadowMap->shadowMapFB[0].get());

    if (ShaderProgram* program = ScreenSpaceEffect::renderFullscreenBegin(
            context, visualEngine, "PassThroughVS", "ShadowBlurFS", BlendState::Mode_None, shadowMapSize, shadowMapSize))
    {
        float params[] = {0, 1.f / shadowMapSize, 0, 0};
        context->setConstant(program->getConstantHandle("Params1"), params, 1);
        context->bindTexture(0, shadowMap->shadowMap[1].get(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));
        context->bindTexture(1, shadowMask.getTexture().get(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));

        ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
    }
}

void SceneManager::onDeviceLost()
{
    gbuffer.reset(0);
}

ShadowMap* SceneManager::pickShadowMap(int qualityLevel) const
{
    if (shadowMaps[2] && qualityLevel >= 14)
        return shadowMaps[2].get();

    if (shadowMaps[1] && qualityLevel >= 6)
        return shadowMaps[1].get();

    return shadowMaps[0].get();
}

} // namespace Graphics
} // namespace Aya
