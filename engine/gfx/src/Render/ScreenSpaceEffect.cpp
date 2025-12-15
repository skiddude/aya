
#include "Render/ScreenSpaceEffect.hpp"

#include "Core/Device.hpp"
#include "Core/Framebuffer.hpp"
#include "Core/States.hpp"
#include "Render/VisualEngine.hpp"

#include "Render/VisualEngine.hpp"
#include "Render/ShaderManager.hpp"
#include "Render/SceneManager.hpp"
#include "Profiler.hpp"

namespace Aya
{
namespace Graphics
{
ShaderProgram* ScreenSpaceEffect::renderFullscreenBegin(DeviceContext* context, VisualEngine* visualEngine, const char* vsName, const char* fsName,
    const BlendState& blendState, unsigned fbWidth, unsigned fbHeight)
{
    ShaderProgram* program = visualEngine->getShaderManager()->getProgram(vsName, fsName).get();

    if (program)
    {
        const float textureSize[] = {(float)fbWidth, (float)fbHeight, 1 / (float)fbWidth, 1 / (float)fbHeight};

        context->setRasterizerState(RasterizerState::Cull_None);
        context->setBlendState(blendState);
        context->setDepthState(DepthState(DepthState::Function_Always, false));

        context->bindProgram(program);
        context->setConstant(program->getConstantHandle("TextureSize"), textureSize, 1);
    }

    return program;
}

void ScreenSpaceEffect::renderFullscreenEnd(DeviceContext* context, VisualEngine* visualEngine)
{
    context->draw(visualEngine->getSceneManager()->getFullscreenTriangle());
}

void ScreenSpaceEffect::renderBlur(DeviceContext* context, VisualEngine* visualEngine, Framebuffer* fb, Framebuffer* intermediateFB, Texture* texture,
    Texture* intermediate, float strength)
{
    AYAASSERT(texture->getWidth() == intermediate->getWidth() && texture->getHeight() == intermediate->getHeight() &&
              texture->getFormat() == intermediate->getFormat());

    AYAPROFILER_SCOPE("Render", "blurRender");
    AYAPROFILER_SCOPE("GPU", "blurRender");

    // find optimal shader
    const char blurSamples[] = {3, 5, 7};
    char bestBlur = blurSamples[0];
    for (int i = 0; i < ARRAYSIZE(blurSamples); ++i)
    {
        bestBlur = blurSamples[i];
        if (strength < blurSamples[i])
            break;
    }

    std::string nameFS = "Blur";
    nameFS += (bestBlur + '0');
    nameFS += "FS";

    context->bindFramebuffer(intermediateFB);

    if (ShaderProgram* program = ScreenSpaceEffect::renderFullscreenBegin(
            context, visualEngine, "PassThroughVS", nameFS.c_str(), BlendState::Mode_None, intermediateFB->getWidth(), intermediateFB->getHeight()))
    {
        float params[] = {1.f / texture->getWidth(), 0, strength, 0};
        context->setConstant(program->getConstantHandle("Params1"), params, 1);
        context->bindTexture(0, texture, SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));

        ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
    }

    context->bindFramebuffer(fb);

    if (ShaderProgram* program = ScreenSpaceEffect::renderFullscreenBegin(
            context, visualEngine, "PassThroughVS", nameFS.c_str(), BlendState::Mode_None, fb->getWidth(), fb->getHeight()))
    {
        context->bindTexture(0, intermediate, SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));
        float params[] = {0, 1.f / texture->getHeight(), strength, 1};
        context->setConstant(program->getConstantHandle("Params1"), params, 1);

        ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
    }
}

void ScreenSpaceEffect::renderFXAA(
    DeviceContext* context, VisualEngine* visualEngine, Framebuffer* fb, Framebuffer* intermediateFB, Texture* texture, Texture* intermediate)
{
    AYAPROFILER_SCOPE("Render", "fxaaRender");
    AYAPROFILER_SCOPE("GPU", "fxaaRender");

    context->bindFramebuffer(intermediateFB);

    if (ShaderProgram* program = ScreenSpaceEffect::renderFullscreenBegin(
            context, visualEngine, "PassThroughVS", "FXAAFS", BlendState::Mode_None, intermediateFB->getWidth(), intermediateFB->getHeight()))
    {
        float params[] = {1.f / texture->getWidth(), 0};
        context->setConstant(program->getConstantHandle("Params1"), params, 1);
        context->bindTexture(0, texture, SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));

        ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
    }

    context->bindFramebuffer(fb);

    if (ShaderProgram* program = ScreenSpaceEffect::renderFullscreenBegin(
            context, visualEngine, "PassThroughVS", "FXAAFS", BlendState::Mode_None, fb->getWidth(), fb->getHeight()))
    {
        context->bindTexture(0, intermediate, SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));
        float params[] = {0, 1.f / texture->getHeight(), 1};
        context->setConstant(program->getConstantHandle("Params1"), params, 1);

        ScreenSpaceEffect::renderFullscreenEnd(context, visualEngine);
    }
}
} // namespace Graphics
} // namespace Aya
