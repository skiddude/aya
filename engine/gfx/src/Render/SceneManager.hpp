#pragma once

#include "GlobalShaderData.hpp"
#include "Core/Resource.hpp"
#include "boost.hpp"

#include "DataModel/Lighting.hpp"

#include "TextureRef.hpp"

namespace Aya
{
namespace Graphics
{

class Sky;
class SpatialHashedScene;
class IndexBuffer;
class RenderCamera;
class RenderQueue;
class VisualEngine;
class Material;
class DeviceContext;
class CullableSceneNode;
class GeometryBatch;
class SSAO;
class Glow;
class FXAA;
class Blur;
class ImageProcess;
class Framebuffer;
class EnvMap;
class BlendState;
class ShaderProgram;
class ShadowMap;

class MSAA
{
public:
    MSAA(VisualEngine* visualEngine, unsigned int width, unsigned int height, unsigned int samples);
    Texture* getResolved() const;
    void renderResolve(DeviceContext* context);
    void renderComposit(DeviceContext* context);
    Framebuffer* getFramebuffer() const;
    unsigned int getWidth() const;
    unsigned int getHeight() const;
    unsigned int getSamples() const;

private:
    VisualEngine* visualEngine;
    shared_ptr<Framebuffer> msaaFB;
    shared_ptr<Texture> msaaResolved;
    shared_ptr<Framebuffer> msaaResolvedFB;
    unsigned int samples;
};

class SceneManager : Resource
{
public:
    struct PostProcessSettings
    {
        PostProcessSettings()
            : brightness(0)
            , contrast(0)
            , grayscaleLevel(0)
            , blurIntensity(0)
            , tintColor(Color3::white())
            , bloomIntensity(1)
            , bloomSize(24)
            , bloomThreshold(2)
        {
        }

        float brightness;
        float contrast;
        float grayscaleLevel;
        float blurIntensity;
        Color3 tintColor;
        float bloomIntensity;
        float bloomSize;
        float bloomThreshold;
    };

    SceneManager(VisualEngine* visualEngine);
    ~SceneManager();

    SpatialHashedScene* getSpatialHashedScene()
    {
        return spatialHashedScene.get();
    }

    Sky* getSky()
    {
        return sky.get();
    }
    EnvMap* getEnvMap()
    {
        return envMap.get();
    }
    SSAO* getSSAO()
    {
        return ssao.get();
    }

    const GlobalShaderData& readGlobalShaderData() const
    {
        return globalShaderData;
    }
    GlobalShaderData& writeGlobalShaderData()
    {
        return globalShaderData;
    }

    // set the center of the culling sphere used by the framerate manager.
    const Vector3& getPointOfInterest() const
    {
        return pointOfInterest;
    }
    void setPointOfInterest(const Vector3& poi);

    void processSqPartDistance(float distance)
    {
        sqMinPartDistance = std::min(sqMinPartDistance, distance);
    }

    float getMinumumSqPartDistance()
    {
        return sqMinPartDistance;
    }
    const Vector3& getMinumumSqDistanceCenter()
    {
        return sqMinDistanceCenter;
    }

    void computeMinimumSqDistance(const RenderCamera& camera);
    void resetShadowMap();

    void renderScene(
        DeviceContext* context, Framebuffer* mainFramebuffer, const RenderCamera& camera, unsigned int viewWidth, unsigned int viewHeight);

    void renderBegin(DeviceContext* context, const RenderCamera& camera, unsigned int viewWidth, unsigned int viewHeight);
    void renderView(
        DeviceContext* context, Framebuffer* mainFramebuffer, const RenderCamera& camera, unsigned int viewWidth, unsigned int viewHeight);
    void renderEnd(DeviceContext* context);

    void setSkyEnabled(bool value);
    void setClearColor(const Color4& value);
    void setFog(const Color3& color, float rangeBegin, float rangeEnd);
    void setLighting(const Color3& ambient, const Vector3& keyDirection, const Color3& keyColor, const Color3& fillColor);
    void trackLightingTimeOfDay(double sec)
    {
        curTimeOfDay = sec;
    }

    void setBlurIntensity(float intensity);
    void setColorCorrection(float brightness, float contrast, float grayscaleLevel, const Color3& tintColor);
    void setBloom(float intensity, float size, float threshold);

    const GeometryBatch& getFullscreenTriangle() const
    {
        return *fullscreenTriangle;
    }

    void onDeviceLost();

    struct GBuffer
    {
        shared_ptr<Texture> mainColor;
        shared_ptr<Framebuffer> mainFB;

        shared_ptr<Texture> gbufferColor;
        shared_ptr<Framebuffer> gbufferColorFB;

        shared_ptr<Texture> gbufferDepth;
        shared_ptr<Framebuffer> gbufferDepthFB;

        shared_ptr<Framebuffer> gbufferFB;
    };

    GBuffer* getGBuffer() const
    {
        return gbuffer.get();
    }

    Texture* getMSAAResolved() const
    {
        return msaa->getResolved();
    }

    const TextureRef& getGBufferColor() const
    {
        return gbufferColor;
    }
    const TextureRef& getGBufferDepth() const
    {
        return gbufferDepth;
    }

    const TextureRef& getShadowMap() const
    {
        return shadowMapTexture;
    }

private:
    VisualEngine* visualEngine;

    Vector3 pointOfInterest;

    float sqMinPartDistance;
    Vector3 sqMinDistanceCenter;

    scoped_ptr<SpatialHashedScene> spatialHashedScene;

    std::vector<CullableSceneNode*> renderNodes;
    scoped_ptr<RenderQueue> renderQueue;
    scoped_ptr<RenderQueue> shadowRenderQueue;

    bool skyEnabled;

    Color4 clearColor;

    GlobalShaderData globalShaderData;

    scoped_ptr<GeometryBatch> fullscreenTriangle;

    scoped_ptr<Sky> sky;

    scoped_ptr<GBuffer> gbuffer;
    TextureRef gbufferColor;
    TextureRef gbufferDepth;

    scoped_ptr<SSAO> ssao;
    scoped_ptr<MSAA> msaa;
    scoped_ptr<FXAA> fxaa;
    scoped_ptr<Glow> glow;
    scoped_ptr<Blur> blur;
    scoped_ptr<EnvMap> envMap;
    scoped_ptr<ImageProcess> imageProcess;

    scoped_ptr<ShadowMap> shadowMaps[3];
    float shadowMapTexelSize;
    TextureRef shadowMask;
    TextureRef shadowMapTexture;

    PostProcessSettings postProcessSettings;

    double curTimeOfDay;
    bool gbufferError; // when set to true, gbuffer is permanently disabled
    bool msaaError;

    struct ShadowValues
    {
        float intensity;
    };

    ShadowValues unsetAndGetShadowValues(DeviceContext* context);
    void restoreShadows(DeviceContext* context, const ShadowValues& shadowValues);

    void updateMSAA(unsigned width, unsigned height);

    void updateGBuffer(unsigned width, unsigned height);
    void resolveGBuffer(DeviceContext* context, Texture* texture);

    void renderShadowMap(DeviceContext* context, ShadowMap* shadowMap);
    RenderCamera getShadowCamera(const Vector3& center, int shadowMapSize, float shadowRadius, float shadowDepthRadius);
    void blurShadowMap(DeviceContext* context, ShadowMap* shadowMap);

    ShadowMap* pickShadowMap(int qualityLevel) const;
};

} // namespace Graphics
} // namespace Aya
